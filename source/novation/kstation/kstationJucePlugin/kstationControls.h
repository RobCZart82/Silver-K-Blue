#pragma once
#include <array>
#include <cstdint>

// Sources: official K-Station manual v1.1a Appendix A2-A5/A9-A11,
// and OS 2.1.01 tables BCC6 (MIDI -> patch), BB2A (patch -> MIDI).
// Values are MIDI/logical raw values; signed continuous values have zero=64.
// Defaults describe the OS-embedded starting patch, not a hardware reset guarantee.
// patchOffset=-1 is a live performance control, outside the 128-byte program.
namespace kstation::controls
{
    enum class Mapping : uint8_t { CC, NRPN };
    struct ControlSpec
    {
        const char* id;
        const char* label;
        const char* group;
        const char* section;
        Mapping mapping;
        uint16_t number;
        uint8_t minimum, maximum, defaultValue;
        int16_t patchOffset;
        uint8_t patchMask, patchShift;
        uint8_t wireMask, wireShift;
        const char* const* enumLabels;
        uint8_t enumCount;
        int16_t displayOffset; // Display raw+offset; does not alter wire encoding.
    };
    inline constexpr const char* Wave[]{"Sine","Triangle","Saw","Pulse"};
    inline constexpr const char* Octave[]{"-1","0","+1","+2"};
    inline constexpr const char* VoiceMode[]{"Mono","Mono autoglide","Poly","Poly retrigger"};
    inline constexpr const char* Unison[]{"Off","2","3","4","5","6","7","8"};
    inline constexpr const char* FilterSlope[]{"12 dB/oct","24 dB/oct"};
    inline constexpr const char* Portamento[]{"Exponential","Linear"};
    inline constexpr const char* LfoWave[]{"Triangle","Saw","Square","Sample & hold"};
    inline constexpr const char* OffOn[]{"Off","On"};
    inline constexpr const char* Trigger[]{"Single","Multi"};
    inline constexpr const char* ArpPattern[]{"Up","Down","Up/down 1","Up/down 2","As played","Random"};
    inline constexpr const char* ArpOctave[]{"1","2","3","4"};
    inline constexpr const char* ReverbType[]{"Echo chamber","Small room","Small hall","Large room","Large hall","Grand hall"};
    inline constexpr const char* ChorusType[]{"Chorus","Phaser"};
    inline constexpr const char* DelayRatio[]{"1/1","4/3","3/4","3/2","2/3","2/1","1/2","3/1","1/3","4/1","1/4","1/Off","Off/1"};
    inline constexpr const char* SyncPan[]{"Off","Left","Centre","Right"};
    inline constexpr const char* SyncEq[]{"Off","Low","Mid","High"};
    inline constexpr const char* Sibilance[]{"High-pass","Noise"};
    inline constexpr const char* LfoLock[]{"Per voice","Common"};
    inline constexpr const char* Phase[]{"Off","On"};
    inline constexpr const char* WavePhase[]{"Free running","0 degrees","24 degrees","48 degrees","72 degrees","96 degrees","120 degrees","144 degrees","168 degrees","192 degrees","216 degrees","240 degrees","264 degrees","288 degrees","312 degrees","336 degrees"};
    inline constexpr const char* Sync[]{"Off","1/32 triplet","1/32","1/16 triplet","1/16","1/8 triplet","1/16 dotted","1/8","1/4 triplet","1/8 dotted","1/4","1/2 triplet","1/4 dotted","1/2","1 bar triplet","1/2 dotted","1 bar","2 bars triplet","1 bar dotted","2 bars","4 bars triplet","3 bars","5 bars triplet","4 bars","3 bars dotted","7 bars triplet","5 bars","8 bars triplet","6 bars","7 bars","5 bars dotted","8 bars","9 bars","7 bars dotted","12 bars"};
    inline constexpr const char* ArpSync[]{"1/32 triplet","1/32","1/16 triplet","1/16","1/8 triplet","1/16 dotted","1/8","1/4 triplet","1/8 dotted","1/4","1/2 triplet","1/4 dotted","1/2","1 bar triplet","1/2 dotted","1 bar"};

