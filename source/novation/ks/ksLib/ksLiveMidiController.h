#pragma once
#include "h8s/h8s.hpp"
#include <cstdint>
#include <cstddef>
#include <utility>
#include "ksLiveNoteController.h"
#include "ksLiveVoiceController.h"
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace ks {
// Board-local ABI adapter; the shared H8 core and its semantics stay untouched.
class ControllerCpu final : public H8SEmulator {
public:
    void step(){if(!interpreterAllowed)throw std::logic_error("H8 interpreter called after native KS handoff");++interpretedSteps;H8SEmulator::step();}
    uint64_t interpretedSteps=0;
    bool interpreterAllowed=true;
    bool nativeFetchClocksOnly=false;
    // Native data RAM access avoids the H8 device-map dispatch. MMIO, Flash,
    // waves and protected code writes still use the actual Board callbacks.
    const uint8_t* nativeLowBase=nullptr;
    unsigned nativeLowSize=0;
    bool nativeLowWritable=false;
    int8_t read8(int address){
        if(!nativeLowBase)return H8SEmulator::read8(address);
        const auto a=unsigned(address)&0xffffff;clockMem(a,lastread);
        if(a<0x200000)return a<nativeLowSize?int8_t(nativeLowBase[a]):int8_t(-1);
        if((a>=0xa00000 && a<0xc00000)||(a>=0xffdc00 && a<0xfffc00))return int8_t(memory[a]);
        return maps[a]?int8_t(maps[a]->read(a)):int8_t(memory[a]);
    }
    uint16_t read16(int a){const auto high=uint8_t(read8(a));return uint16_t((unsigned(high)<<8)|uint8_t(read8(a+1)));}
    uint32_t read32(int a){const auto high=read16(a);return (uint32_t(high)<<16)|read16(a+2);}
    void write8(int8_t value,int address){
        if(!nativeLowBase){H8SEmulator::write8(value,address);return;}
        const auto a=unsigned(address)&0xffffff;clockMem(a,lastwrite);
        if(a>=0xb8e8 && a<0x200000 && nativeLowWritable){memory[0xa00000+a]=uint8_t(value);return;}
        if((a>=0xa0b8e8 && a<0xc00000)||(a>=0xffdc00 && a<0xfffc00)){memory[a]=uint8_t(value);return;}
        if(maps[a])maps[a]->write(a,uint8_t(value));else memory[a]=uint8_t(value);
    }
    void write16(int16_t v,int a){write8(int8_t(v>>8),a);write8(int8_t(v),a+1);}
    void write32(int32_t v,int a){write16(int16_t(v>>16),a);write16(int16_t(v),a+2);}
    void pushL(uint32_t v){auto sp=getSP()-4;write32(int32_t(v),sp);setSP(sp);}
    void pushW(uint16_t v){auto sp=getSP()-2;write16(int16_t(v),sp);setSP(sp);}
    uint32_t popL(){auto sp=getSP();setSP(sp+4);return read32(sp);}
    uint16_t popW(){auto sp=getSP();setSP(sp+2);return read16(sp);}
    void pushPC(uint8_t* target,bool flags=false){auto v=uint32_t(pcoff(target))&0xffffff;pushL(v|(flags?uint32_t(ccr)<<24:0));}
    uint8_t* popPC(bool flags=false){auto v=popL();if(flags)ccr=uint8_t(v>>24);return makepc(v&0xffffff);}
    uint8_t* loadPCFrom(uint32_t a){return makepc(read32(a)&0xffffff);}
    using H8SEmulator::indent;
    using H8SEmulator::sub8;
    using H8SEmulator::add32;
    using H8SEmulator::add8;
    using H8SEmulator::add16;
    using H8SEmulator::sub16;
    using H8SEmulator::sub32;
    using H8SEmulator::lastread;
    using H8SEmulator::lastwrite;
};
// Live C++ byte decoder and note/key/sustain handlers for OS 2.1.02.
// Retains the firmware ABI, UART timing and interrupt boundaries.
// Parameter calculators, periodic servicing, clock/arp and state loaders
// remain downstream. Voice lifecycle and DSP transport also run natively.
// This is a hybrid, NOT yet a controller that can run with H8 removed.
class LiveMidiController final {
public:
    explicit LiveMidiController(const H8SEmulator&);
    // One original guest boundary; false delegates to the original H8.
    bool step(ControllerCpu&);
    static bool candidate(unsigned pc){
        // Cheap page filter in the hot Board loop; step() validates exact PCs.
        constexpr uint64_t pages[]{0x007be03000000000ull,0x0180000000000001ull,0x00000078001f8000ull,0x0000000000000000ull};
        return pc<0x10000 && ((pages[pc>>14]>>((pc>>8)&63))&1);
    }
    LiveNoteController noteActions;
    LiveVoiceController voiceActions;
    // Count actual original-core calls, including interrupt arbitration. These
    // are not instruction counts or CPU time, and never imply full migration.
    void delegated(unsigned pc){++originalStepCalls;if(!originalPages.empty())++originalPages[(pc&0xffffff)>>8];}
    void enableDelegationProfile(){originalPages.assign(65536,0);originalStepCalls=0;}
    void resetMetrics(){
        nativeSteps=bytes=messages=noteOn=noteOff=controls=originalStepCalls=0;
        noteActions=LiveNoteController{};voiceActions=LiveVoiceController{};
        if(!originalPages.empty())std::fill(originalPages.begin(),originalPages.end(),0);
    }
    uint64_t originalStepCalls=0;
    // Detailed page accounting is fixture-only: no allocation or page writes
    // in a normal experimental plugin instance. Heap storage avoids the small
    // default Windows stack when a probe explicitly enables it.
    std::vector<uint64_t> originalPages;
    uint64_t nativeSteps=0,bytes=0,messages=0,noteOn=0,noteOff=0,controls=0;
private:
    bool decoding=false;
    static bool supported(uint8_t status);
};
}
