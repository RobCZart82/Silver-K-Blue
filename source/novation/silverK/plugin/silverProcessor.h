#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "ksLib/ksDevice.h"
#include <atomic>
#include "../../ks/ksJucePlugin/ksControls.h"
#include "../../kstation/kstationJucePlugin/kstationControls.h"
#include "silverLibrary.h"
namespace silverK {
using namespace ksPlugin;
struct KEngine;
class Processor final : public juce::AudioProcessor,private juce::AudioProcessorValueTreeState::Listener {
public:
 Processor();~Processor() override;
 const juce::String getName()const override{return "Silver-K";}
 void prepareToPlay(double,int)override;void releaseResources()override{}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override;
 bool isBusesLayoutSupported(const BusesLayout&)const override;
 bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}bool isMidiEffect()const override{return false;}
 double getTailLengthSeconds()const override{return 20;}
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override;
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}
 const juce::String getProgramName(int)override{return "KS";}void changeProgramName(int,const juce::String&)override{}
 void getStateInformation(juce::MemoryBlock&)override;void setStateInformation(const void*,int)override;
 bool load(const juce::File&,const juce::File&,const juce::File&,juce::String&);
 juce::String status()const;uint64_t underruns()const;
 juce::AudioProcessorValueTreeState parameters;
 juce::File osFile,bankFile,flashFile;
 struct Preset {juce::String name;std::array<uint8_t,270> data;uint8_t bank=0,number=0;};
 struct Performance {juce::String name;std::array<uint8_t,1038> data;};
 std::vector<Preset> presets()const;
 bool importPresets(const juce::File&,juce::String&);
 bool selectPreset(int,juce::String&);
 std::vector<Performance> performances()const;
 bool selectPerformance(int,juce::String&);
 bool setPerformanceMode(bool,juce::String&);
 bool setPartActive(unsigned,bool,juce::String&);
 void selectPart(unsigned part){selectedPart.store(std::min(part,ks::PartCount-1));}
 unsigned currentPart()const{return selectedPart.load();}
 bool performanceMode()const{return multi.load();}
 uint8_t activeParts()const{return activeMask.load();}
 static juce::String parameterId(const Control& c,unsigned part){return part?"part"+juce::String(part+1)+"_"+c.id:juce::String(c.id);}
 bool sendShortMidi(uint8_t,uint8_t,uint8_t=0);
 void refreshControls();
 juce::String currentName()const;
 uint8_t midiChannel()const{return multi.load()?partChannels[currentPart()].load():controlChannel.load();}
 bool firmwareReady()const{return device.load()!=nullptr || kDevice.load()!=nullptr;}

bool isKStation()const{return kMode.load();}
 bool loadFirmware(const juce::File&,juce::String&);
 bool setRomFolder(const juce::File&,juce::String&);
 struct FirmwareChoice {juce::File file;bool kStation;};
 std::vector<FirmwareChoice> firmwares;
 juce::File romFolder;
 juce::String skinChoice="Auto",customSkin,currentPatch;
 std::shared_ptr<Library> libraryOwner;
 Library& patchLibrary;
 std::vector<Control> visibleControls()const;
 const Control* spec(const juce::String&)const;
 juce::RangedAudioParameter* uiParameter(const juce::String&);
 juce::String uiParameterId(const juce::String& id)const;
 bool activatePatch(const pluginLib::patchDB::PatchPtr&,juce::String&);
 bool saveCurrentPatch(const juce::File&,juce::String&);
 uint8_t editorChannel()const{return isKStation()?controlChannel.load():midiChannel();}
 void setEditorChannel(uint8_t c){controlChannel.store(c&15);}
private:
 std::atomic<bool> kMode{false};
 std::atomic<KEngine*> kDevice{nullptr};
 std::unique_ptr<KEngine> ownedK;
 std::vector<uint8_t> kRom,kState;
 std::array<uint8_t,256> kWire{},kMask{};
 void rebuildK();
 void processK(KEngine&,juce::AudioBuffer<float>&,juce::MidiBuffer&);
 void stopEngines();
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 void parameterChanged(const juce::String&,float)override;
 static constexpr size_t ParameterCount=std::size(controls)*ks::PartCount;
 static constexpr size_t KCount=kstation::controls::Catalog.size();
 static constexpr size_t TotalCount=ParameterCount+KCount;
 std::array<juce::String,TotalCount> parameterIds;
 std::array<std::atomic<int>,TotalCount> pending;
 std::unique_ptr<ks::Device> ownedDevice;
 kstation::Queue<uint8_t,65536> uiMidi;
 std::mutex uiMidiMutex;
 std::vector<Preset> library;size_t libraryBaseCount=0;
 std::vector<Performance> performanceLibrary;size_t performanceBaseCount=0;
 std::array<ks::Program,ks::PartCount> wireShadow{},wirePendingMask{};
 std::atomic<bool> resetWire{false};
 std::array<std::atomic<int>,TotalCount> awaiting;
 std::array<std::atomic<uint64_t>,TotalCount> editTime;
 std::atomic<unsigned> selectedPart{0};
 std::atomic<bool> multi{false};
 std::atomic<uint8_t> activeMask{1};
 std::array<std::atomic<uint8_t>,ks::PartCount> partChannels{};
 bool queueState(const ks::State&,bool selectMode,juce::String&);
 void resetPending();
 void buildLibrary();
 bool queueUiMidi(const uint8_t*,size_t);

 std::atomic<ks::Device*> device{nullptr};
 std::atomic<unsigned> activeAudio{0};
 mutable std::mutex controlMutex;
 std::vector<uint8_t> os,bank,flash,programToRestore;
 std::atomic<uint8_t> controlChannel{0};
 std::atomic<uint8_t> volume{127};
 juce::String message="Load KS OS, external wave bank and user Flash. Zero bank is supported.";
 double hostRate=48000;int hostBlock=256;
 bool placeholder=false;
 void rebuild();
};
}
