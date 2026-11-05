#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace ks {
// The zero bank stays byte-exact. Only original MIDI mixer commands are adapted.
// Desired values are retained so generated waves and saved programs keep their levels.
class PlaceholderMidi {
public:
    bool enabled = false;
    uint8_t globalChannel = 0;
    std::array<uint8_t, 16> bank{}, activeBank{};
    std::array<std::array<uint8_t, 3>, 16> waves{}, levels{};

    void seed(uint8_t channel, const uint8_t* program) {
        for (unsigned i = 0; i < 3; ++i) {
            waves[channel][i] = program[128 + i];
            levels[channel][i] = program[37 + i];
        }
        for (size_t i = 0; i < fields.size(); ++i) desired[channel][i] = program[fields[i].offset];
        if(performance)for(unsigned part=0;part<4;++part)if(partChannels[part]==channel)
            std::copy_n(program,256,partPrograms[part].begin());
    }
    void seedPerformance(const uint8_t* programs) {
        performance=false;
        for(unsigned part=0;part<4;++part){const auto* p=programs+part*256;
            partChannels[part]=p[181]&15;std::copy_n(p,256,partPrograms[part].begin());seed(partChannels[part],p);}
        performance=true;
    }
    void restorePart(unsigned part,uint8_t* program)const {
        if(enabled && performance && part<4)restoreProgram(program,partPrograms[part].data());
    }

