#include "silverProcessor.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

using silverK::Processor;
static void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}

// Cubase may synchronously prepare again when a latency change reaches it.
struct ReentrantHost final:juce::AudioProcessorListener {
 Processor& processor;
 double rate=48000;
 int block=256,notifications=0;
 const std::thread::id hostThread=std::this_thread::get_id();
 explicit ReentrantHost(Processor& p):processor(p){processor.addListener(this);}
 ~ReentrantHost()override{processor.removeListener(this);}
 void audioProcessorParameterChanged(juce::AudioProcessor*,int,float)override{}
 void audioProcessorChanged(juce::AudioProcessor*,const ChangeDetails& details)override{
  if(!details.latencyChanged)return;
  check(std::this_thread::get_id()==hostThread,"Latency notification escaped from firmware worker");
  check(processor.firmwareReady(),"Latency notification before engine publication");
  ++notifications;
  processor.status(); // Also requires controlMutex: must be free before notifying.
  juce::String error;processor.finishFirmwareLoad(error); // firmwareLoadMutex too.
  processor.prepareToPlay(rate,block);
  check(processor.firmwareReady(),"Host reentry lost the published engine");
 }
 void prepare(double newRate,int newBlock){rate=newRate;block=newBlock;processor.prepareToPlay(rate,block);}
};

static void render(Processor& p,ReentrantHost& host){
 std::cout<<"Render "<<(p.isKStation()?"K":"KS")<<" rate="<<host.rate<<" block="<<host.block<<" profile="<<p.bufferProfile()<<'\n';
 p.setNonRealtime(true);
 juce::AudioBuffer<float> audio(4,host.block);juce::MidiBuffer midi;
 if(p.isKStation()){
  // Use an explicit dry sustained tone, independent of the firmware's selected
  // factory program and envelope/arp phase after a state round trip.
  synthLib::SysexBuffer data(142,0);
  const uint8_t header[]{0xf0,0,0x20,0x29,1,0x41,0x7f,0,0,0x11,1,0,0};
  std::copy_n(header,13,data.begin());data.back()=0xf7;
  for(const auto& control:kstation::controls::Catalog)if(control.patchOffset>=0){
   int value=control.defaultValue;
   if(juce::String(control.id)=="amp_attack" || juce::String(control.id)=="amp_release")value=0;
   auto& byte=data[13+control.patchOffset];byte=uint8_t((byte&~control.patchMask)|((value<<control.patchShift)&control.patchMask));
  }
  juce::String error;auto patch=p.patchLibrary.createPatch(std::move(data),{});
  check(patch && p.activatePatch(patch,error),"Could not configure test tone");
  check(p.sendShortMidi(uint8_t(0xb0|p.editorChannel()),7,127),"Could not configure test volume");
  // Let the original UART consume the setup before the note.
  for(int n=0;n<int(host.rate*0.15)/host.block+1;++n){audio.clear();p.processBlock(audio,midi);}
 }
 const auto underruns=p.underruns();float peak=0;
 const int blocks=int(host.rate*0.3)/host.block+1;
 for(int n=0;n<blocks;++n){
  audio.clear();midi.clear();
  if(n==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,juce::uint8(100)),0);
  if(n==blocks/2)midi.addEvent(juce::MidiMessage::noteOff(1,60),0);
  p.processBlock(audio,midi);
  for(int channel=0;channel<2;++channel)for(int i=0;i<host.block;++i){
   const auto value=audio.getSample(channel,i);check(std::isfinite(value),"Non-finite offline output");peak=std::max(peak,std::abs(value));
  }
 }
 check(peak>1e-5f,"Offline render was silent");
 check(p.underruns()==underruns,"Offline render dropped audio frames");
 p.setNonRealtime(false);
 audio.clear();midi.clear();p.processBlock(audio,midi);
 check(p.underruns()==underruns,"Offline to realtime transition dropped audio frames");
}

