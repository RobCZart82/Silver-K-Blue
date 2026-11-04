#include "kstationLib/kstationDevice.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <array>
#include <cmath>
#include <filesystem>
#include <iterator>
// Optional private-fixture tool; state files contain user preset data.
// PRIVATE_STATE_NATIVE is only used when compiling against the upstream candidate.
void require(bool x,const char*m){if(!x)throw std::runtime_error(m);}
int main(int argc,char**argv){try{
 require(argc==4,"rom input-state-or-dash output-state");
 require(!std::filesystem::exists(argv[3]),"Output already exists; use a new private path");
 synthLib::DeviceCreateParams params;params.romName=argv[1];std::ifstream rom(argv[1],std::ios::binary);require(bool(rom),"Cannot open firmware");params.romData.assign(std::istreambuf_iterator<char>(rom),{});
 
#ifdef PRIVATE_STATE_NATIVE
 kstation::Device device(params,nullptr,kstation::ControllerMode::Native);
#else
 kstation::Device device(params);
#endif
require(device.isValid(),device.error().c_str());device.setNonRealtime(true);require(device.prepareAudio(256,48000),"prepare");
 std::array<float,256> left{},right{};synthLib::TAudioInputs in{};synthLib::TAudioOutputs out{};out[0]=left.data();out[1]=right.data();std::vector<synthLib::SMidiEvent> events,responses;
 auto render=[&](unsigned blocks){for(unsigned i=0;i<blocks;++i){device.process(in,out,256,events,responses);events.clear();}};
 render(400);
 if(std::string(argv[2])!="-"){std::ifstream f(argv[2],std::ios::binary);require(bool(f),"Cannot open state");std::vector<uint8_t> state((std::istreambuf_iterator<char>(f)),{});require(device.setState(state,synthLib::StateTypeGlobal),"restore failed");render(400);}
 else{events.emplace_back(synthLib::MidiEventSource::Unknown,0xb0,74,93);events.emplace_back(synthLib::MidiEventSource::Unknown,0xb0,71,27);render(400);}
 std::vector<uint8_t> state;require(device.getState(state,synthLib::StateTypeGlobal),"save failed");std::ofstream f(argv[3],std::ios::binary);f.write(reinterpret_cast<const char*>(state.data()),state.size());require(bool(f),"write failed");
 std::array<uint8_t,128> patch{};std::array<uint8_t,2048> cc{};uint64_t revision=0;require(device.getControlSnapshot(patch,cc,revision),"snapshot failed");
 require(patch[39]==93 && patch[6]==27,"Patch CC positive controls not retained");
 require(cc[74]==93 && cc[71]==27,"Controller positive controls not retained");
 events.emplace_back(synthLib::MidiEventSource::Unknown,0x90,60,100);double peak=0;for(unsigned n=0;n<100;++n){render(1);for(auto v:left){require(std::isfinite(v),"nonfinite");peak=std::max(peak,std::abs(double(v)));}for(auto v:right)require(std::isfinite(v),"nonfinite right");}require(peak>1e-5,"silent restore");std::cout<<"PASS backend="<<device.controllerBackend()<<" bytes="<<state.size()<<" peak="<<peak<<" patch=";for(auto v:patch)std::cout<<unsigned(v)<<',';std::cout<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
