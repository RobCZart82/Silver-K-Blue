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
// A separate, explicitly requested post-restore fixture. Entering Performance
// queues state and can clear startup owners, so this is NOT cold-start proof.
static void ksChannelMatrix(Processor& p,ReentrantHost& host){
 if(p.isKStation())return;
 if(juce::SystemStats::getEnvironmentVariable("SILVER_TEST_KS_CHANNEL_MATRIX",{})!="1"){
  std::cout<<"NOT RUN: optional KS four-part/16-channel matrix (SILVER_TEST_KS_CHANNEL_MATRIX=1)\n";return;
 }
 p.setNonRealtime(true);juce::String error;
 check(p.setPerformanceMode(true,error),error.toRawUTF8());
 auto set=[&](unsigned part,const char* id,int value){
  auto* parameter=p.parameters.getParameter((part?"part"+juce::String(part+1)+"_":juce::String{})+id);
  check(parameter!=nullptr,id);parameter->setValueNotifyingHost(parameter->convertTo0to1(float(value)));
 };
 auto capture=[&](int channel,int action){
  juce::AudioBuffer<float> audio(4,host.block);juce::MidiBuffer midi;float peak=0;
  const int blocks=int(host.rate*.6)/host.block+1;
  for(int n=0;n<blocks;++n){audio.clear();midi.clear();
   if(n==0){
    if(action==1){midi.addEvent(juce::MidiMessage::controllerEvent(channel,64,127),0);midi.addEvent(juce::MidiMessage::noteOn(channel,60,juce::uint8(100)),0);}
    if(action==2)midi.addEvent(juce::MidiMessage::noteOff(channel,60),0);
    if(action==3)midi.addEvent(juce::MidiMessage::controllerEvent(channel,64,0),0);
   }
   p.processBlock(audio,midi);
   for(int c=0;c<4;++c)for(int i=0;i<host.block;++i){const auto v=audio.getSample(c,i);check(std::isfinite(v),"Non-finite KS channel matrix output");if(n>=blocks/2)peak=std::max(peak,std::abs(v));}
  }return peak;
 };
 // Apply the queued mode dump before editing per-part parameters.
 capture(1,0);p.refreshControls();
 check(p.performanceMode(),"Matrix Performance mode did not reach firmware");
 for(unsigned group=0;group<4;++group){
  for(unsigned part=0;part<4;++part){
   set(part,"part_channel",int(group*4+part));set(part,"part_output",0);
   set(part,"part_low",0);set(part,"part_high",127);set(part,"part_transpose",64);set(part,"part_fine",64);
  }
  check(capture(1,0)<=1e-5f,"KS channel assignment left an audible note");
  // Confirm firmware state, not just the frontend parameter values.
  juce::MemoryBlock saved;p.getStateInformation(saved);
  auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize()));check(xml!=nullptr,"Matrix state XML missing");
  juce::MemoryBlock dump;check(dump.fromBase64Encoding(xml->getStringAttribute("program"))&&dump.getSize()==1038,"Matrix Performance dump missing");
  const auto* bytes=static_cast<const uint8_t*>(dump.getData());
  check((bytes[13+190]&15)==15,"Matrix requires all four active parts");
  for(unsigned part=0;part<4;++part)check((bytes[13+part*256+181]&15)==group*4+part,"Matrix channel assignment not applied by firmware");
  for(unsigned part=0;part<4;++part){
   const int channel=int(group*4+part)+1;
   const auto played=capture(channel,1),held=capture(channel,2),released=capture(channel,3);
   std::cout<<"KS matrix part="<<part+1<<" channel="<<channel<<" played="<<played<<" held="<<held<<" released="<<released<<'\n';
   check(played>1e-5f && held>1e-5f,"KS matrix positive sustain control was silent");
   check(released<=1e-5f,"KS matrix retained an audible note after pedal release");
  }
 }
 check(p.setPerformanceMode(false,error),error.toRawUTF8());p.selectPart(0);capture(1,0);p.refreshControls();
 std::cout<<"PASS: post-restore KS four-part sustain matrix across all 16 MIDI channels (not cold-start coverage)\n";
}
static void hostInput(Processor& p,ReentrantHost& host){
 if(juce::SystemStats::getEnvironmentVariable("SILVER_TEST_HOST_INPUT",{})!="1")return;
 auto layout=p.getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::mono());
 check(p.setBusesLayout(layout),"Mono host input layout rejected");p.setNonRealtime(true);
 auto set=[&](const char* id,int value){auto* parameter=p.uiParameter(id);check(parameter!=nullptr,id);parameter->setValueNotifyingHost(parameter->convertTo0to1(float(value)));};
 for(const char* id:{"osc1_level","osc2_level","osc3_level","noise_level","ring_level","delay_level","reverb_level","chorus_level","vocoder_balance","arp_on"})set(id,0);
 set("external_level",127);set("external_to_fx",1);set("volume",127);
 auto capture=[&](bool signal,int stereoMode=0){float peak=0;juce::AudioBuffer<float> audio(4,host.block);juce::MidiBuffer midi;
  for(int block=0;block<375;++block){audio.clear();for(int i=0;i<host.block;++i)audio.setSample(0,i,signal?float(.1*std::sin(6.283185307179586*440*(block*host.block+i)/host.rate)):0);
   if(stereoMode)for(int i=0;i<host.block;++i)audio.setSample(1,i,audio.getSample(0,i)*(stereoMode==2?-1.f:1.f));
   p.processBlock(audio,midi);for(int c=0;c<4;++c)for(int i=0;i<host.block;++i){const auto v=audio.getSample(c,i);check(std::isfinite(v),"Host input produced nonfinite output");if(block>187)peak=std::max(peak,std::abs(v));}
  }return peak;};
 // Optional local wrapper fixture: contains private asset paths, never publish it.
 const auto stateFolder=juce::SystemStats::getEnvironmentVariable("SILVER_TEST_EXPORT_INPUT_STATE",{});
 if(stateFolder.isNotEmpty()){
  capture(false); // Apply MIDI-backed controls before serializing the firmware state.
  juce::MemoryBlock state;p.getStateInformation(state);
  check(juce::File(stateFolder).getChildFile(p.isKStation()?"kstation-input.state":"ks-input.state").replaceWithData(state.getData(),state.getSize()),"Cannot write private wrapper fixture");
 }
 const auto quiet=capture(false),tone=capture(true),tail=capture(false);
 std::cout<<(p.isKStation()?"K-Station":"KS")<<" host rate="<<host.rate<<" block="<<host.block<<" mono input quiet="<<quiet<<" tone="<<tone<<" tail="<<tail<<'\n';
 check(tone>.0001f && quiet<tone*.001f && tail<tone*.001f,"KS host mono input positive control failed");
 layout.inputBuses.set(0,juce::AudioChannelSet::stereo());check(p.setBusesLayout(layout),"Stereo host input rejected");
 const auto stereo=capture(true,1),cancel=capture(true,2);
 std::cout<<(p.isKStation()?"K-Station":"KS")<<" host stereo="<<stereo<<" cancel="<<cancel<<'\n';
 check(std::abs(stereo-tone)<tone*.01f && cancel<tone*.001f,"KS stereo host mixing failed");
 const auto report=juce::JSON::parse(p.performanceReport());
 check(bool(report.getProperty("input_overflows_supported",false)),"KS input diagnostics unsupported");
 check(report.getProperty("input_overflows_scope",{}).toString()==(p.isKStation()?"kstation-host-and-dsp-input-queues":"ks-host-input-queue"),"Wrong input diagnostics scope");
 check(int(report.getProperty("input_overflows",-1))==0,"KS host input queue overflow");
 if(juce::SystemStats::getEnvironmentVariable("SILVER_TEST_INPUT_FILTER",{})=="1"){
  layout.inputBuses.set(0,juce::AudioChannelSet::mono());check(p.setBusesLayout(layout),"Filter mono layout");
  set("external_to_fx",0);set("external_trigger",0);
set(p.isKStation()?"amp_attack":"attack",0);set(p.isKStation()?"amp_decay":"decay",0);
  set(p.isKStation()?"amp_sustain":"sustain",127);set(p.isKStation()?"amp_release":"release",0);
  set("cutoff",127);set("resonance",0);set("filter_env",64);set("filter_keytrack",0);capture(false);
  auto note=[&](bool on){juce::AudioBuffer<float> audio(4,host.block);audio.clear();juce::MidiBuffer midi;
   midi.addEvent(on?juce::MidiMessage::noteOn(int(p.editorChannel())+1,60,juce::uint8(100)):juce::MidiMessage::noteOff(int(p.editorChannel())+1,60),0);p.processBlock(audio,midi);};
  note(true);const auto filtered=capture(true);
  note(false);const auto released=capture(true);
  if(!p.isKStation()){juce::MemoryBlock state;p.getStateInformation(state);auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));juce::MemoryBlock program;
   check(xml && program.fromBase64Encoding(xml->getStringAttribute("program")),"Filter fixture state");const auto* values=static_cast<const uint8_t*>(program.getData())+13;
   std::cout<<"Filter actual";for(unsigned offset:{37,42,46,47,62,63,64,65,89,123,179,180,181})std::cout<<' '<<offset<<'='<<unsigned(values[offset]);std::cout<<'\n';}
  std::cout<<(p.isKStation()?"K-Station":"KS")<<" input filtered="<<filtered<<" released="<<released<<'\n';
  check(filtered>.0001f && released<filtered*.001f,"Input filter/envelope positive control failed");
 }
 layout.inputBuses.set(0,juce::AudioChannelSet::disabled());check(p.setBusesLayout(layout),"Input disable failed");
 set("external_level",0);set("external_to_fx",0);set("osc1_level",127);
 capture(false); // Drain restored MIDI controls before prepareToPlay rebuilds the engine.
 std::cout<<"PASS: Processor mono/stereo input and scoped queue diagnostics\n";
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
 if(!p.isKStation())std::cout<<"KS fixture controller_mode="<<p.ksControllerMode()<<'\n';
 ksStartupSilence(p,host);
 ksChannelMatrix(p,host);
 hostInput(p,host);

 if(juce::SystemStats::getEnvironmentVariable("SILVER_TEST_INPUT_RATES",{})=="1"){
  // Rebuild the converters at each host rate, then repeat actual signal controls.
  for(const auto rate:{44100.,96000.}){host.prepare(rate,512);hostInput(p,host);}
  host.prepare(48000,256);
 }

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
 if(juce::SystemStats::getEnvironmentVariable("SILVER_TEST_INPUT_FILTER",{})=="1"){exercise(juce::File(k));exercise(juce::File(ks));}
 else{exercise(juce::File(ks));exercise(juce::File(k));}return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
