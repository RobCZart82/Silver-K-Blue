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
  check(strip.isValid(),"Embedded strip missing");
  Processor p;check(p.setDataFolder(root.getChildFile(name.contains("Retina")?"retina":"standard"),error),error);p.skinChoice=name;p.customSkin.clear();
  const auto before=p.parameters.copyState();
  {Editor editor(p);auto* combo=dynamic_cast<juce::ComboBox*>(find(editor,"skinSelect"));check(combo&&combo->getText()==name,"Embedded selection not shown");
   for(const auto* page:{"pageMain","pagePerf","pagePatches","pageSetup"}){auto* button=dynamic_cast<juce::TextButton*>(find(editor,page));check(button&&bool(button->onClick),"Missing navigation");button->onClick();check(editor.createComponentSnapshot(editor.getLocalBounds(),true,1.f).isValid(),"Page rendering failed");}
   check(editor.reloadSkin(error),error);check(p.customSkin.isEmpty(),"Reload wrote an external skin path");
  }
  check(p.parameters.copyState().isEquivalentTo(before),"Skin rendering changed sound parameters");
  juce::MemoryBlock state;p.getStateInformation(state);Processor restored;restored.setStateInformation(state.getData(),int(state.getSize()));check(restored.skinChoice==name,"Embedded skin choice did not restore");
  std::cout<<"PASS: "<<name<<" loads/reloads without skin files; four pages; host-state restore; parameters preserved\n";
 }
 SkinPackage missing;check(!SkinPackage::loadEmbedded("missing",missing,error),"Unknown embedded choice accepted");
 check(SkinPackage::builtin().id=="builtin","Legacy builtin changed");
 std::cout<<"PASS: embedded skins and classic fallback\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}}
