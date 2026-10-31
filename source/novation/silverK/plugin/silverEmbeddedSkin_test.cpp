#include "silverSkin.h"
#include "silverEditor.h"
#include <iostream>
#include <stdexcept>
using namespace silverK;
static void check(bool value,const juce::String& text){if(!value)throw std::runtime_error(text.toStdString());}
static juce::Component* find(juce::Component& c,const juce::String& id){if(c.getComponentID()==id)return &c;for(auto* child:c.getChildren())if(auto* found=find(*child,id))return found;return nullptr;}
int main(){try{
 juce::ScopedJuceInitialiser_GUI gui;juce::String error;
 const auto root=juce::File(juce::SystemStats::getEnvironmentVariable("SILVER_K_DATA",{}));check(root!=juce::File{},"Isolated test data required");
 for(const auto& name:SkinPackage::choices()){
  if(!name.startsWith("Silver-K Blue"))continue;
  SkinPackage embedded;check(SkinPackage::loadEmbedded(name,embedded,error),error);check(embedded.name==name,"Name mismatch");check(embedded.manifest==juce::File{},"Embedded skin depends on an external manifest");
  const auto& strip=embedded.images.at("cyanV2_unipolar");
  check(strip.isValid(),"Embedded strip missing");check(strip.getWidth()==(name.contains("Retina")?256:64)&&strip.getHeight()==128*strip.getWidth(),"Embedded filmstrip geometry mismatch");
  Processor p;check(p.setDataFolder(root.getChildFile(name.contains("Retina")?"retina":"standard"),error),error);p.skinChoice=name;p.customSkin.clear();
  const auto before=p.parameters.copyState();
  {Editor editor(p);auto* combo=dynamic_cast<juce::ComboBox*>(find(editor,"skinSelect"));check(combo&&combo->getText()==name,"Embedded selection not shown");
   for(const auto* page:{"pageMain","pagePerf","pagePatches","pageSetup"}){auto* button=dynamic_cast<juce::TextButton*>(find(editor,page));check(button&&bool(button->onClick),"Missing navigation");button->onClick();check(editor.createComponentSnapshot(editor.getLocalBounds(),true,1.f).isValid(),"Page rendering failed");
    const auto previewRoot=juce::SystemStats::getEnvironmentVariable("SILVER_K_PREVIEWS",{});
    if(previewRoot.isNotEmpty() && (juce::String(page)=="pageMain" || juce::String(page)=="pageSetup")){
     auto folder=juce::File(previewRoot);folder.createDirectory();auto file=folder.getChildFile((name.contains("Retina")?juce::String("Retina"):juce::String("Standard"))+"-embedded-"+(juce::String(page)=="pageMain"?"Main":"Setup")+"-2x.png");file.deleteFile();auto stream=file.createOutputStream();check(stream&&juce::PNGImageFormat().writeImageToStream(editor.createComponentSnapshot(editor.getLocalBounds(),true,2.f),*stream),"Preview export failed");
    }}
   auto* setup=dynamic_cast<juce::TextButton*>(find(editor,"pageSetup"));setup->onClick();
   auto* advanced=dynamic_cast<juce::TextButton*>(find(editor,"setupAdvancedKS"));
   check(advanced&&advanced->isVisible(),"Missing advanced KS disclosure");
   for(const auto* id:{"ksController","ksTwoCoreProcessing","demoSong"})check(!find(editor,id)->isVisible(),"Advanced control visible while collapsed");
   check(!find(editor,"openSkins")->isVisible(),"Duplicate Skins folder action still visible");
   const auto advancedBefore=p.parameters.copyState();
   advanced->setToggleState(true,juce::dontSendNotification);advanced->onClick();
   for(const auto* id:{"ksController","ksTwoCoreProcessing","demoSong"})check(find(editor,id)->isVisible(),"Advanced control inaccessible when expanded");
   const juce::StringArray setupIds{"setupPathPatches","setupPathROMs","setupPathSkins","loadFirmware","waveRom","factoryData","rescanRoms","dataFolder","setupStatus","setupHelp","skinSelect","loadSkin","reloadSkin","exportSkin","controlChannel","audioBuffer","setupLatency","performanceDiagnostics","setupAdvancedKS","ksController","demoSong","ksTwoCoreProcessing","ksTwoCoreNotice"};
   for(int i=0;i<setupIds.size();++i){auto* a=find(editor,setupIds[i]);check(a!=nullptr,"Setup control missing");
    check(a->getParentComponent()->getLocalBounds().contains(a->getBounds()),"Setup control outside canvas");
    for(int j=i+1;j<setupIds.size();++j)check(!a->getBounds().intersects(find(editor,setupIds[j])->getBounds()),"Setup controls overlap: "+setupIds[i]+" / "+setupIds[j]);
   }
   auto* mainPage=dynamic_cast<juce::TextButton*>(find(editor,"pageMain"));mainPage->onClick();
   check(!find(editor,"ksController")->isVisible(),"Advanced controls leaked onto Main");setup->onClick();
   check(find(editor,"ksController")->isVisible(),"Disclosure state lost on page switch");
   advanced->setToggleState(false,juce::dontSendNotification);advanced->onClick();
   check(p.parameters.copyState().isEquivalentTo(advancedBefore),"Disclosure changed sound parameters");
   for(const auto* folder:{"Patches","ROMs","Skins"}){
    auto* path=dynamic_cast<juce::TextEditor*>(find(editor,"setupPath"+juce::String(folder)));
    check(path&&path->isReadOnly()&&path->isVisible(),"Missing read-only folder path");
    check(path->getText()==p.dataFolder.getChildFile(folder).getFullPathName(),"Incorrect folder path");
   }
   check(p.setDataFolder(root.getChildFile(name.contains("Retina")?"retina-moved":"standard-moved"),error),error);
   setup->onClick();
   auto* patches=dynamic_cast<juce::TextEditor*>(find(editor,"setupPathPatches"));
   check(patches->getText()==p.patchesFolder().getFullPathName(),"Folder path did not follow data-folder change");
   check(editor.reloadSkin(error),error);check(p.customSkin.isEmpty(),"Reload wrote an external skin path");
  }
  check(p.parameters.copyState().isEquivalentTo(before),"Skin rendering changed sound parameters");
  juce::MemoryBlock state;p.getStateInformation(state);Processor restored;restored.setStateInformation(state.getData(),int(state.getSize()));check(restored.skinChoice==name,"Embedded skin choice did not restore");
  std::cout<<"PASS: "<<name<<" loads/reloads without skin files; four pages; host-state restore; parameters preserved\n";
 }
 {
 Processor autoProcessor;autoProcessor.skinChoice="Auto";juce::MemoryBlock saved;autoProcessor.getStateInformation(saved);
 Processor current;current.setStateInformation(saved.getData(),int(saved.getSize()));check(current.skinChoice=="Auto","New Auto state did not restore");
 auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize()));check(bool(xml),"Missing state XML");xml->removeAttribute("blueAutoVersion");
 juce::MemoryBlock legacy;juce::AudioProcessor::copyXmlToBinary(*xml,legacy);Processor previous;previous.setStateInformation(legacy.getData(),int(legacy.getSize()));check(previous.skinChoice=="Silver-K Legacy","Old Auto did not migrate to Legacy");
 }
 SkinPackage autoSkin;check(SkinPackage::loadEmbedded("Auto",autoSkin,error),error);check(autoSkin.name=="Silver-K Blue Retina 4K v1.0","Auto must resolve to Blue Retina");
 check(!SkinPackage::isBlueChoice("Silver-K Legacy"),"Legacy resolved as Blue");
 SkinPackage missing;check(!SkinPackage::loadEmbedded("missing",missing,error),"Unknown embedded choice accepted");
 check(SkinPackage::builtin().id=="builtin","Legacy builtin changed");
 std::cout<<"PASS: embedded skins and classic fallback\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}}
