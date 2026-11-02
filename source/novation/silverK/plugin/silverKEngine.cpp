#include "silverKEngine.h"
#include "silverAudioInput.h"
#include <array>
namespace silverK {
namespace {synthLib::DeviceCreateParams params(const std::vector<uint8_t>& rom){synthLib::DeviceCreateParams p;p.romData=rom;return p;}}
KEngine::KEngine(const std::vector<uint8_t>& rom,double hostRate,int block,const std::vector<uint8_t>& state,unsigned bufferFloor,baseLib::performance::Diagnostics* performance):device(params(rom),performance),rate(hostRate){
 if(!device.isValid())throw std::runtime_error(device.error());
 if(!state.empty() && !device.setState(state,synthLib::StateTypeGlobal))throw std::runtime_error("Invalid K-Station state");
 if(!device.prepareAudio(std::max(block,64),float(rate)))throw std::runtime_error("Unsupported K-Station host buffer");
 device.setBufferFloor(bufferFloor);
 render=[this](const synthLib::TAudioInputs& in,const synthLib::TAudioOutputs& out,size_t n,const auto& midi,auto& response){device.process(in,out,n,midi,response);};
 // Existing Gearmulator input/output converters are prepared before publication.
 resampler.setSamplerates(float(rate),device.getSamplerate());
 device.setNonRealtime(true);std::array<float,256> left{},right{};process(left.data(),right.data(),256,true);device.setNonRealtime(false);
}
void KEngine::process(float* left,float* right,int count,bool offline,const float* input){
 device.setNonRealtime(offline);
 processMonoInputChunks(left,right,count,input,[&](float* l,float* r,const float* mono,int size){
  synthLib::TAudioOutputs out{};out[0]=l;out[1]=r;synthLib::TAudioInputs in{};in[0]=mono;
  resampler.process(in,out,emptyMidi,outputMidi,uint32_t(size),render);
 });
}
unsigned KEngine::latency()const{return unsigned(std::ceil((device.getInternalLatencyMidiToOutput()+resampler.getOutputLatency())*rate/device.getSamplerate()));}
}
