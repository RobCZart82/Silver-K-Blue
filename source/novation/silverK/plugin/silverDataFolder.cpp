#include "silverProcessor.h"
#include "BinaryData.h"
namespace silverK {
juce::File Processor::settingsFile()const{
 const auto test=juce::SystemStats::getEnvironmentVariable("SILVER_K_DATA",{});
 return (test.isEmpty()?juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("The Usual Suspects/Silver-K"):juce::File(test)).getChildFile("settings.json");
}
void Processor::initializeDataFolder(){
 const auto test=juce::SystemStats::getEnvironmentVariable("SILVER_K_DATA",{});
 const auto settings=juce::JSON::parse(settingsFile());const auto saved=settings["dataFolder"].toString();
 const auto fallback=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Silver-K");
 juce::String error;setDataFolder(saved.isNotEmpty()?juce::File(saved):test.isNotEmpty()?juce::File(test).getChildFile("Silver-K"):fallback,error);
 if(error.isNotEmpty())message=error;
}
bool Processor::setDataFolder(const juce::File& folder,juce::String& error){
 if(folder==juce::File{}){error="Choose a Silver-K folder";return false;}
 const auto roms=folder.getChildFile("ROMs"),patches=folder.getChildFile("Patches");
 for(const auto& dir:{roms,patches})if(auto result=dir.createDirectory();result.failed()){error=result.getErrorMessage();return false;}
 const auto zero=roms.getChildFile("ks-wave-zero.bin"),init=roms.getChildFile("ks-init-flash.bin");
 if(!zero.exists()){juce::MemoryBlock bytes(ks::Board::WaveSize,true);if(!zero.replaceWithData(bytes.getData(),bytes.getSize())){error="Cannot create placeholder Wave ROM";return false;}}
 if(!init.exists() && !init.replaceWithData(BinaryData::ksinitflash_bin,BinaryData::ksinitflash_binSize)){error="Cannot create synthetic KS Init Flash";return false;}
 dataFolder=folder;
 if(!bankFile.existsAsFile())bankFile=zero;if(!flashFile.existsAsFile())flashFile=init;
 const auto settings=juce::JSON::parse(settingsFile());
 const bool cached=settings["dataFolder"].toString()==folder.getFullPathName() && int(settings["firmwareIndexVersion"])==1 && settings["firmwares"].isArray();
 romFolder=roms;
 if(cached){
  firmwares.clear();for(const auto& item:*settings["firmwares"].getArray()){
   const auto relative=item["file"].toString();if(relative.isEmpty() || juce::File::isAbsolutePath(relative))continue;
   const auto file=roms.getChildFile(relative);if(file.isAChildOf(roms) && firmwares.size()<1024)firmwares.push_back({file,bool(item["kStation"])});
  }
 }else{juce::String scan;setRomFolder(roms,scan);}
 patchLibrary.addPath(patches);
 return rememberDataFolder(error);
}
bool Processor::rememberDataFolder(juce::String& error){
 const auto settings=settingsFile();if(auto result=settings.getParentDirectory().createDirectory();result.failed()){error=result.getErrorMessage();return false;}
 juce::DynamicObject::Ptr data=new juce::DynamicObject;data->setProperty("dataFolder",dataFolder.getFullPathName());
 juce::Array<juce::var> catalog;for(const auto& choice:firmwares)if(choice.file.isAChildOf(dataFolder.getChildFile("ROMs"))){
  juce::DynamicObject::Ptr entry=new juce::DynamicObject;entry->setProperty("file",choice.file.getRelativePathFrom(dataFolder.getChildFile("ROMs")));entry->setProperty("kStation",choice.kStation);catalog.add(juce::var(entry.get()));
 }
 data->setProperty("firmwareIndexVersion",1);data->setProperty("firmwares",catalog);
 if(!settings.replaceWithText(juce::JSON::toString(juce::var(data.get())))){error="Cannot remember Silver-K folder";return false;}
 return true;
}
juce::File Processor::resolveAsset(const juce::ValueTree& state,const char* role)const{
 const auto relative=state.getProperty(juce::String(role)+"Relative").toString();
 if(relative.isNotEmpty() && !juce::File::isAbsolutePath(relative)){const auto candidate=dataFolder.getChildFile(relative);if(candidate.isAChildOf(dataFolder) && candidate.existsAsFile())return candidate;}
 const auto old=juce::File(state.getProperty(role).toString());
 if(old.getFileName().isNotEmpty()){const auto relocated=dataFolder.getChildFile("ROMs").getChildFile(old.getFileName());if(relocated.existsAsFile())return relocated;}
 return old;
}
}
