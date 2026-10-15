#pragma once

#include "performanceDiagnostics.h"
#include <atomic>
#include <cstdint>

namespace baseLib
{
// One worker waits; audio/control threads notify after publishing progress.
// Only Windows changes its wait strategy. Other platforms keep the 50 us poll.
class WorkerWakeup
{
    static_assert(sizeof(std::atomic<uint32_t>)==sizeof(uint32_t) && std::atomic<uint32_t>::is_always_lock_free,
                  "WaitOnAddress requires a lock-free 32-bit value");
public:
    enum class Result { Skipped, Notified, Timeout, Spurious, Poll };
    explicit WorkerWakeup(bool forcePolling=false) noexcept;
    const char* backend() const noexcept;
    bool usesAddressWait() const noexcept {return addressWait;}
    void notify(performance::Diagnostics* diagnostics=nullptr) noexcept;

    template<class Predicate>
    Result waitWhile(Predicate stillFull,performance::Diagnostics* diagnostics=nullptr,bool allowAddress=true) noexcept
    {
#ifdef _WIN32
        // Arm BEFORE rechecking stop/pause/progress. The seq_cst exchanges on
        // armed also publish preceding progress even if no waiter was armed:
        // they prevent the store/load race from losing the only notification.
        const bool native=addressWait && allowAddress;
        nativeArmed.store(native,std::memory_order_relaxed);
        const auto observed=sequence.load(std::memory_order_acquire);
        armed.exchange(true,std::memory_order_seq_cst);
        if(!stillFull()){armed.exchange(false,std::memory_order_seq_cst);return Result::Skipped;}
        return wait(observed,diagnostics,native);
#else
        (void)stillFull;
        (void)allowAddress;
        return wait(0,diagnostics,false);
#endif
    }

private:
    Result wait(uint32_t observed,performance::Diagnostics*,bool native) noexcept;
    const bool addressWait;
    alignas(64) std::atomic<uint32_t> sequence{0};
    std::atomic<bool> armed{false};
    std::atomic<bool> nativeArmed{false};
    std::atomic<uint64_t> notificationTime{0},notificationEpoch{0};
};
}
