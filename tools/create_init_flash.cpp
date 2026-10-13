// Silver-K's own init data. No firmware, wave ROM, factory MIDI or dump inputs.
// The public control catalog supplies field layout and parameter defaults.
#include "../source/novation/ks/ksJucePlugin/ksControls.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Usage: create_init_flash OUTPUT.bin\n";return 2;}
    std::array<uint8_t,256> init{};
    for(const auto& c:ksPlugin::controls) if(c.patchOffset>=0)
        init[c.patchOffset]=uint8_t((init[c.patchOffset]&~c.patchMask)|((c.defaultValue<<c.patchShift)&c.patchMask));
    // Our simple saw voice: oscillator 1 only, no modulation/effects/vocoder.
    init[128]=2;init[129]=init[130]=0;
    init[37]=100;init[38]=init[39]=init[40]=init[41]=init[42]=0;
    init[62]=2;init[63]=80;init[64]=100;init[65]=30;
    init[100]=init[107]=init[110]=0;
    // KS program boost is 0..30 dB; zero is the normal unboosted level.
    init[125]=0;
    init[179]=100;init[180]=0;init[181]=0;
    init[182]=0;init[183]=127;init[184]=init[185]=64;init[186]=0;
    auto name=[](auto& patch,size_t offset,const std::string& text){
        std::fill_n(patch.begin()+offset,16,uint8_t(' '));
        std::copy_n(text.begin(),std::min<size_t>(text.size(),16),patch.begin()+offset);
    };
    std::vector<uint8_t> flash(0x80000,0xff);
    for(unsigned n=0;n<600;++n){auto patch=init;name(patch,151,"Silver Init");std::copy(patch.begin(),patch.end(),flash.begin()+0x20000+n*256);}
    for(unsigned n=0;n<100;++n)for(unsigned part=0;part<4;++part){
        auto patch=init;patch[181]=uint8_t(part);name(patch,151,"Silver Init");
        if(part==0){patch[189]=1;patch[190]=15;name(patch,192,"Silver Multi");}
        std::copy(patch.begin(),patch.end(),flash.begin()+0x45800+n*1024+part*256);
    }
    // Startup in single mode, bank 1 / program 0, global MIDI channel 1.
    constexpr uint8_t globals[]{1,0,1,0,0,1,15,127,0,0,64,0,0,64,64,0,3,0,0,0,0};
    std::fill(flash.begin()+0x1fc00,flash.begin()+0x20000,0);
    std::copy(std::begin(globals),std::end(globals),flash.begin()+0x1ff00);
    std::ofstream out(argv[1],std::ios::binary);out.write(reinterpret_cast<const char*>(flash.data()),flash.size());
    if(!out)return 1;
    std::cout<<"Wrote "<<flash.size()<<" bytes of synthetic init flash\n";
}
