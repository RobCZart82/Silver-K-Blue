#include "kstationHardware.h"
#include "kstationRomLoader.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: kstationHardware_test user-firmware.bin/syx/os");
        const kstation::Rom rom(argv[1]);
        require(rom.isValid(), rom.error().c_str());
        kstation::Hardware board(rom.data());
        require(board.boot(), "Original H8 reset did not reach the main loop");
        const auto initial = board.controller().debugState();
        require(initial.ramOverlay && initial.currentBank == 1 && initial.currentProgram == 0,
                "Original flash loader/program initialization failed");
        require(board.controller().hasProgram(1, 0) && !board.controller().hasProgram(1, 1),
                "Initialized and erased preset slots were not distinguished");
        const uint8_t volume[]{0xb0, 7, 127};
        require(board.enqueueMidi(volume, sizeof(volume)), "UART queue rejected volume");
        for (size_t i = 0; i < 8192; ++i) board.renderSample();

        int64_t silentPeak = 0, peak = 0, releasePeak = 0;
        for (size_t i = 0; i < 2048; ++i)
            for (const auto v : board.renderSample()) silentPeak = std::max(silentPeak, std::abs(int64_t(v)));
        require(silentPeak == 0, "Audio was nonzero before the note");
        const uint8_t note[]{0x90, 69, 100};
        require(board.enqueueMidi(note, sizeof(note)), "UART queue rejected Note On");
        uint64_t positiveCrossings = 0;
        int32_t last = 0;
        for (size_t i = 0; i < 48611; ++i)
        {
            const auto frame = board.renderSample();
            if (i >= 4861)
            {
                for (auto value : frame) peak = std::max(peak, std::abs(int64_t(value)));
                if (last <= 0 && frame[0] > 0) ++positiveCrossings;
            }
            last = frame[0];
        }
        require(peak > 100000 && peak < 8388608, "Original UART Note On did not sustain an unclipped signal");
        const double frequency = positiveCrossings * kstation::DSP::SampleRate / (48611 - 4861);
        require(std::abs(frequency - 440.0) < 2.0, "A4 frequency was incorrect");
        const uint8_t off[]{0x80, 69, 0};
        require(board.enqueueMidi(off, sizeof(off)), "UART queue rejected Note Off");
        for (size_t i = 0; i < 48611; ++i)
        {
            const auto frame = board.renderSample();
            if (i >= 47611)
                for (auto value : frame) releasePeak = std::max(releasePeak, std::abs(int64_t(value)));
        }
        require(releasePeak == 0, "The released init patch did not return to silence");
        constexpr uint8_t notes[]{48, 52, 55, 59, 60, 64, 67, 71};
        for (auto value : notes)
        {
            const uint8_t chord[]{0x90, value, 90};
            require(board.enqueueMidi(chord, sizeof(chord)), "UART queue rejected polyphonic note");
        }
        int64_t chordPeak = 0;
        for (size_t i = 0; i < 16384; ++i)
        {
            const auto frame = board.renderSample();
            if (i >= 8192)
                for (auto value : frame) chordPeak = std::max(chordPeak, std::abs(int64_t(value)));
        }
        require(chordPeak > peak && chordPeak < 8388608, "Eight-note chord was silent or clipped");
        for (auto value : notes)
        {
            const uint8_t release[]{0x80, value, 0};
            require(board.enqueueMidi(release, sizeof(release)), "UART queue rejected polyphonic release");
        }
        int64_t chordTail = 0;
        for (size_t i = 0; i < 48611; ++i)
        {
            const auto frame = board.renderSample();
            if (i >= 47611)
                for (auto value : frame) chordTail = std::max(chordTail, std::abs(int64_t(value)));
        }
        require(chordTail == 0, "Eight-note chord left a stuck voice");
        require(board.dsp().audioFramesDropped() == 0, "ESAI frames were dropped");
        require(board.controller().debugState().midiReceived == 57, "Original UART did not consume all MIDI bytes");
        std::cout << "Original reset/flash/HDI08/DMA/ESAI/UART passed: A4=" << frequency
                  << " Hz peak=" << peak << " releasePeak=" << releasePeak << " eightNotePeak=" << chordPeak << '\n';
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
