#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include "../../ks/ksJucePlugin/ksControls.h"
#include "../../kstation/kstationJucePlugin/kstationControls.h"

// Optional real-wrapper clock probe: SILVER_TEST_HOST_BPM=90 or 180.
// Use one pulse oscillator and widths around centre. At zero/full duty the
// waveform is silent, so audio onsets would no longer measure arp note timing.
class WrapperClock final:public juce::AudioPlayHead {
public:
 double bpm=juce::SystemStats::getEnvironmentVariable("SILVER_TEST_HOST_BPM",{}).getDoubleValue();
 double samples=0;
 bool editPulse=juce::SystemStats::getEnvironmentVariable("SILVER_TEST_NO_PULSE",{}).isEmpty();
 bool enabled()const{return bpm>0;}
 double beat()const{return samples*bpm/(60*48000);}
 void advance(){samples+=256;}
 juce::Optional<PositionInfo> getPosition()const override{
  PositionInfo p;p.setBpm(bpm);p.setPpqPosition(beat());p.setIsPlaying(true);p.setTimeInSamples(int64_t(samples));return p;
 }
 template<class Emit> static void setup(Emit emit,bool k){
  for(auto cc:{std::pair<int,int>{108,0},{110,127},{111,0},{92,0},{91,0},{93,0},{9,30},{87,9},{88,32},{89,12},{7,127},{72,127},{73,0},{74,0}})emit(cc.first,cc.second);
  if(k)emit(70,3);else{emit(99,0);emit(98,50);emit(6,3);}
 }
 static void setupParameters(juce::AudioProcessor& plugin,bool k){
  const auto set=[&](const char* id,int value){
   if(k){for(size_t n=0;n<kstation::controls::Catalog.size();++n){const auto& c=kstation::controls::Catalog[n];if(juce::String(c.id)==id)plugin.getParameters()[int(ksPlugin::controls.size()*4+n)]->setValueNotifyingHost(float(value-c.minimum)/(c.maximum-c.minimum));}}
   else{for(size_t n=0;n<ksPlugin::controls.size();++n){const auto& c=ksPlugin::controls[n];if(juce::String(c.id)==id)plugin.getParameters()[int(n)]->setValueNotifyingHost(float(value-c.minimum)/(c.maximum-c.minimum));}}
  };
  for(auto v:{std::pair<const char*,int>{"arp_on",1},{"arp_keysync",1},{"arp_latch",0},{"arp_rate",30},{"arp_sync",9},{"arp_gate",32},{"volume",127}})set(v.first,v.second);
  set("osc1_wave",3);set("osc1_level",127);set("osc2_level",0);set("osc3_level",0);
  set(k?"amp_attack":"attack",0);set(k?"amp_release":"release",0);set(k?"amp_sustain":"sustain",127);
  set(k?"delay_level":"delay",0);set(k?"reverb_level":"reverb",0);set(k?"chorus_level":"chorus",0);
 }
 static void pulse(juce::AudioProcessor& plugin,bool k,int value){
  if(k){for(size_t n=0;n<kstation::controls::Catalog.size();++n)if(juce::String(kstation::controls::Catalog[n].id)=="osc1_pulse_width")plugin.getParameters()[int(ksPlugin::controls.size()*4+n)]->setValueNotifyingHost(value/127.f);}
  else{for(size_t n=0;n<ksPlugin::controls.size();++n)if(juce::String(ksPlugin::controls[n].id)=="osc1_pulse_width")plugin.getParameters()[int(n)]->setValueNotifyingHost(value/127.f);}
 }
 static int64_t underruns(juce::AudioProcessor& plugin){
  juce::MemoryBlock saved;plugin.getStateInformation(saved);auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize()));
  if(xml)if(auto* component=xml->getChildByName("IComponent")){juce::MemoryBlock raw;raw.fromBase64Encoding(component->getAllSubText());xml=juce::AudioProcessor::getXmlFromBinary(raw.getData(),int(raw.getSize()));}
  return xml?xml->getStringAttribute("underruns","-1").getLargeIntValue():-1;
 }
 bool verify(const juce::AudioBuffer<float>& capture)const{
  if(!enabled())return true;
  float peak=0;for(int n=160;n<640;++n)peak=std::max(peak,capture.getRMSLevel(0,n*256,256));
  std::vector<int> attacks;bool sounding=capture.getRMSLevel(0,159*256,256)>peak*.06f;
  for(int n=160;n<640;++n){const bool on=capture.getRMSLevel(0,n*256,256)>peak*.06f;if(on && !sounding)attacks.push_back(n*256);sounding=on;}
  if(peak<1e-5 || attacks.size()<3)return false;
  double mean=0;for(size_t n=1;n<attacks.size();++n)mean+=attacks[n]-attacks[n-1];mean/=attacks.size()-1;
  std::cout<<"host_bpm="<<bpm<<" arp_gaps=";for(size_t n=1;n<attacks.size();++n)std::cout<<attacks[n]-attacks[n-1]<<',';std::cout<<std::endl;
  if(std::abs(mean-48000*60/bpm)>=512)return false;
  for(size_t n=1;n<attacks.size();++n)if(std::abs(attacks[n]-attacks[n-1]-mean)>=768)return false;
  return true;
 }
};
