#include "workerWakeup.h"
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>

static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main()
{
    using baseLib::WorkerWakeup;
    using namespace baseLib::performance;
    try
    {
        WorkerWakeup wake;
        Diagnostics diagnostics;diagnostics.start();
#ifdef _WIN32
        check(wake.waitWhile([]{return false;})==WorkerWakeup::Result::Skipped,"False predicate entered OS wait");
        // Deterministically notify in the arm -> OS wait window. WaitOnAddress
        // must observe the changed sequence even though no thread was parked.
        const auto result=wake.waitWhile([&]{wake.notify(&diagnostics);return true;},&diagnostics);
        check(result==WorkerWakeup::Result::Notified,"Notification before park was lost");
        check(diagnostics.read(Metric::WakeLatency).count==1,"Notification latency missing");
        std::atomic<unsigned> progress{0},acknowledged{0};
        std::thread worker([&]{
            for(unsigned n=1;n<=2000;++n)
            {
                while(progress.load(std::memory_order_acquire)<n)
                    wake.waitWhile([&]{return progress.load(std::memory_order_acquire)<n;},&diagnostics);
                acknowledged.store(n,std::memory_order_release);
            }
        });
        for(unsigned n=1;n<=2000;++n)
        {
            progress.store(n,std::memory_order_release);wake.notify(&diagnostics);
            while(acknowledged.load(std::memory_order_acquire)<n)std::this_thread::yield();
        }
        worker.join();check(acknowledged==2000,"Progress/notification race stalled worker");
        std::atomic<bool> stop{false},entered{false};
        std::thread parked([&]{entered=true;while(!stop.load())wake.waitWhile([&]{return !stop.load();});});
        while(!entered.load())std::this_thread::yield();
        stop=true;wake.notify();parked.join();
        // On a host without callbacks, the worker still returns periodically.
        if(wake.usesAddressWait())
        {
            auto timeout=WorkerWakeup::Result::Spurious;
            for(unsigned n=0;n<10 && timeout!=WorkerWakeup::Result::Timeout;++n)
                timeout=wake.waitWhile([]{return true;});
            check(timeout==WorkerWakeup::Result::Timeout,"Safety timeout not observed");
        }
        WorkerWakeup polling(true);
        check(!polling.usesAddressWait(),"Forced polling override ignored");
        check(polling.waitWhile([&]{polling.notify();return true;})==WorkerWakeup::Result::Notified,"Polling notification telemetry missing");
        check(wake.waitWhile([&]{wake.notify();return true;},nullptr,false)==WorkerWakeup::Result::Notified,"Per-wait polling guard lost notification telemetry");
#else
        check(!wake.usesAddressWait(),"Non-Windows backend changed");
        check(wake.waitWhile([]{return true;})==WorkerWakeup::Result::Poll,"Non-Windows polling changed");
#endif
        std::cout<<"PASS: "<<wake.backend()<<"; arm/recheck, notification before park, repeated progress, stop, timeout and poll fallback\n";
        return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
