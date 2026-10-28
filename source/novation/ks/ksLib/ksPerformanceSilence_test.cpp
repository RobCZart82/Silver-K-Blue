#include "ksBoard.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
std::vector<uint8_t> bytes(const char* path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
int main(int argc,char** argv){if(argc!=4)return 2;try{
 auto flash=bytes(argv[3]);require(flash.size()==0x80000,"Test Flash size");ks::Program init;std::copy_n(flash.data()+0x2c800,256,init.begin());
 std::fill(flash.begin()+0x20000,flash.begin()+0x60000,0xff);std::copy(init.begin(),init.end(),flash.begin()+0x2c800);
 ks::State state;state.performance=true;state.activeMask=15;
 for(unsigned part=0;part<4;++part){auto& p=state.parts[part];p=init;for(unsigned n:{38,39,40,41,62,65,100,107,110})p[n]=0;p[37]=127;p[128]=part&1?31:27;p[179]=127;p[180]=uint8_t(2+part);p[181]=uint8_t(part);p[182]=0;p[183]=127;p[184]=p[185]=64;std::copy(p.begin(),p.end(),flash.begin()+0x45800+part*256);}flash[0x45800+189]=1;flash[0x45800+190]=15;
 flash[0x1ff01]=0;flash[0x1ff04]=1;
 ks::Board board(bytes(argv[1]),bytes(argv[2]),flash);require(board.boot(),"Original reset failed");require(board.zeroBank(),"Expected zero bank");
 const auto startup=board.state();require(startup.performance,"Startup Performance mode lost");
 for(unsigned part=0;part<4;++part)require(startup.parts[part][37]==127,"Startup Performance lost desired level");
 require(board.midiReceived<2048,"Boot rewrote stored Performances over MIDI");
 auto send=[&](std::initializer_list<uint8_t> data){std::vector<uint8_t> v(data);require(board.enqueueMidi(v.data(),v.size()),"MIDI overflow");};
 auto settle=[&](unsigned n=20000){for(unsigned i=0;i<n;++i)board.renderSample();};
 send({0xb0,32,65,0xc0,0});settle();require(board.read(0x167ba)==1,"Stored Performance did not select native mode");
 for(unsigned part=0;part<4;++part){const auto ch=uint8_t(0xb0|part);send({uint8_t(0x90|part),69,100,ch,72,127,ch,70,1,ch,98,63,ch,6,127});}settle();
 for(unsigned n=0;n<8192;++n){const auto frame=board.renderSample();for(unsigned c=0;c<4;++c)require(frame[c]==0,"ROM Performance produced DAC audio");for(unsigned c=16;c<20;++c)require(frame[c]==0,"ROM Performance produced voice audio");}
 auto snapshot=board.state();for(unsigned part=0;part<4;++part)require(snapshot.parts[part][37]==127 && snapshot.parts[part][141]==127,"Desired part values lost");
 auto saved=board.saveUserFlash();for(unsigned part=0;part<4;++part)require(saved[0x45800-0x1fc00+part*256+37]==127,"Saved Performance lost desired levels");
 const auto dump=ks::stateMessage(snapshot);require(board.enqueueMidi(dump.data(),dump.size()),"Current Performance overflow");settle();for(unsigned part=0;part<4;++part)send({uint8_t(0x90|part),72,100});settle();for(unsigned n=0;n<8192;++n){auto frame=board.renderSample();for(unsigned c=0;c<4;++c)require(frame[c]==0,"Current Performance unmuted ROM");}
 std::cout<<"PASS: stored/current Performance, 4 ROM parts, all 4 DAC outputs and 4 voice stems exact zero, mixer/glide/modulation, desired current values and saved Flash\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}return 0;}
