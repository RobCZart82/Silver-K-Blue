#pragma once
#include "ksState.h"

namespace ks {
// The desktop editor addresses parts, whereas external MIDI addresses channels.
// Give the firmware four private receive channels and fan external messages out
// according to the user's channel assignments. Original UART/CC/NRPN processing
// still performs every sound edit; no patch reload or guest RAM write is needed.
class PartMidi {
public:
    bool enabled=false, performance=false;
    uint8_t globalChannel=0;
    std::array<uint8_t,PartCount> channels{};
    void seed(const uint8_t* programs){for(unsigned p=0;p<PartCount;++p)channels[p]=programs[p*256+181]&15;performance=true;}
    template<class Emit,class Resolve,class Store>
    bool accept(uint8_t byte,Emit emit,Resolve resolve,Store store){
        if(!enabled)return emit(&byte,1);
        if(byte>=0xf8)return emit(&byte,1);
        if(byte==0xf0){inSysex=true;sysexSize=0;status=used=0;}
        if(inSysex){
            if(sysexSize<sysex.size())sysex[sysexSize]=byte;
            else {if(sysexSize==sysex.size() && !emit(sysex.data(),sysex.size()))return false;if(!emit(&byte,1))return false;}
            ++sysexSize;
            if(byte!=0xf7)return true;
            inSysex=false;
            if(sysexSize>sysex.size())return true;
            constexpr uint8_t prefix[]{0xf0,0,0x20,0x29,1,0x42};
            if(sysexSize>=14 && std::equal(std::begin(prefix),std::end(prefix),sysex.begin()) && (sysex[6]==127 || sysex[6]==globalChannel)){
                // Older Silver-K versions exposed a nonexistent 0..127 boost.
                // Repair only invalid values; valid programs remain byte-exact.
                if(sysexSize==270 && sysex[7]<=1 && sysex[13+125]>30)sysex[13+125]=0;
                if(sysexSize==1038 && (sysex[7]==2 || sysex[7]==3)){
                    for(unsigned p=0;p<PartCount;++p)if(sysex[13+p*256+125]>30)sysex[13+p*256+125]=0;
                    if(sysex[7]==2 && performance)seed(sysex.data()+13);
                    if(sysex[7]==3)store(sysex.data(),sysexSize);
                    for(unsigned p=0;p<PartCount;++p)sysex[13+p*256+181]=uint8_t(p);
                }
            }
            return emit(sysex.data(),sysexSize);
        }
        if(byte&0x80){status=byte;used=0;if(byte>=0xf0){if(byte==0xf1 || byte==0xf2 || byte==0xf3)return true;status=0;return emit(&byte,1);}return true;}
        if(!status)return emit(&byte,1);
        data[used++]=byte;
        const unsigned kind=status&0xf0, count=(kind==0xc0 || kind==0xd0 || status==0xf1 || status==0xf3)?1:2;
        if(used<count)return true;
        used=0;const uint8_t channel=status&15;
        if(status>=0xf0){const uint8_t packet[]{status,data[0],data[1]};status=0;return emit(packet,count+1);}
        if(kind==0xb0){
            if(data[0]==119 && data[1]>30)data[1]=30;
            if(data[0]==32)banks[channel]=data[1];
            if(data[0]==98)nrpn[channel]=data[1];
            if(data[0]==6 && nrpn[channel]==113)globalChannel=data[1]&15;
        }
        if(kind==0xc0){
            if(banks[channel]==65 && data[0]<100){seed(resolve(data[0]));}
            else if(banks[channel]>=33 && banks[channel]<=40)performance=false;
        }
        const uint8_t packet[]{status,data[0],data[1]};
        // Mode selection and global NRPNs must reach the original global path.
        const bool modeSelect=(kind==0xb0 && data[0]==32 && banks[channel]>=33) ||
                              (kind==0xc0 && banks[channel]>=33);
        const bool globalNrpn=kind==0xb0 && ((data[0]==6 && nrpn[channel]>=113) || (data[0]==98 && data[1]>=113));
        if(performance && globalNrpn && data[0]==6){
            const uint8_t parameter[]{status,99,0,status,98,nrpn[channel],status,6,data[1]};
            return emit(parameter,sizeof(parameter));
        }
        if(!performance || modeSelect || globalNrpn)return emit(packet,count+1);
        if(kind==0xb0 && data[0]==6 && nrpn[channel]==86){
            for(auto& c:channels)if(c==channel)c=data[1]&15;
            return true; // Private firmware channels remain fixed.
        }
        for(unsigned part=0;part<PartCount;++part)if(channels[part]==channel){
            // Editor transactions also use each private channel's NRPN state.
            // Restore the external sender's selector with its Data Entry byte.
            if(kind==0xb0 && data[0]==6){
                const auto s=uint8_t(0xb0|part);
                const uint8_t parameter[]{s,99,0,s,98,nrpn[channel],s,6,data[1]};
                if(!emit(parameter,sizeof(parameter)))return false;
                continue;
            }
            const uint8_t routed[]{uint8_t(kind|part),data[0],data[1]};
            if(!emit(routed,count+1))return false;
        }
        return true;
    }
private:
    std::array<uint8_t,16> banks{},nrpn{};
    std::array<uint8_t,1038> sysex{};
    std::array<uint8_t,2> data{};
    size_t sysexSize=0;
    uint8_t status=0,used=0;
    bool inSysex=false;
};
}
