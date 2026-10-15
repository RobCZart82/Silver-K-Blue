#pragma once
#include "h8s/h8s.hpp"
#include <cstdint>
#include <cstddef>
#include <utility>
#include "ksLiveNoteController.h"

namespace ks {
// Board-local ABI adapter; the shared H8 core and its semantics stay untouched.
class ControllerCpu final : public H8SEmulator {
public:
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
// Retains the firmware ABI, UART timing and interrupt boundaries; the original
// DSP voice allocation/parameter generation engine remains downstream.
// This is a hybrid, NOT yet a controller that can run with H8 removed.
class LiveMidiController final {
public:
    explicit LiveMidiController(const H8SEmulator&);
    // One original guest boundary; false delegates to the original H8.
    bool step(ControllerCpu&);
    static bool candidate(unsigned pc){
        // Cheap page filter in the hot Board loop; step() validates exact PCs.
        constexpr uint64_t pages[]{0x007be00000000000ull,0x1ull,0x0000004000180000ull,0};
        return pc<0x10000 && ((pages[pc>>14]>>((pc>>8)&63))&1);
    }
    LiveNoteController noteActions;
    uint64_t nativeSteps=0,bytes=0,messages=0,noteOn=0,noteOff=0,controls=0;
private:
    bool decoding=false;
    static bool supported(uint8_t status);
};
}
