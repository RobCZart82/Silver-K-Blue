#include "silverProcessor.h"
#include "dsp56kBase/threadtools.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#if JUCE_WINDOWS
#include <Windows.h>
#endif

namespace {
using Clock=std::chrono::steady_clock;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Cpu {double process=0,caller=0;};
Cpu cpu(){
#if JUCE_WINDOWS
    auto seconds=[](const FILETIME& t){return double((uint64_t(t.dwHighDateTime)<<32)|t.dwLowDateTime)*1.e-7;};
    FILETIME created{},exited{},kernel{},user{};Cpu result;
    check(GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user)!=0,"GetProcessTimes failed");
    result.process=seconds(kernel)+seconds(user);
    check(GetThreadTimes(GetCurrentThread(),&created,&exited,&kernel,&user)!=0,"GetThreadTimes failed");
    result.caller=seconds(kernel)+seconds(user);return result;
#else
    throw std::runtime_error("CPU accounting fixture currently requires Windows");
#endif
}
struct Event {int frame;juce::MidiMessage message;};
std::vector<Event> passage(bool warm){
    std::vector<Event> result;
    auto on=[&](int frame,int note,int velocity){result.push_back({frame,juce::MidiMessage::noteOn(1,note,juce::uint8(velocity))});};
    auto off=[&](int frame,int note){result.push_back({frame,juce::MidiMessage::noteOff(1,note)});};
    if(warm){for(int n=0;n<8;++n){on(4800,36+7*n,90);off(86400,36+7*n);}
        std::stable_sort(result.begin(),result.end(),[](const Event& a,const Event& b){return a.frame<b.frame;});return result;}
    on(4800,60,100);off(52800,60);
    std::array<int,8> held{};held.fill(-1);
    for(int frame=53760,tick=0;frame<576000;frame+=1920,++tick){
        for(unsigned n=0;n<held.size();++n){if(held[n]>=0)off(frame,held[n]);held[n]=36+(tick*7+int(n)*5)%60;on(frame,held[n],64+held[n]%48);}
    }
    std::stable_sort(result.begin(),result.end(),[](const Event& a,const Event& b){return a.frame<b.frame;});return result;
}
struct Result {double wall=0,process=0,caller=0,peak=0;uint64_t missing=0,hash=14695981039346656037ull;int frames=0;};
Result render(silverK::Processor& p,int block,int requestedFrames,const std::vector<Event>& events,bool paced){
    juce::AudioBuffer<float> audio(4,block);juce::MidiBuffer midi;midi.ensureSize(16384);
    const int blocks=(requestedFrames+block-1)/block;size_t next=0;
    Result result;const auto beforeMiss=p.underruns();
    // Only read-only process/thread accounting here. Construction, event
    // allocation, firmware boot, warm-up, report formatting and IO are excluded.
    const auto before=cpu();const auto begin=Clock::now();
    for(int n=0;n<blocks;++n){
        midi.clear();const int cursor=n*block;
        while(next<events.size() && events[next].frame<cursor+block){midi.addEvent(events[next].message,events[next].frame-cursor);++next;}
        p.processBlock(audio,midi);
        for(int c=0;c<4;++c)for(int i=0;i<block;++i){const float value=audio.getSample(c,i);check(std::isfinite(value),"Non-finite audio");
            result.peak=std::max(result.peak,double(std::abs(value)));uint32_t bits=0;std::memcpy(&bits,&value,sizeof(bits));
            result.hash^=bits;result.hash*=1099511628211ull;
        }
        if(paced)std::this_thread::sleep_until(begin+std::chrono::nanoseconds(int64_t((n+1)*int64_t(block)*1000000000/48000)));
    }
    const auto end=Clock::now();const auto after=cpu();
    result.wall=std::chrono::duration<double>(end-begin).count();result.process=after.process-before.process;
    result.caller=after.caller-before.caller;result.missing=p.underruns()-beforeMiss;result.frames=blocks*block;return result;
}
}
int main(int argc,char** argv){try{
    if(argc!=5){std::cout<<"SKIP: KS OS/zip, Flash, wave, output JSON required\n";return 77;}
#if ! JUCE_WINDOWS
    std::cout<<"SKIP: Windows CPU accounting only; macOS measurement pending\n";return 77;
#endif
    juce::ScopedJuceInitialiser_GUI gui;
    const bool paced=juce::SystemStats::getEnvironmentVariable("SILVER_CPU_PACED","0")=="1";
    const int block=paced?512:128,profile=paced?3:0;
    const auto setupCpu=cpu();const auto setupStart=Clock::now();
    silverK::Processor p;p.prepareToPlay(48000,block);p.setNonRealtime(true);
    p.flashFile=juce::File(argv[2]);p.bankFile=juce::File(argv[3]);juce::String error;
    check(p.loadFirmware(juce::File(argv[1]),error),error.toRawUTF8());p.setBufferProfile(profile);
    auto set=[&](const char* id,int value){auto* parameter=p.uiParameter(id);if(!parameter)throw std::runtime_error(std::string("Missing fixture parameter: ")+id);parameter->setValueNotifyingHost(parameter->convertTo0to1(float(value)));};
    for(auto item:{std::pair<const char*,int>{"arp_on",0},{"amp_attack",0},{"amp_release",0},{"amp_sustain",127},{"delay_level",0},{"reverb_level",0},{"chorus_level",0},{"volume",127},{"osc1_level",127},{"osc2_level",0},{"osc3_level",0},{"cutoff",127}})set(item.first,item.second);
    const auto setupEnd=cpu();const double setupWall=std::chrono::duration<double>(Clock::now()-setupStart).count();
    dsp56k::ThreadTools::setCurrentThreadPriority(dsp56k::ThreadPriority::Highest);
    dsp56k::ThreadTools::setCurrentThreadRealtimeParameters(48000,block);
    const auto warmEvents=passage(true),events=passage(false);
    const auto warm=render(p,block,96000,warmEvents,false);check(warm.missing==0,"Offline warm-up lost frames");
    p.setNonRealtime(!paced);
    const auto result=render(p,block,576000,events,paced);
    const auto runtime=juce::JSON::parse(p.performanceReport());
    auto* root=new juce::DynamicObject;juce::var report(root);
    root->setProperty("schema","silver-k.cpu-cost.v1");root->setProperty("mode",paced?"paced-realtime":"offline-fixed-work");
    root->setProperty("dsp_execution_backend",runtime["dsp_execution_backend"]);root->setProperty("system",runtime["system"]);root->setProperty("build",runtime["build"]);
    root->setProperty("block_frames",block);root->setProperty("buffer_profile",profile);root->setProperty("latency_frames",p.getLatencySamples());
    root->setProperty("frames",result.frames);root->setProperty("audio_seconds",result.frames/48000.0);root->setProperty("wall_seconds",result.wall);
    root->setProperty("process_cpu_seconds",result.process);root->setProperty("caller_cpu_seconds",result.caller);
    root->setProperty("background_cpu_seconds",result.process-result.caller);
    root->setProperty("missing_frames",juce::int64(result.missing));root->setProperty("audio_hash",juce::String(result.hash));root->setProperty("peak",result.peak);
    root->setProperty("setup_cpu_seconds",setupEnd.process-setupCpu.process);root->setProperty("setup_wall_seconds",setupWall);
    root->setProperty("warmup_cpu_seconds",warm.process);root->setProperty("warmup_wall_seconds",warm.wall);
    root->setProperty("notes","CPU=user+kernel summed across all process threads via GetProcessTimes. Caller via GetThreadTimes includes host callback, hashing, offline queue waits and pacing calls. Background=process-caller, includes H8/DSP workers and any other process threads. No diagnostics session. Equal 576000 delivered frames required; compare hashes within each scenario. Boot/setup and two-second warm-up excluded from measured passage. No real driver/DAW, affinity or OS settings changes.");
    check(juce::File(argv[4]).replaceWithText(juce::JSON::toString(report,false)),"Cannot save report");
    std::cout<<juce::JSON::toString(report,false)<<'\n';
    return result.missing==0 && result.peak>1.e-4?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