static void ksStartupSilence(Processor& p,ReentrantHost& host){
 if(p.isKStation())return;
 p.setNonRealtime(true);
 auto set=[&](const char* id,int value){auto* parameter=p.uiParameter(id);check(parameter!=nullptr,id);parameter->setValueNotifyingHost(parameter->convertTo0to1(float(value)));};
 // Do not send Panic/All Notes Off or a fresh program dump here: those would
 // erase the warm-up key/sustain state this regression is intended to expose.
 for(const char* id:{"osc2_level","osc3_level","noise_level","ring_level","external_level","delay_level","reverb_level","chorus_level","arp_on","vocoder_balance","attack","release","resonance"})set(id,0);
 set("osc1_wave",2);set("osc1_level",127);set("sustain",127);set("cutoff",127);set("filter_env",64);set("volume",127);
 auto capture=[&](bool noteOn=false,bool noteOff=false,int sustain=-1){
  juce::AudioBuffer<float> audio(4,host.block);juce::MidiBuffer midi;float peak=0;
  const int blocks=int(host.rate*.6)/host.block+1;
  for(int n=0;n<blocks;++n){audio.clear();midi.clear();
   if(n==0 && sustain>=0)midi.addEvent(juce::MidiMessage::controllerEvent(int(p.editorChannel())+1,64,sustain),0);
   if(n==0 && noteOn)midi.addEvent(juce::MidiMessage::noteOn(int(p.editorChannel())+1,60,juce::uint8(100)),0);
   if(n==0 && noteOff)midi.addEvent(juce::MidiMessage::noteOff(int(p.editorChannel())+1,60),0);
   p.processBlock(audio,midi);
   for(int c=0;c<4;++c)for(int i=0;i<host.block;++i){const auto value=audio.getSample(c,i);check(std::isfinite(value),"Non-finite KS startup output");if(n>=blocks/2)peak=std::max(peak,std::abs(value));}
  }return peak;
 };
 const auto dry=capture();set("vocoder_balance",127);const auto vocoder=capture();set("vocoder_balance",0);
 set("osc1_level",0);capture();set("osc1_level",127);const auto mixer=capture();
 const auto played=capture(true);const auto released=capture(false,true);
 std::cout<<"KS startup peaks dry="<<dry<<" vocoder="<<vocoder<<" mixer="<<mixer<<" played="<<played<<" released="<<released<<'\n';
 check(dry<=1e-5f && vocoder<=1e-5f && mixer<=1e-5f,"KS warm-up left audible notes after mixer/vocoder edits");
 check(played>1e-5f,"KS startup silence fixture did not produce its positive-control note");
 check(released<=1e-5f,"KS startup fixture retained a released note");
 // Only after the untouched startup checks above: prove CC64 ownership with
 // an audible held note, Note Off while sustained, and pedal release. Never
 // use Panic/CC120/CC123 to make the release assertion pass.
 const auto pedalPlayed=capture(true,false,127);
 const auto pedalHeld=capture(false,true);
 const auto pedalReleased=capture(false,false,0);
 std::cout<<"KS sustain peaks played="<<pedalPlayed<<" held="<<pedalHeld<<" released="<<pedalReleased<<'\n';
 check(pedalPlayed>1e-5f && pedalHeld>1e-5f,"KS sustain fixture did not retain its positive-control note");
 check(pedalReleased<=1e-5f,"KS sustain pedal release retained an audible note");
 std::cout<<"PASS: KS startup silence on all four outputs, mixer/vocoder edits and sustain/release positive controls on the editor channel without Panic\n";
}
static void exercise(const juce::File& rom){
 Processor p;ReentrantHost host(p);host.prepare(48000,256);p.setBufferProfile(0);
 // Upstream ab91ba1: private KS assets remain caller supplied, never in CI.
 const auto data=juce::SystemStats::getEnvironmentVariable("SILVER_TEST_KS_DATA",{});
 const auto waves=juce::SystemStats::getEnvironmentVariable("SILVER_TEST_KS_WAVES",{});
 if(data.isNotEmpty())p.flashFile=juce::File(data);
 if(waves.isNotEmpty())p.bankFile=juce::File(waves);
 juce::String error;
 check(p.loadFirmware(rom,error),error.toRawUTF8());
 check(host.notifications>0,"Firmware load did not notify host latency");
 ksStartupSilence(p,host);
 render(p,host);
 const int loaded=host.notifications;
 host.prepare(44100,512);render(p,host);
 check(host.notifications>loaded,"Reconfiguration did not notify host latency");
 host.prepare(48000,1024);render(p,host);
 host.prepare(48000,256);render(p,host);
 const int prepared=host.notifications;
 host.prepare(48000,256);
 check(host.notifications==prepared,"Unchanged preparation notified recursively");
 p.setBufferProfile(3);check(host.notifications>prepared,"Buffer profile did not notify host latency");
 render(p,host);
 juce::MemoryBlock state;p.getStateInformation(state);
 p.setBufferProfile(0);
 const int beforeRestore=host.notifications;
 p.setStateInformation(state.getData(),int(state.getSize()));
 check(host.notifications>beforeRestore,"State restoration did not notify host latency");render(p,host);

 // Worker loads queue the notification; finishFirmwareLoad delivers it after
 // releasing both firmwareLoadMutex and controlMutex on the caller's thread.
 p.setBufferProfile(0);const int beforeAsync=host.notifications;
 check(p.beginFirmwareLoad(rom),"Could not start asynchronous firmware load");
 host.rate=44100;host.block=512;p.prepareToPlay(host.rate,host.block);
 while(!p.finishFirmwareLoad(error))std::this_thread::sleep_for(std::chrono::milliseconds(5));
 check(error.isEmpty(),error.toRawUTF8());
 check(p.firmwareReady(),"Asynchronous load lost engine");
 check(host.notifications>beforeAsync,"Asynchronous completion lost pending latency");render(p,host);
 std::cout<<"PASS: "<<(p.isKStation()?"K-Station":"KS")<<" synchronous host reentry, buffer/rate changes, state restore, worker completion, offline audio and realtime transition; notifications="<<host.notifications<<'\n';
}

int main(int argc,char** argv){std::cout<<std::unitbuf;try{
 const auto k=argc==3?juce::String(argv[1]):juce::SystemStats::getEnvironmentVariable("SILVER_TEST_K_OS",{});
 const auto ks=argc==3?juce::String(argv[2]):juce::SystemStats::getEnvironmentVariable("SILVER_TEST_KS_OS",{});
 if(k.isEmpty() || ks.isEmpty()){std::cout<<"SKIP: supply K and KS firmware paths as arguments or SILVER_TEST_K_OS/SILVER_TEST_KS_OS\n";return 77;}
 const auto root=juce::File(juce::SystemStats::getEnvironmentVariable("SILVER_K_DATA",juce::File::getSpecialLocation(juce::File::tempDirectory).getFullPathName())).getChildFile("silver-latency-"+juce::Uuid().toString());
 check(root.createDirectory().wasOk(),"Cannot create isolated test folder");
#if JUCE_WINDOWS
 _putenv_s("SILVER_K_DATA",root.getFullPathName().toRawUTF8());
#else
 setenv("SILVER_K_DATA",root.getFullPathName().toRawUTF8(),1);
#endif
 juce::ScopedJuceInitialiser_GUI gui;
 exercise(juce::File(ks));exercise(juce::File(k));return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