    // Resolve a Program Change against the board's external user Flash. The
    // callback seeds the new program and returns true only for a valid address.
    template<class Emit, class Resolve, class Store>
    bool accept(uint8_t byte, Emit emit, Resolve resolve, Store store) {
        if (!enabled || byte >= 0xf8) return emit(&byte, 1);
        if (byte == 0xf0) {
            sysex = true;
            sysexCount = 0;
            sysexData[sysexCount++] = byte;
            used = status = 0;
            return true;
        }
        if (sysex) {
            if (sysexCount < sysexData.size()) sysexData[sysexCount] = byte;
            else {
                if (sysexCount == sysexData.size() && !emit(sysexData.data(), sysexData.size())) return false;
                if (!emit(&byte, 1)) return false;
            }
            ++sysexCount;
            if (byte == 0xf7) {
                sysex = false;
                if (sysexCount > sysexData.size()) return true;
                constexpr uint8_t prefix[]{0xf0, 0, 0x20, 0x29, 1, 0x42};
                const bool ksMessage=sysexCount>=14 && std::equal(std::begin(prefix),std::end(prefix),sysexData.begin());
                store(sysexData.data(),sysexCount); // Preserve desired fields before muting.
                if(ksMessage && sysexCount==270 && sysexData[7]==0) {
                    performance=false;
                    seed(globalChannel,sysexData.data()+13);
                    // Mute before the preset applies its ROM waves. The same
                    // original program command carries already-muted fields.
                    if(!mute(globalChannel,emit))return false;
                    muteProgram(sysexData.data()+13);
                }
                if(ksMessage && sysexCount==1038 && sysexData[7]==2) {
                    seedPerformance(sysexData.data()+13);
                    for(unsigned part=0;part<4;++part) {
                        auto* p=sysexData.data()+13+part*256;
                        const auto channel=p[181]&15;
                        if(!mute(channel,emit))return false;
                        muteProgram(p);
                    }
                }
                if(!emit(sysexData.data(),sysexCount))return false;
            }
            return true;
        }
        if (byte & 0x80) {
            status = byte;
            used = 0;
            // System common cancels running status; F6 has no data bytes.
            if (byte >= 0xf4) { status = 0; return emit(&byte, 1); }
            return true;
        }
        if (!status) return emit(&byte, 1);
        data[used++] = byte;
        const unsigned count = ((status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0
                                || status == 0xf1 || status == 0xf3) ? 1 : 2;
        if (used < count) return true;
        const uint8_t channel = status & 15;
        std::array<uint8_t, 256> out{};
        size_t size = 0;
        auto cc = [&](uint8_t number, uint8_t value) {
            out[size++] = 0xb0 | channel; out[size++] = number; out[size++] = value;
        };
        auto parameter = [&](const Field& f, uint8_t value) {
            if (f.nrpn) { cc(98, f.number); cc(6, value); }
            else cc(f.number, value);
        };
        const auto oldMask = romMask(channel);
        bool waveChanged = false;
        uint16_t changedChannels = 0;
        if ((status & 0xf0) == 0xb0) {
            const auto number = data[0];
            auto& value = data[1];
            const auto desiredValue=value;
            if (number == 32) bank[channel] = value;
            if (number == 98) nrpn[channel] = value;
            // KS ignores NRPN MSB, matching the original MIDI implementation.
            for (size_t i = 0; i < fields.size(); ++i) {
                const auto& f = fields[i];
                if ((!f.nrpn && number == f.number) || (f.nrpn && number == 6 && nrpn[channel] == f.number)) {
                    desired[channel][i] = value;
                    if(performance)for(unsigned part=0;part<4;++part)if(partChannels[part]==channel)
                        partPrograms[part][f.offset]=desiredValue;
                    if (i < 3) levels[channel][i] = value;
                    if (oldMask & f.dependencies) value = f.muted;
                }
            }
            if (number == 6 && nrpn[channel] >= 50 && nrpn[channel] <= 52) {
                waves[channel][nrpn[channel] - 50] = value;
                if(performance)for(unsigned part=0;part<4;++part)if(partChannels[part]==channel)
                    partPrograms[part][128+nrpn[channel]-50]=value;
                waveChanged = true;
            }
            // KS CC70 selects the glide curve. Waveforms use NRPN 50..52;
            // interpreting K-Station's packed wave CC here would unmute ROMs.
            if(performance && number==6 && nrpn[channel]==86)
                for(unsigned part=0;part<4;++part)if(partChannels[part]==channel)partChannels[part]=value&15;
            // Set all new mutes before the original wave command. Restore the
            // NRPN selector because injected modulation commands use CC98 too.
            if (waveChanged) {
                const auto mask = romMask(channel);
                bool selected = false;
                for (size_t i = 0; i < fields.size(); ++i) {
                    const auto& f = fields[i];
                    if ((mask & f.dependencies) && !(oldMask & f.dependencies)) {
                        parameter(f, f.muted); selected |= f.nrpn;
                    }
                }
                if (selected) cc(98, nrpn[channel]);
            }
        } else if ((status & 0xf0) == 0xc0) {
            if(bank[channel]>=33 && bank[channel]<=40)performance=false;
            changedChannels = resolve(channel, bank[channel], data[0]);
            for(uint8_t ch=0;ch<16;++ch)if(changedChannels & (1u<<ch))activeBank[ch]=bank[channel];
        }
        for(uint8_t ch=0;ch<16;++ch)if((changedChannels & (1u<<ch)) && !mute(ch,emit))return false;
        out[size++] = status; out[size++] = data[0];
        if (count == 2) out[size++] = data[1];
        if (waveChanged) {
            const auto mask = romMask(channel);
            bool selected = false;
            for (size_t i = 0; i < fields.size(); ++i) {
                const auto& f = fields[i];
                if ((oldMask & f.dependencies) && !(mask & f.dependencies)) {
                    parameter(f, desired[channel][i]); selected |= f.nrpn;
                }
            }
            if (selected) cc(98, nrpn[channel]);
        }
        used = 0;
        if (status >= 0xf0) status = 0;
        if (!emit(out.data(), size)) return false;
        return true;
    }

    template<class Emit> bool mute(uint8_t channel, Emit emit) const {
        std::array<uint8_t, 128> out{};
        size_t size = 0;
        auto cc = [&](uint8_t number, uint8_t value) {
            out[size++] = 0xb0 | channel; out[size++] = number; out[size++] = value;
        };
        const auto mask = romMask(channel);
        bool selected = false;
        for (const auto& f : fields) if (mask & f.dependencies) {
            if (f.nrpn) { cc(98, f.number); cc(6, f.muted); selected = true; }
            else cc(f.number, f.muted);
        }
        if (selected) cc(98, nrpn[channel]);
        return !size || emit(out.data(), size);
    }
    static void muteProgram(uint8_t* program) {
        uint8_t mask=0;for(unsigned i=0;i<3;++i)if(program[128+i]>=8)mask|=1u<<i;
        for(const auto& f:fields)if(mask & f.dependencies)program[f.offset]=f.muted;
    }
    static void restoreProgram(uint8_t* program,const uint8_t* original) {
        uint8_t mask=0;for(unsigned i=0;i<3;++i)if(program[128+i]>=8)mask|=1u<<i;
        for(const auto& f:fields)if(mask & f.dependencies)program[f.offset]=original[f.offset];
    }
    void restoreDesired(uint8_t channel, uint8_t* program) const {
        if (enabled) for (size_t i = 0; i < fields.size(); ++i)
            if (romMask(channel) & fields[i].dependencies) program[fields[i].offset] = desired[channel][i];
    }
private:
    struct Field { uint8_t number, offset, muted, dependencies; bool nrpn; };
    // Signed modulation depths have centre 64. Mixer, ring and FM paths must
    // all be muted: zero mixer level alone can be raised by level modulation.
    inline static constexpr std::array<Field, 12> fields{{
        {72,37,0,1,false}, {73,38,0,2,false}, {74,39,0,4,false},
        {76,41,0,3,false}, {0,56,0,2,true}, {1,57,64,2,true},
        {63,141,64,1,true}, {64,142,64,2,true}, {65,143,64,4,true},
        {77,171,64,1,true}, {67,145,64,3,true}, {78,172,64,2,true}
    }};
    uint8_t romMask(uint8_t channel) const {
        uint8_t mask = 0;
        if(performance){for(unsigned part=0;part<4;++part)if(partChannels[part]==channel)
            for(unsigned i=0;i<3;++i)if(partPrograms[part][128+i]>=8)mask|=1u<<i;return mask;}
        for (unsigned i = 0; i < 3; ++i) if (waves[channel][i] >= 8) mask |= 1 << i;
        return mask;
    }
    std::array<std::array<uint8_t, fields.size()>, 16> desired{};
    bool performance=false;
    std::array<uint8_t,4> partChannels{};
    std::array<std::array<uint8_t,256>,4> partPrograms{};
    std::array<uint8_t, 16> nrpn{};
    std::array<uint8_t, 2> data{};
    uint8_t status = 0, used = 0;
    bool sysex = false;
    size_t sysexCount = 0;
    std::array<uint8_t, 1038> sysexData{};
};
}
