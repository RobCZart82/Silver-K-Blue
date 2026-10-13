#include "kstationHardware.h"

#include <stdexcept>

namespace kstation
{
    Hardware::Hardware(const std::vector<uint8_t>& rom, const std::vector<uint8_t>& flash)
        : m_h8(rom, {
            [this](uint32_t offset) { return m_dsp.hostRead(offset); },
            [this](uint32_t offset, uint8_t value) { return m_dsp.hostWrite(offset, value); },
            [this](bool asserted)
            {
                if (asserted && !m_resetAsserted)
                {
                    m_dsp.reset();
                    m_dspStarted = false;
                }
                m_resetAsserted = asserted;
            }}, flash)
    {
    }

    void Hardware::advance()
    {
        m_h8Cycles += m_h8.exec(128);
        if (!m_dsp.bootFinished() || m_resetAsserted) return;
        if (!m_dspStarted)
        {
            m_dspEpoch = static_cast<int64_t>(m_h8Cycles * 7) - static_cast<int64_t>(m_dsp.cycles());
            m_dspStarted = true;
        }
        const auto target = static_cast<int64_t>(m_h8Cycles * 7) - m_dspEpoch;
        for (size_t iterations = 0; static_cast<int64_t>(m_dsp.cycles()) < target; ++iterations)
        {
            if (iterations == 10000) throw std::runtime_error("K-Station DSP did not advance its clock.");
            m_dsp.step(16);
        }
    }

    bool Hardware::boot(const uint64_t maxH8Cycles)
    {
        const auto start = m_h8Cycles;
        while (m_h8Cycles - start < maxH8Cycles)
        {
            advance();
            m_dsp.discardAudio();
            if (m_h8.bootReady() && m_dsp.bootFinished())
            {
                // The main loop becomes ready before the initial DAC/FX state
                // has settled. Advance both guests normally and discard only
                // this cold-start interval, before any host MIDI is accepted.
                for (size_t i = 0; i < 4096; ++i) renderSample();
                m_dsp.discardAudio();
                return true;
            }
        }
        return false;
    }

    DSP::AudioFrame Hardware::renderSample()
    {
        DSP::AudioFrame frame{};
        const auto start = m_h8Cycles;
        while (!m_dsp.popAudioFrame(frame))
        {
            advance();
            if (m_h8Cycles - start > H8::ClockHz)
                throw std::runtime_error("K-Station ESAI did not produce an audio frame.");
        }
        return frame;
    }
}
