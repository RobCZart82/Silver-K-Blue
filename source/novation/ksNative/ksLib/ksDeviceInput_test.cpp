#include "ksDevice.h"
#include "ksRom.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
static std::vector<uint8_t> bytes(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){try{
 if(argc!=4){std::cout<<"SKIP: supply private KS firmware, wave bank and INIT flash\n";return 77;}
 ks::Rom os(argv[1]);auto wave=bytes(argv[2]),flash=bytes(argv[3]);
 if(!os.isValid() || wave.size()!=ks::Board::WaveSize || flash.size()!=0x80000)throw std::runtime_error("Invalid fixture");
 ks::Device d(os.data(),wave,flash,48000,256);
 auto cc=[&](uint8_t number,uint8_t value){const uint8_t m[]{0xb0,number,value};if(!d.midi(m,3))throw std::runtime_error("MIDI rejected");};
 for(auto n:{72,73,74,75,76,91,92,93,95})cc(uint8_t(n),0);
 cc(77,127);cc(7,127);cc(99,0);cc(98,23);cc(6,64);
 // Measure the settled half of each two-second segment, excluding queue/filter latency.
 auto render=[&](int mode){float peak=0;std::array<float,256> left{},right{},input{},inputRight{};
  for(int block=0;block<375;++block){for(int i=0;i<256;++i){
   const auto sample=float(.1*std::sin(6.283185307179586*440*(block*256+i)/48000));
   input[i]=mode==0?0:sample;inputRight[i]=mode==3?-sample:sample;
   left[i]=input[i];right[i]=inputRight[i];
  }
   const bool stereo=mode>=2;
   // Mode 4 deliberately aliases both input channels with the output storage.
   d.process(left.data(),right.data(),256,true,mode==4?left.data():input.data(),stereo?(mode==4?right.data():inputRight.data()):nullptr);
   for(int i=0;i<256;++i){if(!std::isfinite(left[i]) || !std::isfinite(right[i]))throw std::runtime_error("Nonfinite output");if(block>187)peak=std::max(peak,std::max(std::abs(left[i]),std::abs(right[i])));}
  }return peak;};
 const auto quiet=render(0),tone=render(1),stereo=render(2),cancel=render(3),alias=render(4),tail=render(0);
 std::cout<<"quiet="<<quiet<<" mono="<<tone<<" stereo="<<stereo<<" cancel="<<cancel<<" alias="<<alias<<" tail="<<tail<<" overflow="<<d.inputOverflows()<<std::endl;
 if(tone<.0001f || quiet>tone*.001f || tail>tone*.001f || cancel>tone*.001f || std::abs(stereo-tone)>tone*.01f || std::abs(alias-tone)>tone*.01f || d.inputOverflows())throw std::runtime_error("External input positive control failed");
 std::cout<<"PASS: Device mono/stereo external audio, phase cancellation, aliased buffers and silence\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
