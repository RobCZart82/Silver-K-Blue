// Load this task's AU bundle into this process without installing/registering it system-wide.
#include <juce_audio_utils/juce_audio_utils.h>
#include <AudioToolbox/AudioToolbox.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <array>
#include <cmath>

int main(int argc,char** argv) {
 if(argc!=4 && argc!=5)return 2;const bool multi=argc==5 && juce::String(argv[4])=="multi";const int channels=multi?4:2;juce::ScopedJuceInitialiser_GUI gui;
 auto path=juce::File(argv[1]).getFullPathName();
 auto url=CFURLCreateFromFileSystemRepresentation(nullptr,reinterpret_cast<const UInt8*>(path.toRawUTF8()),path.getNumBytesAsUTF8(),true);
 auto bundle=CFBundleCreate(nullptr,url);CFRelease(url);if(!bundle || !CFBundleLoadExecutable(bundle))return 3;
 auto factory=reinterpret_cast<AudioComponentFactoryFunction>(CFBundleGetFunctionPointerForName(bundle,CFSTR("Silver_KAUFactory")));if(!factory)return 4;
 // A private registration avoids colliding with a user-installed KS AU.
 AudioComponentDescription desc{kAudioUnitType_MusicDevice,'SkMt','TusP',0,0};
 auto component=AudioComponentRegister(&desc,CFSTR("Silver-K isolated test"),0x100,factory);
 AudioUnit unit=nullptr;const auto created=component?AudioComponentInstanceNew(component,&unit):OSStatus(-1);if(created){std::cerr<<"AudioComponentInstanceNew="<<created<<std::endl;return 5;}
 struct Cleanup {AudioUnit unit;~Cleanup(){AudioUnitUninitialize(unit);AudioComponentInstanceDispose(unit);}} cleanup{unit};
 juce::MemoryBlock state;if(!juce::File(argv[2]).loadFileAsData(state))return 6;
 CFPropertyListRef previous=nullptr;UInt32 size=sizeof(previous);
 if(AudioUnitGetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&previous,&size))return 7;
 auto dictionary=CFDictionaryCreateMutableCopy(nullptr,0,static_cast<CFDictionaryRef>(previous));CFRelease(previous);
 auto data=CFDataCreate(nullptr,static_cast<const UInt8*>(state.getData()),state.getSize());CFDictionarySetValue(dictionary,CFSTR("jucePluginState"),data);CFRelease(data);
 auto status=AudioUnitSetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&dictionary,sizeof(dictionary));CFRelease(dictionary);if(status)return 8;
 AudioStreamBasicDescription format{};format.mSampleRate=48000;format.mFormatID=kAudioFormatLinearPCM;format.mFormatFlags=kAudioFormatFlagsNativeFloatPacked|kAudioFormatFlagIsNonInterleaved;format.mBytesPerPacket=format.mBytesPerFrame=4;format.mFramesPerPacket=1;format.mChannelsPerFrame=2;format.mBitsPerChannel=32;
 UInt32 maximum=256;
 if(AudioUnitSetProperty(unit,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&maximum,sizeof(maximum)) || AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&format,sizeof(format)))return 9;
 if(multi && AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,1,&format,sizeof(format)))return 15;
 if(AudioUnitInitialize(unit))return 9;
 double latency=0;size=sizeof(latency);AudioUnitGetProperty(unit,kAudioUnitProperty_Latency,kAudioUnitScope_Global,0,&latency,&size);
 std::array<std::array<float,256>,4> output{};std::array<double,4> channelPeaks{};juce::AudioBuffer<float> capture(channels,192000);
 double peak=0,maxms=0;auto start=std::chrono::steady_clock::now();
 for(unsigned pos=0;pos<192000;pos+=256) {
  if(pos<=24000 && pos+256>24000){if(multi)for(auto ch:{0,1,8,3})MusicDeviceMIDIEvent(unit,0x90|ch,ch==3?90:69,100,24000-pos);else MusicDeviceMIDIEvent(unit,0x90,69,100,24000-pos);}
  if(pos<=72000 && pos+256>72000){if(multi)for(auto ch:{0,1,8,3})MusicDeviceMIDIEvent(unit,0x80|ch,ch==3?90:69,0,72000-pos);else MusicDeviceMIDIEvent(unit,0x80,69,0,72000-pos);}
  struct Buffers {UInt32 count;AudioBuffer buffers[2];} buffers{2,{{1,1024,output[0].data()},{1,1024,output[1].data()}}};
  AudioTimeStamp time{};time.mSampleTime=pos;time.mFlags=kAudioTimeStampSampleTimeValid;AudioUnitRenderActionFlags flags=0;
  auto begin=std::chrono::steady_clock::now();if(AudioUnitRender(unit,&flags,&time,0,256,reinterpret_cast<AudioBufferList*>(&buffers)))return 10;
  if(multi){buffers.buffers[0].mData=output[2].data();buffers.buffers[1].mData=output[3].data();if(AudioUnitRender(unit,&flags,&time,1,256,reinterpret_cast<AudioBufferList*>(&buffers)))return 16;}
  maxms=std::max(maxms,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
  for(int c=0;c<channels;++c)for(int i=0;i<256;++i){const auto v=output[c][i];if(!std::isfinite(v))return 11;channelPeaks[c]=std::max(channelPeaks[c],double(std::abs(v)));peak=std::max(peak,double(std::abs(v)));capture.setSample(c,pos+i,v);}
  std::this_thread::sleep_until(start+std::chrono::microseconds(int64_t((pos+256)*1000000.0/48000)));
 }
 CFPropertyListRef saved=nullptr;size=sizeof(saved);if(AudioUnitGetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&saved,&size))return 12;
 auto raw=static_cast<CFDataRef>(CFDictionaryGetValue(static_cast<CFDictionaryRef>(saved),CFSTR("jucePluginState")));
 auto xml=juce::AudioProcessor::getXmlFromBinary(CFDataGetBytePtr(raw),int(CFDataGetLength(raw)));const auto underruns=xml?xml->getStringAttribute("underruns","-1").getLargeIntValue():-1;CFRelease(saved);
 juce::WavAudioFormat wav;auto stream=juce::File(argv[3]).createOutputStream();auto writer=std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(stream.release(),48000,channels,24,{},0));if(!writer || !writer->writeFromAudioSampleBuffer(capture,0,192000))return 13;
 if(multi){std::cout<<"multi_channel_peaks=";for(auto value:channelPeaks){std::cout<<value<<",";if(value<0.001)return 17;}std::cout<<std::endl;}
 std::cout<<"format=AU mode=realtime peak="<<peak<<" underruns="<<underruns<<" latency_seconds="<<latency<<" max_callback_ms="<<maxms<<std::endl;
 return peak>0.001 && underruns==0?0:14;
}
