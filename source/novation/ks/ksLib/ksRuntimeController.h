#pragma once
#include <cstdint>

namespace ks {
class ControllerCpu;
class LiveMidiController;
// Complete post-boot compatibility controller. Fixed C++ control flow consumes
// live MIDI/state and DSP responses; no runtime opcode decoder, tape or JIT.
// The timing/register ABI is retained to make the handoff reversible and keep
// exact DSP scheduling. This is not yet an event-batched controller design.
class RuntimeController final {
public:
    RuntimeController(const ControllerCpu&,LiveMidiController&);
    void step(ControllerCpu&);
    // Exposed for differential verification of individual native boundaries.
    bool stepCode(ControllerCpu&);
    static void arbitrate(ControllerCpu&);
    uint64_t steps=0,interruptEntries=0;
private:
    LiveMidiController& shared;
    bool io(ControllerCpu&);
    bool midi(ControllerCpu&);
    bool state(ControllerCpu&);
    bool parameters(ControllerCpu&);
    bool voices(ControllerCpu&);
    bool extended(ControllerCpu&);
};
}
