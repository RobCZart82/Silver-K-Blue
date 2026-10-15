#include "silverProcessor.h"
#include "BinaryData.h"
namespace silverK {
juce::File Processor::settingsFile()const{
 const auto test=juce::SystemStats::getEnvironmentVariable("SILVER_K_DATA",{});
 return (test.isEmpty()?juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("The Usual Suspects/Silver-K"):juce::File(test)).getChildFile("settings.json");
}
bool Processor::updateSettings(const juce::NamedValueSet& edits,juce::String& error)const{
 // Merge changed keys so closing another instance cannot restore stale defaults.
 static std::mutex settingsMutex;std::lock_guard guard(settingsMutex);
 const auto file=settingsFile();juce::InterProcessLock lock("Silver-K-settings-"+juce::String::toHexString(file.getFullPathName().hashCode64()));
 if(!lock.enter(250)){error="Settings are busy; try again";return false;}
 struct Unlock{juce::InterProcessLock& lock;~Unlock(){lock.exit();}} unlock{lock};
 if(auto result=file.getParentDirectory().createDirectory();result.failed()){error=result.getErrorMessage();return false;}
 auto saved=juce::JSON::parse(file);if(!saved.isObject())saved=juce::var(new juce::DynamicObject);
 bool changed=false;for(int n=0;n<edits.size();++n){const auto key=edits.getName(n);const auto& value=edits.getValueAt(n);if(!saved.hasProperty(key)||saved[key]!=value){saved.getDynamicObject()->setProperty(key,value);changed=true;}}
 if(!changed)return true;
 juce::TemporaryFile temp(file);
 if(!temp.getFile().replaceWithText(juce::JSON::toString(saved)) || !temp.overwriteTargetFileWithTemporary()){error="Cannot save Silver-K settings: "+file.getFullPathName();return false;}
 return true;
}
juce::NamedValueSet Processor::preferenceValues()const{
 juce::NamedValueSet values;values.set("editorScale",editorScale.load());values.set("skinChoice",skinChoice);values.set("customSkin",customSkin);values.set("bufferProfile",buffering.load());values.set("ksControllerMode",ksControllerMode());values.set("editorChannel",int(editorChannelPreference.load()));values.set("browserState",browserState);return values;
}
void Processor::savePreferences(){
 if(isFirmwareLoading())return;
 std::lock_guard guard(controlMutex);const auto values=preferenceValues();juce::NamedValueSet edits;
 for(int n=0;n<values.size();++n)if(preferenceBaseline[values.getName(n)]!=values.getValueAt(n))edits.set(values.getName(n),values.getValueAt(n));
 if(edits.size()==0)return;juce::String error;if(updateSettings(edits,error))preferenceBaseline=values;else message=error;
}
void Processor::initializeDataFolder(){
 const auto test=juce::SystemStats::getEnvironmentVariable("SILVER_K_DATA",{});
 const auto settings=juce::JSON::parse(settingsFile());const auto saved=settings["dataFolder"].toString();
 const auto fallback=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Silver-K");
 dataFolder=saved.isNotEmpty()?juce::File(saved):test.isNotEmpty()?juce::File(test).getChildFile("Silver-K"):fallback;
 const auto assetRoot=settings["assetRoot"].toString();const bool sameAssetRoot=assetRoot.isEmpty() || assetRoot==dataFolder.getFullPathName();
 auto selectedAsset=[&](const char* role){
  const auto path=settings[role].toString();if(path.isEmpty())return juce::File{};const juce::File file(path);if(file.existsAsFile())return file;
  const auto relative=settings[juce::Identifier(juce::String(role)+"Relative")].toString();
  if(sameAssetRoot && relative.isNotEmpty() && !juce::File::isAbsolutePath(relative)){const auto relocated=dataFolder.getChildFile(relative);if(relocated.isAChildOf(dataFolder) && relocated.existsAsFile())return relocated;}
  return file;
 };
 osFile=selectedAsset("os");bankFile=selectedAsset("bank");flashFile=selectedAsset("flash");
 const double scale=double(settings.getProperty("editorScale",.75));editorScale.store(std::isfinite(scale)&&scale>=.5&&scale<=1.5?scale:.75);
 skinChoice=settings.getProperty("skinChoice","Auto").toString();customSkin=settings["customSkin"].toString();browserState=settings["browserState"].toString();
 const int controller=int(settings.getProperty("ksControllerMode",ksControllerMode()));ksController.store(controller>=0 && controller<=2?ks::ControllerMode(controller):ks::ControllerMode::H8);
 buffering.store(juce::jlimit(0,3,int(settings.getProperty("bufferProfile",1))));controlChannel.store(uint8_t(int(settings["editorChannel"])&15));editorChannelPreference.store(controlChannel.load());
 juce::String error;setDataFolder(dataFolder,error);if(error.isNotEmpty())message=error;
 preferenceBaseline=preferenceValues();startupFirmware=osFile;startupPending.store(osFile!=juce::File{});
}
bool Processor::setDataFolder(const juce::File& folder,juce::String& error){
 if(folder==juce::File{}){error="Choose a Silver-K folder";return false;}
 const auto roms=folder.getChildFile("ROMs"),patches=folder.getChildFile("Patches");
 for(const auto& dir:{roms,patches})if(auto result=dir.createDirectory();result.failed()){error=result.getErrorMessage();return false;}
 const auto zero=roms.getChildFile("ks-wave-zero.bin"),init=roms.getChildFile("ks-init-flash.bin");
 if(!zero.exists()){juce::MemoryBlock bytes(ks::Board::WaveSize,true);if(!zero.replaceWithData(bytes.getData(),bytes.getSize())){error="Cannot create placeholder Wave ROM";return false;}}
 if(!init.exists() && !init.replaceWithData(BinaryData::ksinitflash_bin,BinaryData::ksinitflash_binSize)){error="Cannot create synthetic KS Init Flash";return false;}
 dataFolder=folder;
 if(bankFile==juce::File{})bankFile=zero;if(flashFile==juce::File{})flashFile=init;
 const auto settings=juce::JSON::parse(settingsFile());
 const bool cached=settings["dataFolder"].toString()==folder.getFullPathName() && int(settings["firmwareIndexVersion"])==1 && settings["firmwares"].isArray();
 romFolder=roms;
 if(cached){
  firmwares.clear();for(const auto& item:*settings["firmwares"].getArray()){
   const auto relative=item["file"].toString();if(relative.isEmpty() || juce::File::isAbsolutePath(relative))continue;
   const auto file=roms.getChildFile(relative);if(file.isAChildOf(roms) && file.existsAsFile() && firmwares.size()<1024)firmwares.push_back({file,bool(item["kStation"])});
  }
 }else{juce::String scan;setRomFolder(roms,scan);}
 patchLibrary.addPath(patches,true);
 return rememberDataFolder(error) && rememberAssets(error);
}
bool Processor::rememberDataFolder(juce::String& error){
 juce::NamedValueSet values;values.set("dataFolder",dataFolder.getFullPathName());
 juce::Array<juce::var> catalog;for(const auto& choice:firmwares)if(choice.file.isAChildOf(dataFolder.getChildFile("ROMs"))){
  juce::DynamicObject::Ptr entry=new juce::DynamicObject;entry->setProperty("file",choice.file.getRelativePathFrom(dataFolder.getChildFile("ROMs")));entry->setProperty("kStation",choice.kStation);catalog.add(juce::var(entry.get()));
 }
 values.set("firmwareIndexVersion",1);values.set("firmwares",catalog);return updateSettings(values,error);
}
bool Processor::rememberAssets(juce::String& error){
 juce::NamedValueSet values;values.set("assetRoot",dataFolder.getFullPathName());
 for(const auto& asset:{std::pair<const char*,juce::File>{"os",osFile},{"bank",bankFile},{"flash",flashFile}}){
  values.set(asset.first,asset.second.getFullPathName());values.set(juce::String(asset.first)+"Relative",asset.second.isAChildOf(dataFolder)?asset.second.getRelativePathFrom(dataFolder):juce::String{});
 }
 return updateSettings(values,error);
}
bool Processor::setWaveRom(const juce::File& file,juce::String& error){
 if(file.getSize()!=ks::Board::WaveSize){error="Wave ROM must contain exactly 129024 bytes";return false;}
 bankFile=file;return rememberAssets(error);
}
bool Processor::setFlashRom(const juce::File& file,juce::String& error){
 if(file.hasFileExtension("mid;midi")){
  auto input=file.createInputStream();juce::MidiFile midi;if(!input || !midi.readFrom(*input)){error="Cannot read KS factory MIDI";return false;}
  bool found=false;for(int t=0;t<midi.getNumTracks();++t)for(int n=0;n<midi.getTrack(t)->getNumEvents();++n){const auto& m=midi.getTrack(t)->getEventPointer(n)->message;const auto* bytes=m.getRawData();const uint8_t prefix[]{0xf0,0,0x20,0x29,1,0x42};if(m.getRawDataSize()==270 && std::equal(std::begin(prefix),std::end(prefix),bytes) && bytes[7]==1 && bytes[269]==0xf7)found=true;}
  if(!found){error="No KS factory programs in MIDI file";return false;}
 }else if(!file.hasFileExtension("bin") || file.getSize()!=0x80000){error="KS Flash ROM must be a 524288-byte BIN or KS factory MIDI";return false;}
 flashFile=file;return rememberAssets(error);
}
juce::File Processor::resolveAsset(const juce::ValueTree& state,const char* role)const{
 const auto relative=state.getProperty(juce::String(role)+"Relative").toString();
 if(relative.isNotEmpty() && !juce::File::isAbsolutePath(relative)){const auto candidate=dataFolder.getChildFile(relative);if(candidate.isAChildOf(dataFolder) && candidate.existsAsFile())return candidate;}
 const auto path=state.getProperty(role).toString();if(path.isEmpty())return {};
 const auto old=juce::File(path);
 if(old.getFileName().isNotEmpty()){const auto relocated=dataFolder.getChildFile("ROMs").getChildFile(old.getFileName());if(relocated.existsAsFile())return relocated;}
 return old;
}
}
