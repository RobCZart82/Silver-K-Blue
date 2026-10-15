#include "baseLib/performanceDiagnostics.h"
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace baseLib::performance;
static void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main()
{
    try
    {
        Diagnostics d;
        {Scope ignored(&d,Metric::Callback);}
        check(d.read(Metric::Callback).count==0,"Disabled scope recorded work");
        d.start();auto first=d.epoch();
        d.add(Metric::Callback,first,1000,128,500);
        d.add(Metric::Callback,first,3000,256,4000);
        auto v=d.read(Metric::Callback);
        check(v.count==2&&v.total==4000&&v.minimum==1000&&v.maximum==3000&&v.units==384&&v.overBudget==1,"Aggregates or deadline counts incorrect");
        check(v.percentileUpperBound(99)==4095,"Histogram percentile is not an upper bound");
        check(d.epoch(true)==0,"Summary enabled detailed probes");
        d.add(Metric::QueueDepth,first,0);d.add(Metric::QueueDepth,first,512);
        check(d.read(Metric::QueueDepth).minimum==0,"Zero queue depth lost");
        d.stop();check(!d.epoch()&&d.read(Metric::Callback).count==2,"Stop discarded recording");
        d.start(true);check(d.epoch(true)!=0&&d.epoch()!=first,"New detailed session did not advance epoch");
        check(d.read(Metric::Callback).count==0,"Previous session leaked into new session");
        d.add(Metric::Callback,first,999999);check(!d.read(Metric::Callback).count,"Stale scope wrote into new session");
        std::atomic<bool> done{false};
        std::thread writer([&]{const auto epoch=d.epoch();for(unsigned i=0;i<100000;++i)d.add(Metric::DspA,epoch,123,1);done=true;});
        while(!done.load())(void)d.read(Metric::DspA);
        writer.join();v=d.read(Metric::DspA);
        check(v.count==100000&&v.total==12300000&&v.units==100000,"Concurrent reader corrupted writer counters");
        {WorkerSegment worker(&d);worker.start();for(unsigned n=0;n<256;++n)worker.frame();}
        check(d.read(Metric::Worker).units==256,"Worker frame accounting incorrect");
        // Reset while both owners keep working; old tokens must not contaminate
        // the next session, including measurements which straddle Start.
        std::atomic<bool> running{true};
        std::thread owner([&]{while(running.load()){auto e=d.epoch();d.add(Metric::Callback,e,77);}});
        for(unsigned i=0;i<100;++i){d.stop();d.start(i%2!=0);(void)d.read(Metric::Callback);}
        running=false;owner.join();d.start();
        d.add(Metric::Callback,d.epoch(),9);v=d.read(Metric::Callback);
        check(v.count==1&&v.total==9,"Reset did not isolate session");
        std::cout<<"PASS: disabled mode, aggregate/deadline/histogram, queue zero, session isolation, concurrent snapshots, worker frames\n";
        return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
