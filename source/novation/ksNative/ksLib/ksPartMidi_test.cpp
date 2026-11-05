#include "ksPartMidi.h"
#include <iostream>
#include <stdexcept>
using namespace ks;
static void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    PartMidi router;router.enabled=true;router.globalChannel=9;
    State state;state.performance=true;
    for(unsigned p=0;p<PartCount;++p)state.parts[p][181]=uint8_t(p<2?9:p+8);
    auto message=stateMessage(state);router.seed(message.data()+13);
    std::vector<uint8_t> out,stored;
    auto send=[&](const std::vector<uint8_t>& input){out.clear();for(auto byte:input)need(router.accept(byte,
        [&](const uint8_t* b,size_t n){out.insert(out.end(),b,b+n);return true;},
        [&](uint8_t){return message.data()+13;},
        [&](const uint8_t* b,size_t n){stored.assign(b,b+n);}),"Router rejected MIDI");};
    send({0x99,60,100,61,90});need(out==std::vector<uint8_t>({0x90,60,100,0x91,60,100,0x90,61,90,0x91,61,90}),"Layer/running status routing");
    send({0xb9,7,40});need(out==std::vector<uint8_t>({0xb0,7,40,0xb1,7,40}),"External CC should still affect both layered parts");
    send({0xb9,98,89});send({0xb9,6,72});need(out==std::vector<uint8_t>({0xb0,99,0,0xb0,98,89,0xb0,6,72,0xb1,99,0,0xb1,98,89,0xb1,6,72}),"Data Entry must restore the external NRPN selector");
    send({0xb9,119,127});need(out==std::vector<uint8_t>({0xb0,119,30,0xb1,119,30}),"Out-of-range MIDI boost");
    send({0x8a,60,0});need(out==std::vector<uint8_t>({0x82,60,0}),"Distinct-channel Note Off");
    send({0x95,60,100});need(out.empty(),"Unassigned channel reached a private part");
    send({0xb9,98,86,0xb9,6,15});need(router.channels[0]==15 && router.channels[1]==15 && out.size()==6,"Channel assignment must not change private channels");
    send({0x9f,60,100});need(out.size()==6 && out[0]==0x90 && out[3]==0x91,"Reassigned layer routing");
    send(message);need(out.size()==1038 && router.channels[0]==9 && router.channels[1]==9,"Current Performance assignments");
    for(unsigned p=0;p<PartCount;++p)need(out[13+p*256+181]==p,"Firmware channel is not private");
    message[7]=3;send(message);need(stored==message && out[13+181]==0 && router.channels[0]==9,"Stored Performance must retain original assignments");
    auto legacy=message;legacy[13+125]=127;legacy[13+256+125]=20;send(legacy);need(stored[13+125]==0 && stored[13+256+125]==20,"Legacy boost repair changed a valid program");
    send({0xbf,98,115,0xbf,6,65});need(out==std::vector<uint8_t>({0xbf,98,115,0xbf,99,0,0xbf,98,115,0xbf,6,65}),"Global NRPN selectors/data must pass on unassigned channels");
    send({0x99,60,0xf8,100});need(out==std::vector<uint8_t>({0xf8,0x90,60,100,0x91,60,100}),"Realtime byte split a channel message");
    send({0xf2,1,2,0xf1,3});need(out==std::vector<uint8_t>({0xf2,1,2,0xf1,3}),"System common routing");
    send({0xb9,32,33,0xc9,0});need(!router.performance && out==std::vector<uint8_t>({0xb9,32,33,0xc9,0}),"Single mode selection");
    send({0x99,60,100});need(out==std::vector<uint8_t>({0x99,60,100}),"Single mode MIDI must remain exact");
    send({0xb9,32,65,0xc9,0});need(router.performance && router.channels[0]==9,"Stored Performance mode selection");
    router.enabled=false;send({0x99,60,100,61,90});need(out==std::vector<uint8_t>({0x99,60,100,61,90}),"Raw-board bypass");
    std::cout<<"PASS: layers, distinct channels, CC, channel reassignment, current/stored Performance, global NRPN, running status, realtime/system common, mode switches\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
