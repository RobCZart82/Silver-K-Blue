#pragma once

#include "kstationDSP.h"
#include "kstationH8.h"

namespace kstation
{
    // A single owner schedules both original processors. Used on the plugin's
    // worker and by offline validation; it never calls guest entry points.
    class Hardware
    {
    public:
        explicit Hardware(const std::vector<uint8_t>& rom, const std::vector<uint8_t>& flash = {});
        bool boot(uint64_t maxH8Cycles = H8::ClockHz * 10ull);
        DSP::AudioFrame renderSample();
        bool enqueueMidi(const uint8_t* bytes, size_t count) { return m_h8.enqueueMidi(bytes, count); }
        bool popMidiOut(uint8_t& byte) { return m_h8.popMidiOut(byte); }
        H8& controller() { return m_h8; }
        DSP& dsp() { return m_dsp; }

    private:
        void advance();
        uint64_t m_h8Cycles = 0;
        int64_t m_dspEpoch = 0;
        bool m_dspStarted = false;
        bool m_resetAsserted = true;
        DSP m_dsp;
        H8 m_h8;
    };
}