    inline constexpr std::array<ControlSpec, 154> Catalog{{
        {"volume", "Volume", "Voice", "Performance", Mapping::CC, 7, 0, 127, 127, -1, 127, 0, 127, 0, nullptr, 0, 0},
        {"modulation", "Mod wheel", "Voice", "Performance", Mapping::CC, 1, 0, 127, 0, -1, 127, 0, 127, 0, nullptr, 0, 0},
        {"breath", "Breath", "Voice", "Performance", Mapping::CC, 2, 0, 127, 0, -1, 127, 0, 127, 0, nullptr, 0, 0},
        {"voice_level", "Voice level", "Voice", "Performance", Mapping::CC, 119, 0, 127, 64, 125, 127, 0, 127, 0, nullptr, 0, 0},
        {"portamento_time", "Portamento time", "Voice", "Glide", Mapping::CC, 5, 0, 127, 0, 3, 127, 0, 127, 0, nullptr, 0, 0},
        {"preglide", "Preglide", "Voice", "Glide", Mapping::CC, 8, 52, 76, 64, 4, 127, 0, 127, 0, nullptr, 0, -64},
        {"portamento_mode", "Glide curve", "Voice", "Glide", Mapping::CC, 70, 0, 1, 0, 5, 64, 6, 64, 6, Portamento, 2, 0},
        {"voice_mode", "Voice mode", "Voice", "Polyphony", Mapping::CC, 67, 0, 3, 2, 0, 24, 3, 24, 3, VoiceMode, 4, 0},
        {"unison", "Unison voices", "Voice", "Polyphony", Mapping::CC, 67, 0, 7, 0, 0, 7, 0, 7, 0, Unison, 8, 0},
        {"unison_detune", "Unison detune", "Voice", "Polyphony", Mapping::CC, 68, 0, 127, 25, 1, 127, 0, 127, 0, nullptr, 0, 0},
        {"random_detune", "Random detune", "Voice", "Polyphony", Mapping::CC, 69, 0, 127, 0, 2, 127, 0, 127, 0, nullptr, 0, 0},
        {"osc_key_phase", "Wave keysync", "Voice", "Polyphony", Mapping::CC, 65, 0, 15, 7, 80, 120, 3, 120, 3, WavePhase, 16, 0},
        {"osc1_wave", "Waveform", "Oscillators", "Oscillator 1", Mapping::CC, 70, 0, 3, 2, 5, 3, 0, 3, 0, Wave, 4, 0},
        {"osc1_octave", "Octave", "Oscillators", "Oscillator 1", Mapping::CC, 71, 0, 3, 1, 6, 3, 0, 3, 0, Octave, 4, 0},
        {"osc1_semitone", "Semitone", "Oscillators", "Oscillator 1", Mapping::CC, 40, 52, 76, 64, 7, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc1_fine", "Fine tune", "Oscillators", "Oscillator 1", Mapping::CC, 41, 14, 114, 64, 8, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc1_bend_depth", "Pitch bend", "Oscillators", "Oscillator 1", Mapping::CC, 42, 0, 127, 66, 9, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc1_lfo1_pitch", "LFO1 pitch", "Oscillators", "Oscillator 1", Mapping::CC, 43, 0, 127, 64, 10, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc1_env_pitch", "Envelope pitch", "Oscillators", "Oscillator 1", Mapping::CC, 44, 0, 127, 64, 11, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc1_pulse_width", "Pulse / double phase", "Oscillators", "Oscillator 1", Mapping::CC, 45, 0, 127, 64, 12, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc1_lfo2_pwm", "LFO2 pulse width", "Oscillators", "Oscillator 1", Mapping::CC, 46, 0, 127, 64, 13, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc1_env_pwm", "Envelope pulse width", "Oscillators", "Oscillator 1", Mapping::CC, 47, 0, 127, 64, 14, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc2_wave", "Waveform", "Oscillators", "Oscillator 2", Mapping::CC, 70, 0, 3, 2, 5, 12, 2, 12, 2, Wave, 4, 0},
        {"osc2_octave", "Octave", "Oscillators", "Oscillator 2", Mapping::CC, 71, 0, 3, 1, 6, 12, 2, 12, 2, Octave, 4, 0},
        {"osc2_semitone", "Semitone", "Oscillators", "Oscillator 2", Mapping::CC, 48, 52, 76, 64, 15, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc2_fine", "Fine tune", "Oscillators", "Oscillator 2", Mapping::CC, 49, 14, 114, 64, 16, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc2_bend_depth", "Pitch bend", "Oscillators", "Oscillator 2", Mapping::CC, 50, 0, 127, 66, 17, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc2_lfo1_pitch", "LFO1 pitch", "Oscillators", "Oscillator 2", Mapping::CC, 51, 0, 127, 64, 18, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc2_env_pitch", "Envelope pitch", "Oscillators", "Oscillator 2", Mapping::CC, 52, 0, 127, 64, 19, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc2_pulse_width", "Pulse / double phase", "Oscillators", "Oscillator 2", Mapping::CC, 53, 0, 127, 64, 20, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc2_lfo2_pwm", "LFO2 pulse width", "Oscillators", "Oscillator 2", Mapping::CC, 54, 0, 127, 64, 21, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc2_env_pwm", "Envelope pulse width", "Oscillators", "Oscillator 2", Mapping::CC, 55, 0, 127, 64, 22, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc3_wave", "Waveform", "Oscillators", "Oscillator 3", Mapping::CC, 70, 0, 3, 3, 5, 48, 4, 48, 4, Wave, 4, 0},
        {"osc3_octave", "Octave", "Oscillators", "Oscillator 3", Mapping::CC, 71, 0, 3, 0, 6, 48, 4, 48, 4, Octave, 4, 0},
        {"osc3_semitone", "Semitone", "Oscillators", "Oscillator 3", Mapping::CC, 56, 52, 76, 64, 23, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc3_fine", "Fine tune", "Oscillators", "Oscillator 3", Mapping::CC, 57, 14, 114, 64, 24, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc3_bend_depth", "Pitch bend", "Oscillators", "Oscillator 3", Mapping::CC, 58, 0, 127, 66, 25, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc3_lfo1_pitch", "LFO1 pitch", "Oscillators", "Oscillator 3", Mapping::CC, 59, 0, 127, 64, 26, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc3_env_pitch", "Envelope pitch", "Oscillators", "Oscillator 3", Mapping::CC, 60, 0, 127, 64, 27, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc3_pulse_width", "Pulse / double phase", "Oscillators", "Oscillator 3", Mapping::CC, 61, 0, 127, 64, 28, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc3_lfo2_pwm", "LFO2 pulse width", "Oscillators", "Oscillator 3", Mapping::CC, 62, 0, 127, 64, 29, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc3_env_pwm", "Envelope pulse width", "Oscillators", "Oscillator 3", Mapping::CC, 63, 0, 127, 64, 30, 127, 0, 127, 0, nullptr, 0, -64},
        {"osc_sync", "Osc1 to Osc2 sync", "Oscillators", "Sync / FM", Mapping::CC, 71, 0, 1, 0, 6, 64, 6, 64, 6, OffOn, 2, 0},
        {"fm_level", "FM level", "Oscillators", "Sync / FM", Mapping::NRPN, 0, 0, 127, 0, 56, 127, 0, 127, 0, nullptr, 0, 0},
        {"fm_env_depth", "FM envelope depth", "Oscillators", "Sync / FM", Mapping::NRPN, 1, 0, 127, 64, 57, 127, 0, 127, 0, nullptr, 0, -64},
        {"fm_velocity", "FM velocity", "Oscillators", "Sync / FM", Mapping::NRPN, 2, 0, 127, 64, 58, 127, 0, 127, 0, nullptr, 0, -64},
        {"fm_attack", "FM attack", "Oscillators", "Sync / FM", Mapping::NRPN, 3, 0, 127, 0, 59, 127, 0, 127, 0, nullptr, 0, 0},
        {"fm_decay", "FM decay", "Oscillators", "Sync / FM", Mapping::NRPN, 4, 0, 127, 60, 60, 127, 0, 127, 0, nullptr, 0, 0},
        {"osc1_level", "Oscillator 1", "Mixer", "Levels", Mapping::CC, 72, 0, 127, 127, 37, 127, 0, 127, 0, nullptr, 0, 0},
        {"osc2_level", "Oscillator 2", "Mixer", "Levels", Mapping::CC, 73, 0, 127, 0, 38, 127, 0, 127, 0, nullptr, 0, 0},
        {"osc3_level", "Oscillator 3", "Mixer", "Levels", Mapping::CC, 74, 0, 127, 0, 39, 127, 0, 127, 0, nullptr, 0, 0},
        {"noise_level", "Noise", "Mixer", "Levels", Mapping::CC, 75, 0, 127, 0, 40, 127, 0, 127, 0, nullptr, 0, 0},
        {"ring_level", "Ring 1 x 2", "Mixer", "Levels", Mapping::CC, 76, 0, 127, 0, 41, 127, 0, 127, 0, nullptr, 0, 0},
        {"external_level", "External input", "Mixer", "Levels", Mapping::CC, 77, 0, 127, 0, 42, 127, 0, 127, 0, nullptr, 0, 0},
        {"cutoff", "Cutoff", "Filter", "Low-pass", Mapping::CC, 105, 0, 127, 127, 46, 127, 0, 127, 0, nullptr, 0, 0},
        {"resonance", "Resonance", "Filter", "Low-pass", Mapping::CC, 106, 0, 127, 0, 44, 127, 0, 127, 0, nullptr, 0, 0},
        {"filter_drive", "Overdrive", "Filter", "Low-pass", Mapping::CC, 104, 0, 127, 0, 43, 127, 0, 127, 0, nullptr, 0, 0},
        {"filter_normalise", "Q normalise", "Filter", "Low-pass", Mapping::CC, 103, 0, 127, 64, 45, 127, 0, 127, 0, nullptr, 0, 0},
        {"filter_lfo2", "LFO2 depth", "Filter", "Low-pass", Mapping::CC, 102, 0, 127, 64, 51, 127, 0, 127, 0, nullptr, 0, -64},
        {"filter_env", "Envelope depth", "Filter", "Low-pass", Mapping::CC, 107, 0, 127, 64, 52, 127, 0, 127, 0, nullptr, 0, -64},
        {"filter_slope", "Slope", "Filter", "Low-pass", Mapping::CC, 67, 0, 1, 1, 0, 32, 5, 32, 5, FilterSlope, 2, 0},
        {"filter_keytrack", "Key tracking", "Filter", "Low-pass", Mapping::NRPN, 11, 0, 127, 127, 47, 127, 0, 127, 0, nullptr, 0, 0},
        {"amp_attack", "Attack", "Envelopes", "Amplitude", Mapping::CC, 108, 0, 127, 2, 62, 127, 0, 127, 0, nullptr, 0, 0},
        {"amp_decay", "Decay", "Envelopes", "Amplitude", Mapping::CC, 109, 0, 127, 90, 63, 127, 0, 127, 0, nullptr, 0, 0},
        {"amp_sustain", "Sustain", "Envelopes", "Amplitude", Mapping::CC, 110, 0, 127, 127, 64, 127, 0, 127, 0, nullptr, 0, 0},
        {"amp_release", "Release", "Envelopes", "Amplitude", Mapping::CC, 111, 0, 127, 40, 65, 127, 0, 127, 0, nullptr, 0, 0},
        {"amp_velocity", "Velocity depth", "Envelopes", "Amplitude", Mapping::CC, 112, 0, 127, 64, 61, 127, 0, 127, 0, nullptr, 0, -64},
        {"amp_trigger", "Trigger", "Envelopes", "Amplitude", Mapping::CC, 65, 0, 1, 1, 80, 1, 0, 1, 0, Trigger, 2, 0},
        {"mod_attack", "Attack", "Envelopes", "Modulation", Mapping::CC, 114, 0, 127, 2, 67, 127, 0, 127, 0, nullptr, 0, 0},
        {"mod_decay", "Decay", "Envelopes", "Modulation", Mapping::CC, 115, 0, 127, 65, 68, 127, 0, 127, 0, nullptr, 0, 0},
        {"mod_sustain", "Sustain", "Envelopes", "Modulation", Mapping::CC, 116, 0, 127, 0, 69, 127, 0, 127, 0, nullptr, 0, 0},
        {"mod_release", "Release", "Envelopes", "Modulation", Mapping::CC, 117, 0, 127, 65, 70, 127, 0, 127, 0, nullptr, 0, 0},
        {"mod_velocity", "Velocity depth", "Envelopes", "Modulation", Mapping::CC, 118, 0, 127, 64, 66, 127, 0, 127, 0, nullptr, 0, -64},
        {"mod_trigger", "Trigger", "Envelopes", "Modulation", Mapping::CC, 65, 0, 1, 1, 80, 2, 1, 2, 1, Trigger, 2, 0},
        {"fm_trigger", "FM trigger", "Envelopes", "FM", Mapping::CC, 65, 0, 1, 1, 80, 4, 2, 4, 2, Trigger, 2, 0},
        {"lfo1_rate", "Rate", "LFOs", "LFO 1", Mapping::CC, 80, 0, 127, 68, 72, 127, 0, 127, 0, nullptr, 0, 0},
        {"lfo1_sync", "Sync rate", "LFOs", "LFO 1", Mapping::CC, 81, 0, 34, 0, 73, 127, 0, 127, 0, Sync, 35, 0},
        {"lfo1_delay", "Delay", "LFOs", "LFO 1", Mapping::CC, 82, 0, 127, 50, 74, 127, 0, 127, 0, nullptr, 0, 0},
        {"lfo1_wave", "Waveform", "LFOs", "LFO 1", Mapping::CC, 78, 0, 3, 0, 78, 12, 2, 12, 2, LfoWave, 4, 0},
        {"lfo1_delay_multi", "Delay trigger", "LFOs", "LFO 1", Mapping::CC, 78, 0, 1, 0, 78, 1, 0, 1, 0, Trigger, 2, 0},
        {"lfo1_phase", "Keysync phase", "LFOs", "LFO 1", Mapping::CC, 79, 0, 1, 1, 79, 1, 0, 1, 0, Phase, 2, 0},
        {"lfo1_keysync", "Keysync", "LFOs", "LFO 1", Mapping::CC, 79, 0, 1, 0, 79, 2, 1, 2, 1, OffOn, 2, 0},
        {"lfo1_lock", "Phase lock", "LFOs", "LFO 1", Mapping::CC, 79, 0, 1, 0, 79, 4, 2, 4, 2, LfoLock, 2, 0},
        {"lfo2_rate", "Rate", "LFOs", "LFO 2", Mapping::CC, 83, 0, 127, 68, 75, 127, 0, 127, 0, nullptr, 0, 0},
        {"lfo2_sync", "Sync rate", "LFOs", "LFO 2", Mapping::CC, 84, 0, 34, 0, 76, 127, 0, 127, 0, Sync, 35, 0},
        {"lfo2_delay", "Delay", "LFOs", "LFO 2", Mapping::CC, 85, 0, 127, 50, 77, 127, 0, 127, 0, nullptr, 0, 0},
        {"lfo2_wave", "Waveform", "LFOs", "LFO 2", Mapping::CC, 78, 0, 3, 0, 78, 48, 4, 48, 4, LfoWave, 4, 0},
        {"lfo2_delay_multi", "Delay trigger", "LFOs", "LFO 2", Mapping::CC, 78, 0, 1, 0, 78, 2, 1, 2, 1, Trigger, 2, 0},
        {"lfo2_phase", "Keysync phase", "LFOs", "LFO 2", Mapping::CC, 79, 0, 1, 1, 79, 8, 3, 8, 3, Phase, 2, 0},
        {"lfo2_keysync", "Keysync", "LFOs", "LFO 2", Mapping::CC, 79, 0, 1, 0, 79, 16, 4, 16, 4, OffOn, 2, 0},
        {"lfo2_lock", "Phase lock", "LFOs", "LFO 2", Mapping::CC, 79, 0, 1, 0, 79, 32, 5, 32, 5, LfoLock, 2, 0},
        {"delay_level", "Send level", "Effects", "Delay", Mapping::CC, 92, 0, 127, 0, 100, 127, 0, 127, 0, nullptr, 0, 0},
        {"delay_time", "Time", "Effects", "Delay", Mapping::CC, 19, 0, 127, 64, 102, 127, 0, 127, 0, nullptr, 0, 0},
        {"delay_feedback", "Feedback", "Effects", "Delay", Mapping::CC, 21, 0, 127, 64, 104, 127, 0, 127, 0, nullptr, 0, 0},
        {"delay_width", "Stereo width", "Effects", "Delay", Mapping::CC, 22, 0, 127, 127, 105, 127, 0, 127, 0, nullptr, 0, 0},
        {"delay_ratio", "Left/right ratio", "Effects", "Delay", Mapping::CC, 23, 0, 12, 0, 106, 127, 0, 127, 0, DelayRatio, 13, 0},
        {"delay_wheel", "Mod wheel send", "Effects", "Delay", Mapping::CC, 18, 0, 127, 64, 101, 127, 0, 127, 0, nullptr, 0, 0},
        {"reverb_level", "Send level", "Effects", "Reverb", Mapping::CC, 91, 0, 127, 0, 107, 127, 0, 127, 0, nullptr, 0, 0},
        {"reverb_decay", "Decay", "Effects", "Reverb", Mapping::CC, 25, 0, 127, 90, 109, 127, 0, 127, 0, nullptr, 0, 0},
        {"reverb_wheel", "Mod wheel send", "Effects", "Reverb", Mapping::CC, 24, 0, 127, 64, 108, 127, 0, 127, 0, nullptr, 0, -64},
        {"chorus_level", "Send level", "Effects", "Chorus / Phaser", Mapping::CC, 93, 0, 127, 0, 110, 127, 0, 127, 0, nullptr, 0, 0},
        {"chorus_rate", "Rate", "Effects", "Chorus / Phaser", Mapping::CC, 27, 0, 127, 20, 112, 127, 0, 127, 0, nullptr, 0, 0},
        {"chorus_feedback", "Feedback", "Effects", "Chorus / Phaser", Mapping::CC, 29, 0, 127, 74, 114, 127, 0, 127, 0, nullptr, 0, -64},
        {"chorus_depth", "Depth", "Effects", "Chorus / Phaser", Mapping::CC, 30, 0, 127, 64, 115, 127, 0, 127, 0, nullptr, 0, 0},
        {"chorus_centre", "Centre", "Effects", "Chorus / Phaser", Mapping::CC, 31, 0, 127, 64, 116, 127, 0, 127, 0, nullptr, 0, 0},
        {"chorus_wheel", "Mod wheel send", "Effects", "Chorus / Phaser", Mapping::CC, 26, 0, 127, 64, 111, 127, 0, 127, 0, nullptr, 0, -64},
        {"distortion_level", "Drive", "Effects", "Distortion", Mapping::CC, 90, 0, 127, 0, 97, 127, 0, 127, 0, nullptr, 0, 0},
        {"distortion_compensation", "Compensation", "Effects", "Distortion", Mapping::CC, 17, 0, 127, 100, 99, 127, 0, 127, 0, nullptr, 0, 0},
        {"distortion_wheel", "Mod wheel depth", "Effects", "Distortion", Mapping::CC, 16, 0, 127, 64, 98, 127, 0, 127, 0, nullptr, 0, -64},
        {"eq_level", "EQ shape", "Effects", "EQ", Mapping::CC, 33, 0, 127, 64, 92, 127, 0, 127, 0, nullptr, 0, -64},
        {"eq_frequency", "Frequency", "Effects", "EQ", Mapping::CC, 34, 0, 127, 64, 93, 127, 0, 127, 0, nullptr, 0, 0},
        {"eq_rate", "Rate", "Effects", "EQ", Mapping::CC, 35, 0, 127, 10, 94, 127, 0, 127, 0, nullptr, 0, 0},
        {"eq_depth", "Depth", "Effects", "EQ", Mapping::CC, 37, 0, 127, 0, 96, 127, 0, 127, 0, nullptr, 0, 0},
        {"pan_position", "Position", "Effects", "Panning", Mapping::CC, 10, 0, 127, 64, 117, 127, 0, 127, 0, nullptr, 0, -64},
        {"pan_rate", "Rate", "Effects", "Panning", Mapping::CC, 12, 0, 127, 20, 118, 127, 0, 127, 0, nullptr, 0, 0},
        {"pan_depth", "Depth", "Effects", "Panning", Mapping::CC, 94, 0, 127, 0, 120, 127, 0, 127, 0, nullptr, 0, 0},
        {"vocoder_balance", "Balance", "Effects", "Vocoder", Mapping::CC, 95, 0, 127, 0, 89, 127, 0, 127, 0, nullptr, 0, 0},
        {"vocoder_width", "Stereo width", "Effects", "Vocoder", Mapping::CC, 14, 0, 127, 127, 90, 127, 0, 127, 0, nullptr, 0, 0},
        {"vocoder_sibilance", "Sibilance level", "Effects", "Vocoder", Mapping::CC, 15, 0, 127, 64, 91, 127, 0, 127, 0, nullptr, 0, 0},
        {"delay_sync", "Sync time", "Effects", "Delay", Mapping::CC, 20, 0, 19, 0, 103, 127, 0, 127, 0, Sync, 20, 0},
        {"chorus_sync", "Sync rate", "Effects", "Chorus / Phaser", Mapping::CC, 28, 0, 34, 0, 113, 127, 0, 127, 0, Sync, 35, 0},
        {"eq_sync", "Sync rate", "Effects", "EQ", Mapping::CC, 36, 0, 34, 0, 95, 127, 0, 127, 0, Sync, 35, 0},
        {"pan_sync", "Sync rate", "Effects", "Panning", Mapping::CC, 13, 0, 34, 0, 119, 127, 0, 127, 0, Sync, 35, 0},
        {"reverb_type", "Type", "Effects", "Reverb", Mapping::NRPN, 21, 0, 5, 4, 121, 7, 0, 7, 0, ReverbType, 6, 0},
        {"chorus_type", "Type", "Effects", "Chorus / Phaser", Mapping::NRPN, 21, 0, 1, 0, 121, 24, 3, 24, 3, ChorusType, 2, 0},
        {"chorus_start", "Sync start", "Effects", "Chorus / Phaser", Mapping::NRPN, 22, 0, 3, 0, 122, 3, 0, 3, 0, SyncPan, 4, 0},
        {"pan_start", "Sync start", "Effects", "Panning", Mapping::NRPN, 22, 0, 3, 0, 122, 12, 2, 12, 2, SyncPan, 4, 0},
        {"eq_start", "Sync start", "Effects", "EQ", Mapping::NRPN, 22, 0, 3, 0, 122, 48, 4, 48, 4, SyncEq, 4, 0},
        {"vocoder_sibilance_type", "Sibilance type", "Effects", "Vocoder", Mapping::NRPN, 23, 0, 1, 0, 123, 8, 3, 8, 3, Sibilance, 2, 0},
        {"external_trigger", "Envelope trigger", "Effects", "External input", Mapping::NRPN, 23, 0, 1, 0, 123, 32, 5, 32, 5, OffOn, 2, 0},
        {"external_to_fx", "Direct to effects", "Effects", "External input", Mapping::NRPN, 23, 0, 1, 0, 123, 64, 6, 64, 6, OffOn, 2, 0},
        {"arp_pattern", "Pattern", "Arpeggiator", "Pattern", Mapping::CC, 3, 0, 5, 0, 87, 127, 0, 127, 0, ArpPattern, 6, 0},
        {"arp_rate", "Tempo", "Arpeggiator", "Pattern", Mapping::CC, 9, 0, 127, 56, 84, 127, 0, 127, 0, nullptr, 0, 64},
        {"arp_sync", "Sync rate", "Arpeggiator", "Pattern", Mapping::CC, 87, 0, 15, 3, 85, 127, 0, 127, 0, ArpSync, 16, 0},
        {"arp_gate", "Gate time", "Arpeggiator", "Pattern", Mapping::CC, 88, 0, 127, 64, 86, 127, 0, 127, 0, nullptr, 0, 0},
        {"arp_octaves", "Octaves", "Arpeggiator", "Pattern", Mapping::CC, 89, 0, 3, 0, 88, 3, 0, 3, 0, ArpOctave, 4, 0},
        {"arp_on", "Enable", "Arpeggiator", "Pattern", Mapping::CC, 89, 0, 1, 0, 88, 4, 2, 4, 2, OffOn, 2, 0},
        {"arp_keysync", "Keysync", "Arpeggiator", "Pattern", Mapping::CC, 89, 0, 1, 0, 88, 8, 3, 8, 3, OffOn, 2, 0},
        {"arp_latch", "Latch", "Arpeggiator", "Pattern", Mapping::CC, 89, 0, 1, 0, 88, 16, 4, 16, 4, OffOn, 2, 0},
        {"wheel_osc_pitch", "Oscillator pitch", "Modulation", "Mod wheel", Mapping::NRPN, 5, 0, 127, 64, 31, 127, 0, 127, 0, nullptr, 0, -64},
        {"wheel_osc_lfo1", "Oscillator LFO1", "Modulation", "Mod wheel", Mapping::NRPN, 8, 0, 127, 73, 34, 127, 0, 127, 0, nullptr, 0, -64},
        {"wheel_filter_frequency", "Filter frequency", "Modulation", "Mod wheel", Mapping::NRPN, 12, 0, 127, 64, 48, 127, 0, 127, 0, nullptr, 0, -64},
        {"wheel_filter_lfo2", "Filter LFO2", "Modulation", "Mod wheel", Mapping::NRPN, 15, 0, 127, 64, 53, 127, 0, 127, 0, nullptr, 0, -64},
        {"wheel_amplitude", "Amplitude", "Modulation", "Mod wheel", Mapping::NRPN, 18, 0, 127, 64, 81, 127, 0, 127, 0, nullptr, 0, -64},
        {"aftertouch_osc_pitch", "Oscillator pitch", "Modulation", "Aftertouch", Mapping::NRPN, 6, 0, 127, 64, 32, 127, 0, 127, 0, nullptr, 0, -64},
        {"aftertouch_osc_lfo1", "Oscillator LFO1", "Modulation", "Aftertouch", Mapping::NRPN, 9, 0, 127, 64, 35, 127, 0, 127, 0, nullptr, 0, -64},
        {"aftertouch_filter_frequency", "Filter frequency", "Modulation", "Aftertouch", Mapping::NRPN, 13, 0, 127, 64, 49, 127, 0, 127, 0, nullptr, 0, -64},
        {"aftertouch_filter_lfo2", "Filter LFO2", "Modulation", "Aftertouch", Mapping::NRPN, 16, 0, 127, 64, 54, 127, 0, 127, 0, nullptr, 0, -64},
        {"aftertouch_amplitude", "Amplitude", "Modulation", "Aftertouch", Mapping::NRPN, 19, 0, 127, 64, 82, 127, 0, 127, 0, nullptr, 0, -64},
        {"breath_osc_pitch", "Oscillator pitch", "Modulation", "Breath", Mapping::NRPN, 7, 0, 127, 64, 33, 127, 0, 127, 0, nullptr, 0, -64},
        {"breath_osc_lfo1", "Oscillator LFO1", "Modulation", "Breath", Mapping::NRPN, 10, 0, 127, 64, 36, 127, 0, 127, 0, nullptr, 0, -64},
        {"breath_filter_frequency", "Filter frequency", "Modulation", "Breath", Mapping::NRPN, 14, 0, 127, 64, 50, 127, 0, 127, 0, nullptr, 0, -64},
        {"breath_filter_lfo2", "Filter LFO2", "Modulation", "Breath", Mapping::NRPN, 17, 0, 127, 64, 55, 127, 0, 127, 0, nullptr, 0, -64},
        {"breath_amplitude", "Amplitude", "Modulation", "Breath", Mapping::NRPN, 20, 0, 127, 64, 83, 127, 0, 127, 0, nullptr, 0, -64},
    }};
    constexpr uint8_t decode(const ControlSpec& spec, const uint8_t* packed128)
    {
        return spec.patchOffset < 0 ? spec.defaultValue :
            static_cast<uint8_t>((packed128[spec.patchOffset] & spec.patchMask) >> spec.patchShift);
    }
    constexpr uint8_t encode(const ControlSpec& spec, uint8_t oldWire, uint8_t logicalValue)
    {
        return static_cast<uint8_t>((oldWire & ~spec.wireMask) | ((logicalValue << spec.wireShift) & spec.wireMask));
    }
}
