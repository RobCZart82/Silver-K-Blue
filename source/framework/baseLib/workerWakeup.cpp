#include "workerWakeup.h"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
namespace
{
struct AddressApi
{
    using Wait=BOOL(WINAPI*)(volatile VOID*,PVOID,SIZE_T,DWORD);
    using Wake=VOID(WINAPI*)(PVOID);
    Wait wait=nullptr;
    Wake wake=nullptr;
    AddressApi() noexcept
    {
        if(const auto module=GetModuleHandleW(L"kernelbase.dll"))
        {
            wait=reinterpret_cast<Wait>(GetProcAddress(module,"WaitOnAddress"));
            wake=reinterpret_cast<Wake>(GetProcAddress(module,"WakeByAddressSingle"));
        }
    }
};
const AddressApi& api() noexcept {static const AddressApi value;return value;}
bool useAddressWait(bool forcePolling) noexcept
{
    // Read only during construction, never in the callback. A poll override is
    // useful for matched A/B captures on the SAME binary; not a saved preference.
    const auto* overrideValue=std::getenv("SILVER_WORKER_WAIT");
    return !forcePolling && !(overrideValue && std::strcmp(overrideValue,"poll")==0) && api().wait && api().wake;
}
}
#endif

namespace baseLib
{
WorkerWakeup::WorkerWakeup(bool forcePolling) noexcept
#ifdef _WIN32
    :addressWait(useAddressWait(forcePolling))
#else
    :addressWait(false)
#endif
{
    (void)forcePolling;
}
const char* WorkerWakeup::backend() const noexcept
{
    return addressWait?"windows-address":"poll-50us";
}
void WorkerWakeup::notify(performance::Diagnostics* diagnostics) noexcept
{
#ifdef _WIN32
    if(!armed.exchange(false,std::memory_order_seq_cst))return;
    const auto epoch=diagnostics?diagnostics->epoch():0;
    notificationTime.store(epoch?performance::now():0,std::memory_order_relaxed);
    notificationEpoch.store(epoch,std::memory_order_relaxed);
    sequence.fetch_add(1,std::memory_order_release);
    if(nativeArmed.load(std::memory_order_relaxed))api().wake(&sequence);
#else
    (void)diagnostics;
#endif
}
WorkerWakeup::Result WorkerWakeup::wait(uint32_t observed,performance::Diagnostics* diagnostics,bool native) noexcept
{
    Result result=Result::Poll;
#ifdef _WIN32
    if(native)
    {
        // Bounded safety timeout for host stalls and unexpected OS failures;
        // normal audio/control progress explicitly wakes us. No spin loop.
        const auto ok=api().wait(&sequence,&observed,sizeof(observed),10);
        result=ok?Result::Spurious:GetLastError()==ERROR_TIMEOUT?Result::Timeout:Result::Spurious;
    }
    else
#endif
        std::this_thread::sleep_for(std::chrono::microseconds(50));
#ifdef _WIN32
    armed.exchange(false,std::memory_order_seq_cst);
    if(sequence.load(std::memory_order_acquire)!=observed)
    {
        const auto epoch=notificationEpoch.load(std::memory_order_relaxed);
        const auto begin=notificationTime.load(std::memory_order_relaxed);
        if(diagnostics && epoch && begin)diagnostics->add(performance::Metric::WakeLatency,epoch,performance::now()-begin);
        return Result::Notified;
    }
#else
    (void)observed;(void)diagnostics;(void)native;
#endif
    return result;
}
}
