#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>

// Optional wall-time diagnostics, never a scheduler or a source of guest time.
// Each metric has ONE writer (audio callback or emulation worker). Readers use
// atomic snapshots; a live report is approximate, not a transaction.
namespace baseLib::performance
{
using Clock = std::chrono::steady_clock;
inline uint64_t now() noexcept
{
    return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count());
}
enum class Metric : size_t
{
    Callback, Controls, AudioRender, Worker, Sleep, Snapshot,
    H8, DspA, DspB, Serial, WakeLatency, DspBarrier, QueueDepth, MissingFrames, StaleFrames, HostFrames, OfflineCallbacks, Count
};
inline constexpr std::array<const char*, size_t(Metric::Count)> Names{
    "audio_callback", "audio_controls", "audio_render", "worker_segment", "worker_sleep", "worker_snapshot",
    "h8_sampled", "dsp_a_sampled", "dsp_b_sampled", "serial_sampled", "worker_wake_latency", "dsp_barrier_sampled", "queue_depth", "missing_frames", "stale_frames", "host_block_frames", "offline_callback"
};
inline constexpr size_t HistogramSize = 64;
struct Values
{
    uint64_t count=0, total=0, minimum=0, maximum=0, units=0, overBudget=0;
    std::array<uint64_t, HistogramSize> histogram{};
    uint64_t percentileUpperBound(unsigned percent) const noexcept
    {
        if (!count) return 0;
        const auto target = count / 100 * percent + (count % 100 * percent + 99) / 100;
        uint64_t seen=0;
        for (size_t i=0; i<histogram.size(); ++i)
        {
            seen += histogram[i];
            if (seen >= target) return i==63 ? std::numeric_limits<uint64_t>::max() : (uint64_t(1)<<i)-1;
        }
        return maximum; // a live reader may see an in-flight histogram update
    }
};
class Diagnostics
{
    static_assert(std::atomic<uint64_t>::is_always_lock_free, "Diagnostics require lock-free 64-bit atomics");
    struct alignas(64) Counter
    {
        std::atomic<uint64_t> generation{0}, count{0}, total{0}, minimum{0}, maximum{0}, units{0}, overBudget{0};
        std::array<std::atomic<uint64_t>, HistogramSize> histogram{};
        void add(uint64_t epoch, uint64_t value, uint64_t amount, uint64_t budget) noexcept
        {
            if (generation.load(std::memory_order_relaxed) != epoch)
            {
                generation.store(0, std::memory_order_release);
                count.store(0,std::memory_order_relaxed);total.store(0,std::memory_order_relaxed);
                minimum.store(std::numeric_limits<uint64_t>::max(),std::memory_order_relaxed);
                maximum.store(0,std::memory_order_relaxed);units.store(0,std::memory_order_relaxed);
                overBudget.store(0,std::memory_order_relaxed);
                for(auto& bin:histogram)bin.store(0,std::memory_order_relaxed);
                generation.store(epoch,std::memory_order_release);
            }
            total.store(total.load(std::memory_order_relaxed)+value,std::memory_order_relaxed);
            minimum.store(value<minimum.load(std::memory_order_relaxed)?value:minimum.load(std::memory_order_relaxed),std::memory_order_relaxed);
            maximum.store(value>maximum.load(std::memory_order_relaxed)?value:maximum.load(std::memory_order_relaxed),std::memory_order_relaxed);
            units.store(units.load(std::memory_order_relaxed)+amount,std::memory_order_relaxed);
            if(budget && value>budget)overBudget.store(overBudget.load(std::memory_order_relaxed)+1,std::memory_order_relaxed);
            size_t bin=0;for(auto remaining=value;remaining && bin<63;remaining>>=1)++bin;
            histogram[bin].store(histogram[bin].load(std::memory_order_relaxed)+1,std::memory_order_relaxed);
            count.store(count.load(std::memory_order_relaxed)+1,std::memory_order_release);
        }
        Values read(uint64_t epoch) const noexcept
        {
            Values v;
            if(!epoch || generation.load(std::memory_order_acquire)!=epoch)return v;
            v.count=count.load(std::memory_order_acquire);v.total=total.load(std::memory_order_relaxed);
            v.minimum=v.count?minimum.load(std::memory_order_relaxed):0;v.maximum=maximum.load(std::memory_order_relaxed);
            v.units=units.load(std::memory_order_relaxed);v.overBudget=overBudget.load(std::memory_order_relaxed);
            for(size_t i=0;i<HistogramSize;++i)v.histogram[i]=histogram[i].load(std::memory_order_relaxed);
            return generation.load(std::memory_order_acquire)==epoch?v:Values{};
        }
    };
    // Low two bits are mode (0=off, 1=summary, 2=detailed); upper bits identify
    // a session. Start never clears counters owned by another running thread.
    std::atomic<uint64_t> state{0};
    std::array<Counter,size_t(Metric::Count)> counters;
public:
    void start(bool detailed=false) noexcept
    {
        state.store(((state.load(std::memory_order_relaxed)>>2)+1)*4+(detailed?2:1),std::memory_order_release);
    }
    void stop() noexcept {state.fetch_and(~uint64_t(3),std::memory_order_acq_rel);}
    int mode() const noexcept {return int(state.load(std::memory_order_relaxed)&3);}
    uint64_t generation() const noexcept {return state.load(std::memory_order_acquire)>>2;}
    uint64_t epoch(bool detailed=false) const noexcept
    {
        const auto s=state.load(std::memory_order_acquire);
        return (s&3)>=(detailed?2u:1u)?s>>2:0;
    }
    void add(Metric metric,uint64_t epoch,uint64_t value,uint64_t units=0,uint64_t budget=0) noexcept
    {
        if(epoch && generation()==epoch)counters[size_t(metric)].add(epoch,value,units,budget);
    }
    Values read(Metric metric) const noexcept {return counters[size_t(metric)].read(generation());}
};
class Scope
{
    Diagnostics* diagnostics;
    Metric metric;
    uint64_t epoch=0, begin=0, units, budget;
public:
    Scope(Diagnostics* d,Metric m,uint64_t u=0,uint64_t b=0,bool detailed=false,bool sample=true) noexcept
        :diagnostics(d),metric(m),units(u),budget(b)
    {if(sample && d)epoch=d->epoch(detailed);if(epoch)begin=now();}
    ~Scope(){finish();}
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
    void finish() noexcept {if(epoch){diagnostics->add(metric,epoch,now()-begin,units,budget);epoch=0;}}
};
// Measure contiguous worker work, ending before sleeps and at most every 256
// generated frames. No clock read per sample. A session change drops only the
// unfinished segment; no guest state, queue or scheduling is touched.
class WorkerSegment
{
    Diagnostics* diagnostics;
    uint64_t epoch=0,begin=0,frames=0;
public:
    explicit WorkerSegment(Diagnostics* d):diagnostics(d){}
    ~WorkerSegment(){flush();}
    void start() noexcept
    {
        const auto current=diagnostics?diagnostics->epoch():0;
        if(current!=epoch){epoch=current;frames=0;begin=current?now():0;}
        else if(current && !begin)begin=now();
    }
    void frame() noexcept {if(epoch && ++frames>=256)flush();}
    void flush() noexcept
    {
        if(epoch && begin && frames)diagnostics->add(Metric::Worker,epoch,now()-begin,frames);
        begin=0;frames=0;
    }
};
}
