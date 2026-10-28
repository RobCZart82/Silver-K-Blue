#include "kstationHardware.h"
#include "kstationRomLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
    void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
    void send(kstation::Hardware& board, std::initializer_list<uint8_t> bytes)
    {
        const std::vector<uint8_t> midi(bytes);
        require(board.enqueueMidi(midi.data(), midi.size()), "UART rejected test MIDI");
    }
    double render(kstation::Hardware& board, unsigned frames, unsigned skip = 0)
    {
        double sum = 0;
        for (unsigned i = 0; i < frames; ++i)
        {
            const auto frame = board.renderSample();
            for (auto word : frame)
            {
                const double x = word / 8388608.0;
                require(std::isfinite(x) && std::abs(x) < 1, "Nonfinite or clipped output");
                if (i >= skip) sum += x * x;
            }
        }
        return std::sqrt(sum / ((frames - skip) * 2));
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: kstationArp_test user-firmware.bin/syx/os");
        kstation::Rom rom(argv[1]);
        require(rom.isValid(), rom.error().c_str());
        kstation::Hardware board(rom.data());
        require(board.boot(), "H8 boot failed");
        // Init patch, internal tempo, latch off, keysync on, one octave.
        send(board, {0xb0, 7, 127, 0xb0, 9, 127, 0xb0, 87, 3, 0xb0, 88, 64, 0xb0, 89, 12});
        render(board, 4096);
        std::array<uint8_t,128> patch{};
        board.controller().copyCurrentPatch(patch.data());
        require(patch[88] == 12, "Arpeggiator configuration did not reach original firmware");
        double minimumRms = 1;
        // Original Up search starts at zero and searches strictly above it;
        // MIDI note 0 is its boundary sentinel, rather than an audible Up note.
        // Bitmap handling for every 0..127 note is also tested separately.
        for (unsigned note = 1; note < 128; ++note)
        {
            send(board, {0x90, uint8_t(note), 100});
            const auto rms = render(board, 14584, 4861); // 300 ms, discard initial 100 ms.
            const auto bitmap = board.controller().readMemory(0xcbde + (note >> 3));
            const auto mask = 1u << (note & 7);
            require((bitmap & mask) != 0, "Original held-note bitmap missed Note On");
            require(rms > 0.0001, "Arpeggiator produced silence for a MIDI note");
            minimumRms = std::min(minimumRms, rms);
            send(board, {0x80, uint8_t(note), 0});
            const auto tail = render(board, 24306, 21875); // last 50 ms of 500 ms release.
            require((board.controller().readMemory(0xcbde + (note >> 3)) & mask) == 0,
                    "Original held-note bitmap missed Note Off");
            require(tail < 0.000001, "Arpeggiator left a stuck note");
            std::cout << "NOTE " << note << " rms=" << rms << " tail=" << tail << '\n';
        }
        // Check audio and release for every octave setting; this does not
        // establish the ordering or exact pitch of every generated step.
        for (uint8_t octaves = 0; octaves < 4; ++octaves)
        {
            send(board, {0xb0, 89, uint8_t(12 | octaves), 0x90, 48, 100});
            require(render(board, 48611, 4861) > 0.0001, "Octave setting silenced arpeggiator");
            send(board, {0x80, 48, 0});
            require(render(board, 24306, 21875) < 0.000001, "Octave setting left a stuck note");
        }
        require(board.dsp().audioFramesDropped() == 0, "ESAI frames dropped");
        std::cout << "RESULT {\"notesPassed\":127,\"noteOffPassed\":127,\"octaveSettingsPassed\":4,\"minimumRms\":"
                  << minimumRms << ",\"audioFramesDropped\":0}\n";
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
