#include "ksPlaceholderMidi.h"
#include <vector>
#include <iostream>
#include <stdexcept>
#include <string>

void require(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
int main() {
 try {
    ks::PlaceholderMidi p;
    std::array<uint8_t,256> patch{};
    patch.fill(64); patch[128]=patch[129]=patch[130]=2;
    patch[37]=101; patch[38]=92; patch[39]=83;
    p.seed(0,patch.data());
    std::vector<uint8_t> out;
    auto emit=[&](const uint8_t* b,size_t n){out.insert(out.end(),b,b+n);return true;};
    auto resolve=[&](uint8_t ch,uint8_t bank,uint8_t number){if(bank!=1 || number!=0)return uint16_t(0);p.seed(ch,patch.data());return uint16_t(1u<<ch);};
    auto feed=[&](std::initializer_list<uint8_t> b){out.clear();for(auto v:b)require(p.accept(v,emit,resolve,[](uint8_t*,size_t){}),"MIDI emission failed");};
    feed({0xb0,98,50,6,27});
    require(out==std::vector<uint8_t>({0xb0,98,50,6,27}),"Real bank must pass bytes through exactly");
    p.enabled=true;
    feed({0xb0,98,50,6,27});
    require(out.size()>6 && p.waves[0][0]==27,"ROM wave did not mute dependencies");
    require(out[out.size()-3]==0xb0 && out[out.size()-2]==6 && out.back()==27,"Injected NRPNs lost wave selection");
    feed({0xb0,72,117});
    require(out==std::vector<uint8_t>({0xb0,72,0}),"Level change unmuted ROM wave");
    feed({0xb0,98,63,6,127});
    require(out==std::vector<uint8_t>({0xb0,98,63,0xb0,6,64}),"Level modulation unmuted ROM wave");
    auto saved=patch; p.restoreDesired(0,saved.data());
    require(saved[37]==117 && saved[141]==127,"Saved program lost desired controls");
    feed({0xb0,98,50,6,2});
    require(p.waves[0][0]==2,"Generated wave was not restored");
    bool level=false,mod=false;
    for(size_t i=0;i+2<out.size();i+=3){if(out[i+1]==72 && out[i+2]==117)level=true;if(out[i+1]==6 && out[i+2]==127)mod=true;}
    require(level && mod,"Generated wave lost original level/modulation");
    feed({0xb1,98,51,6,31});
    require(p.waves[0][1]==2 && p.waves[1][1]==31,"Channel isolation failed");
    feed({0x90,69,0xf8,100,70,100});
    require(out==std::vector<uint8_t>({0xf8,0x90,69,100,0x90,70,100}),"Realtime/running status corrupts notes");
    feed({0xf2,1,2,0xf6,3});
    require(out==std::vector<uint8_t>({0xf2,1,2,0xf6,3}),"System common did not clear running status");
    patch[128]=27; p.globalChannel=3;
    std::vector<uint8_t> sysex{0xf0,0,0x20,0x29,1,0x42,0x7f,0,0,0x11,2,0,0};
    sysex.insert(sysex.end(),patch.begin(),patch.end());sysex.push_back(0xf7);
    out.clear();for(auto v:sysex)require(p.accept(v,emit,resolve,[](uint8_t*,size_t){}),"SysEx failed");
    require(out.size()>sysex.size(),"Program SysEx was not muted");
    const auto offset=out.size()-sysex.size();require(std::equal(sysex.begin(),sysex.begin()+13,out.begin()+offset) && out[offset+13+37]==0 && out[offset+13+128]==27,"Current preset did not carry muted fields");
    require(out[0]==0xb3 && p.waves[3][0]==27,"SysEx used wrong global channel");
    feed({0xb3,70,1,72,127});
    require(p.waves[3][0]==27 && out.back()==0,"KS glide CC unmuted a ROM waveform");
    feed({0xb0,32,1,0xc0,0,0x90,69,100});
    require(out.size()>8 && p.waves[0][0]==27,"Program Change did not mute destination preset");
    require(out[out.size()-3]==0x90,"Mute did not precede Note On");
    sysex[7]=1;sysex[8]=1;sysex[11]=5;out.clear();
    for(auto v:sysex)require(p.accept(v,emit,resolve,[](uint8_t* msg,size_t n){if(n==270)ks::PlaceholderMidi::muteProgram(msg+13);}),"Stored drum dump failed");
    require(out.size()==270 && out[13+37]==0 && out[13+128]==27,"Stored drum dump was not muted before transmission");
    std::vector<uint8_t> performance{0xf0,0,0x20,0x29,1,0x42,0x7f,2,0,0x11,2,0,0};
    for(unsigned part=0;part<4;++part){auto program=patch;program[181]=uint8_t(part);program[37]=uint8_t(90+part);performance.insert(performance.end(),program.begin(),program.end());}performance.push_back(0xf7);
    out.clear();for(auto byte:performance)require(p.accept(byte,emit,resolve,[](uint8_t*,size_t){}),"Performance SysEx failed");
    require(out.size()>1038,"Performance was not adapted");const auto start=out.size()-1038;
    for(unsigned part=0;part<4;++part){const auto offset=start+13+part*256;require(out[offset+37]==0 && out[offset+128]==27,"Performance part was not muted");auto program=patch;program[37]=0;p.restorePart(part,program.data());require(program[37]==90+part,"Per-part desired level was lost");}
    feed({0xb2,72,117});auto desired=patch;desired[37]=0;p.restorePart(2,desired.data());require(out.back()==0 && desired[37]==117,"Part edit lost desired level or ROM mute");
    std::vector<uint8_t> large(2048,0);large.front()=0xf0;large.back()=0xf7;out.clear();
    for(auto v:large)require(p.accept(v,emit,resolve,[](uint8_t*,size_t){}),"Large SysEx failed");
    require(out==large,"Large unrelated SysEx was corrupted");
    p.enabled=false;
    feed({0xb0,98,50,6,27,0xf8,72,127});
    require(out==std::vector<uint8_t>({0xb0,98,50,6,27,0xf8,72,127}),"Disabled adaptation changed real-bank MIDI");
    std::cout<<"PASS: zero-bank muting, modulation, restoration, channels, SysEx, Program Change, realtime, running status, real-bank passthrough\n";
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
