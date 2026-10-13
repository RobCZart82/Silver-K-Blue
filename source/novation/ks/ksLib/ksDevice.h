#pragma once
#include "ksBoard.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
namespace ks {
// DSP/H8 ownership belongs to the worker. Host audio does bounded queue access.
class Device {
public:
    Device(const std::vector<uint8_t>& os,const std::vector<uint8_t>& bank,const std::vector<uint8_t>& flash,double hostRate,int block,const std::vector<uint8_t>& program={});
    ~Device();
    bool valid() const{return running.load();}
    const std::string& error()const{return failure;}
    bool midi(const uint8_t*,size_t,uint64_t offset=0);
    void process(float* left,float* right,size_t n,bool offline=false);
    void process(const std::array<float*,OutputCount>&,size_t n,bool offline=false);
    std::vector<uint8_t> dumpProgram(); // Control thread, original UART dump.
    std::array<uint64_t,4> diagnostics();
    std::vector<uint8_t> saveFlash(); // Excludes OS and external wave bank.
    std::array<uint8_t,256> snapshot(unsigned part=0)const;
    State fullSnapshot()const;
    uint64_t underruns()const{return underrun.load();}
    uint64_t overflows()const{return overflow.load();}
    unsigned latency()const{return lead;}
private:
    struct Event{uint64_t due;uint8_t value;};
    struct Frame{uint64_t due;std::array<float,OutputCount> value;};
    std::unique_ptr<Board> board;
    kstation::Queue<Event,65536> events;
    kstation::Queue<Frame,8192> output;
    struct SnapshotSlot{State value;mutable std::atomic<unsigned> readers{0};};
    std::array<SnapshotSlot,3> snapshots;
    std::atomic<unsigned> publishedSnapshot{0};
    State initialSnapshot;bool initialPublished=false;
    std::atomic<uint64_t> consumed{0},underrun{0},overflow{0};
    std::atomic<bool> stopping{false},running{false},pause{false},paused{false};
    std::thread worker;
    std::mutex startMutex,coldMutex;
    std::condition_variable startCV;
    bool started=false;
    unsigned lead;
    std::string failure;
    double rate,phase=0;
    uint64_t produced=0;
    std::array<std::array<double,OutputCount>,4> history{};
    std::array<float,OutputCount> nextSample();
    void publish();
    void suspend();
    void resume();
};
}
