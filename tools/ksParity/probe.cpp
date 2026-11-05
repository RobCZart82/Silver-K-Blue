#include "ksLib/ksBoard.h"
#include "ksLib/ksRom.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <cmath>
#include <filesystem>
// Developer-only private-fixture probe; outputs are not release artifacts.
void check(bool x,const char*m){if(!x)throw std::runtime_error(m);}
std::vector<uint8_t> read(const char*p){std::ifstream f(p,std::ios::binary);check(bool(f),"fixture missing");return {std::istreambuf_iterator<char>(f),{}};}
void save(const std::string&p,const std::vector<uint8_t>&v){std::ofstream f(p,std::ios::binary);f.write((const char*)v.data(),v.size());check(bool(f),"write failed");}
int main(int argc,char**argv){try{check(argc==5 || argc==6,"OS FLASH WAVE PREFIX [single|fourpart]");const std::string scenario=argc==6?argv[5]:"single";check(scenario=="single" || scenario=="fourpart","unknown scenario");ks::Rom rom(argv[1]);check(rom.isValid(),"invalid OS");auto flash=read(argv[2]),wave=read(argv[3]);
check(flash.size()==0x80000 && wave.size()==ks::Board::WaveSize,"invalid fixture sizes");
for(const auto* suffix:{".pcm",".state",".flash",".midi"})check(!std::filesystem::exists(std::string(argv[4])+suffix),"output exists; use a new private prefix");auto board=std::make_unique<ks::Board>(rom.data(),wave,flash,true,false);check(board->boot(),"boot failed");
#ifdef KS_SEMANTIC_PROBE
check(board->nativeOwnerState() && board->h8InterpretedSteps()==0,"not semantic native");
#endif
std::ofstream pcm(std::string(argv[4])+".pcm",std::ios::binary);int64_t peak=0;uint64_t frames=0;
auto send=[&](std::initializer_list<uint8_t> x){std::vector<uint8_t>v(x);check(board->enqueueMidi(v.data(),v.size()),"midi overflow");};
auto render=[&](unsigned n){for(unsigned i=0;i<n;++i){auto a=board->renderSample();for(auto v:a){peak=std::max(peak,std::abs(int64_t(v)));pcm.write((const char*)&v,sizeof(v));}++frames;}};
if(scenario=="fourpart"){
 auto state=board->state();state.performance=true;state.activeMask=15;
 for(unsigned part=0;part<4;++part){state.parts[part]=state.parts[0];state.parts[part][179]=100;state.parts[part][181]=uint8_t(part);}
 state.parts[0][189]=1;state.parts[0][190]=15;
 send({0xb0,32,65,0xc0,0});
 auto message=ks::stateMessage(state);check(board->enqueueMidi(message.data(),message.size()),"state overflow");
 render(16384);
 for(uint8_t part=0;part<4;++part)send({uint8_t(0x90|part),uint8_t(48+7*part),100});
 render(16384);
 for(uint8_t part=0;part<4;++part)send({uint8_t(0xb0|part),120,0});
 render(8192);
 auto current=board->state();check(current.performance && current.activeMask==15,"four-part state not retained");
}else{
render(8192);send({0xb0,7,127,91,0,92,0,93,0,0x90,60,100});render(16384);send({0xb0,64,127,0x80,60,0,0x90,67,96});render(8192);send({0xb0,64,0,120,0});render(8192);}
check(peak>1000 && bool(pcm),"silent or invalid audio");auto state=board->state();save(std::string(argv[4])+".state",ks::stateMessage(state));save(std::string(argv[4])+".flash",board->saveUserFlash());std::vector<uint8_t>midi;uint8_t b;while(board->popMidiOut(b))midi.push_back(b);save(std::string(argv[4])+".midi",midi);
#ifdef KS_SEMANTIC_PROBE
check(board->h8InterpretedSteps()==0,"native interpreted H8");
#endif
std::cout<<"PASS scenario="<<scenario<<" frames="<<frames<<" peak="<<peak<<" backend="<<board->controllerBackend()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
