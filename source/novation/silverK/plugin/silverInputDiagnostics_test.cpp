#include "silverProcessor.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
juce::var report(silverK::Processor& p){return juce::JSON::parse(p.performanceReport());}
int64_t drops(const juce::var& r){return r["input_overflows"].toString().getLargeIntValue();}
}
int main(int argc,char** argv){try{
 juce::ScopedJuceInitialiser_GUI gui;
 const auto base=juce::File(juce::SystemStats::getEnvironmentVariable("SILVER_K_DATA",juce::File::getSpecialLocation(juce::File::tempDirectory).getFullPathName()));
 const auto root=base.getNonexistentChildFile("input-diagnostics",{},false);
 check(root.createDirectory().wasOk(),"Cannot create isolated diagnostic profile");
#if JUCE_WINDOWS
 _putenv_s("SILVER_K_DATA",root.getFullPathName().toRawUTF8());
#else
 setenv("SILVER_K_DATA",root.getFullPathName().toRawUTF8(),1);
#endif
 silverK::Processor p;
 // Exercise the actual Processor layout contract in ROM-free platform CI.
 const auto original=p.getBusesLayout();
 for(const auto input:{juce::AudioChannelSet::disabled(),juce::AudioChannelSet::mono(),juce::AudioChannelSet::stereo()}){
  auto layout=original;layout.inputBuses.set(0,input);
  check(p.isBusesLayoutSupported(layout) && p.setBusesLayout(layout),"Supported input layout rejected");
  p.prepareToPlay(48000,256);juce::AudioBuffer<float> audio(4,256);juce::MidiBuffer midi;
  for(int c=0;c<4;++c)for(int n=0;n<256;++n)audio.setSample(c,n,.1f);
  p.processBlock(audio,midi);check(audio.getMagnitude(0,256)==0,"No-firmware input leaked into outputs");
 }
 auto invalid=original;invalid.inputBuses.set(0,juce::AudioChannelSet::discreteChannels(3));
 check(!p.isBusesLayoutSupported(invalid),"Unsupported three-channel input accepted");
 check(p.setBusesLayout(original),"Original input layout restore failed");
 auto empty=report(p);
 check(empty.hasProperty("input_overflows") && !bool(empty["input_overflows_supported"]) && drops(empty)==0,"Unavailable input counter reported as supported");
 check(empty["input_overflows_scope"].toString()=="not-applicable","Unavailable input scope");
 check(!p.setPerformanceDiagnostics(1),"Recording started without firmware");
 if(argc!=2){std::cout<<"PASS: ROM-free input diagnostic availability/schema; SKIP private K-Station engine checks (supply K OS)\n";return 0;}
 p.prepareToPlay(48000,256);p.setNonRealtime(true);juce::String error;
 check(p.loadFirmware(juce::File(argv[1]),error),error.toRawUTF8());
 check(p.isKStation(),"Supply K-Station firmware for this fixture");
 auto buses=p.getBusesLayout();buses.inputBuses.set(0,juce::AudioChannelSet::mono());check(p.setBusesLayout(buses),"Mono input layout rejected");
 check(p.setPerformanceDiagnostics(1),"Recording did not start");
 const auto start=report(p);const auto first=drops(start);
 check(bool(start["input_overflows_supported"]) && start["controller_backend"].toString()=="original-h8","K controller/counter unavailable");
 check(start["input_overflows_scope"].toString()=="kstation-host-and-dsp-input-queues","Wrong counter scope");
 check(start["session"]["controller_backend"].toString()=="original-h8" && bool(start["session"]["input_overflows_supported"]),"Session backend/counter metadata missing");
 check(start["session"]["input_overflows_at_start"].toString().getLargeIntValue()==first,"Session baseline differs from engine counter");
 juce::AudioBuffer<float> audio(4,256);juce::MidiBuffer midi;
 for(int block=0;block<200;++block){audio.clear();for(int n=0;n<256;++n)audio.setSample(0,n,.1f*std::sin(float(block*256+n)*.03f));p.processBlock(audio,midi);}
 const auto nominal=drops(report(p));check(nominal==first,"Offline mono input rejected samples");
 // Deliberately overrun the nonblocking realtime bridge with oversized host
 // bursts. This is a counter/backpressure fixture, not a realtime benchmark.
 p.setNonRealtime(false);juce::AudioBuffer<float> burst(4,131072);
 for(int attempt=0;attempt<8 && drops(report(p))==nominal;++attempt){burst.clear();for(int n=0;n<burst.getNumSamples();++n)burst.setSample(0,n,.1f);p.processBlock(burst,midi);}
 const auto after=drops(report(p));check(after>nominal,"Overload fixture did not record rejected input");
 check(p.setPerformanceDiagnostics(0),"Recording did not stop");
 check(drops(report(p))>=after,"Stopping diagnostics reset the engine counter");
 check(p.setPerformanceDiagnostics(1),"Recording did not restart");
 const auto restarted=report(p);
 check(drops(restarted)>=after && restarted["session"]["input_overflows_at_start"].toString().getLargeIntValue()>=after,"Restart reset engine counter/baseline");
 std::cout<<"PASS: K backend, mono input, induced overflow, session baseline and stop/restart lifetime; nominal_drops="<<nominal<<" overloaded_drops="<<drops(restarted)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
