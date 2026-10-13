#pragma once
#include "../../ks/ksJucePlugin/ksControls.h"
#include "../../kstation/kstationJucePlugin/kstationControls.h"
#include <juce_core/juce_core.h>
namespace silverK {
enum class Model {Unknown,KStation,KS,AStation,XStation};
juce::String semanticId(const juce::String&);
Model modelOf(const uint8_t*,size_t);
template<class Alloc> Model modelOf(const std::vector<uint8_t,Alloc>& p){return modelOf(p.data(),p.size());}
juce::String modelName(Model);
bool validPatch(const uint8_t*,size_t);
template<class Alloc> bool validPatch(const std::vector<uint8_t,Alloc>& p){return validPatch(p.data(),p.size());}
struct Conversion {std::vector<uint8_t> data;juce::StringArray changes;};
Conversion convertPatch(const uint8_t*,size_t,Model target,unsigned part=0);
template<class Alloc> Conversion convertPatch(const std::vector<uint8_t,Alloc>& p,Model target,unsigned part=0){return convertPatch(p.data(),p.size(),target,part);}
std::vector<ksPlugin::Control> kControls();
}
