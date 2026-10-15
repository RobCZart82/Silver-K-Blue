#pragma once
#include "h8s/h8s.hpp"
#include <cstdint>
#include <cstddef>
#include <utility>

namespace ks {
// Board-local ABI adapter; the shared H8 core and its semantics stay untouched.
class ControllerCpu final : public H8SEmulator {
public:
    using H8SEmulator::indent;
    using H8SEmulator::sub8;
    using H8SEmulator::add32;
    using H8SEmulator::lastread;
    using H8SEmulator::lastwrite;
};
// First live stage: a C++ port of the OS 2.1.02 Note/CC byte decoder.
// Retains the firmware ABI, UART timing and instruction boundaries so original
// voice allocation/parameter generation can remain the reference downstream.
// This is a hybrid, NOT yet a controller that can run with H8 removed.
class LiveMidiController final {
public:
    explicit LiveMidiController(const H8SEmulator&);
    // One original guest boundary; false delegates to the original H8.
    bool step(ControllerCpu&);
    uint64_t nativeSteps=0,bytes=0,messages=0,noteOn=0,noteOff=0,controls=0;
private:
    bool decoding=false;
    static bool supported(uint8_t status);
};
}
