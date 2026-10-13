# Silver-K 0.2 Alpha

A combined Windows x64 Standalone/VST3 and macOS arm64 Standalone/VST3/AU instrument for original K-Station OS 2.1.01 and KS OS 2.1.02 firmware. The project was extracted from ungeermoolator and uses Gearmulator components. See the [installation guide](INSTALL.md) for platform installation and ROM/Data setup.

The original H8/DSP engines run internally. Firmware is validated by contents and SHA-256, not filename. The integration uses Gearmulator patchDB for folder scanning, caching, tags, favourites and metadata changes. K-Station retains the Gearmulator/libresample rate converter; KS retains its four-channel stream. ROM code is not embedded in the application or DAW project state.

## Getting started

1. On Setup, select **ROM folder**. Subfolders are scanned; decoded BIN, OS/SYX and firmware ZIP updates are supported. Select firmware in the menu at the top.
2. For KS, select our `assets/ks-init-flash.bin` (524288 bytes) under **KS factory data**. It contains only Silver-K initial sounds. Alternatively, supply an official All Data MIDI file locally. A separate Wave ROM can be selected (129024 bytes); without one, the engine uses an in-memory zero bank. For a zero bank, ROM-dependent fields are silenced in a working Flash copy before boot; subsequent edits are adapted through original MIDI commands. Original levels remain in snapshots and saved state. This adaptation is disabled with a real bank.
3. On Patches, add a folder, SYX, MID or ZIP. Files and subfolders are scanned in the background. Double-click or **Load** loads a patch into the selected part. Tags and favourites are stored separately from source files.

On macOS, the library is stored in `~/Library/Application Support/The Usual Suspects/Silver-K/patches`. Isolated checks can use `SILVER_K_DATA` to select a separate database; this does not supply factory ROMs.

## Interface

The 1280 × 752 layout follows the existing Silver-K K-Station panel. OSC/Mixer/Filter/LFO/Env and the lower effects sections are retained. The top pages are Main, Extra, Perf, Patches and Setup.

Tabs 1–4 select a part. K-Station exposes only the first part; the remaining tabs are dimmed. For four KS parts, enable Performance on Perf. This page controls part activity, MIDI channel, keyboard range, transpose, level and Main 1+2 / Aux 3+4 / Mono 1–4 output routing. Switching tabs does not itself stop notes on other parts.

KS editor changes and automation address individual parts, including tuning and Part volume, even when parts share a MIDI channel. Incoming MIDI notes and controllers still affect all matching parts for layered playing. User MIDI channels are preserved in Performance and project state; internal engine channels are hidden. Level and tuning edits do not reload the Performance or interrupt held notes.

KS **Program boost (dB)** in Voice / Performance provides 0–30 dB of gain, matching the firmware's ENV ProgBoost menu. At 0 dB, the program uses its normal unboosted level. This is separate from Part volume (0–127). The erroneous boost range of 0–127 has been corrected; host ID `voice_level` is preserved, but normalized automation now maps to 0–30. Legacy program values above 30 reset to 0 dB; valid values are preserved. High boost with loud oscillators or polyphony can still overload the signal path. See the [KS Rack manual, pages 39 and 63](https://fael-downloads-prod.focusrite.com/customer/dev/s3fs-public/novation/downloads/10720/english102.pdf).

Auto mode uses the original dark lower panel for K-Station and a grey panel with dark labels for KS. Setup offers Silver-K, KS Grey, Classic and custom JSON skins based on the supplied skin. The template has a fixed 1280 × 752 size.

VST3/AU expose two stereo buses; the host can disable Aux. Standalone supports two or four physical output channels. K-Station outputs 3–4 are silent. KS routing preserves its original MIDI parameters and four-part engine.

## Patch compatibility

The browser shows the SysEx source model, Program/Performance type, source, tags and compatibility status. Original data stays in the database; conversion operates on a copy during loading.

KS → K-Station:

- Extra oscillator waves, including noise variants, are replaced with sine.
- Every filter becomes LP 12 dB/oct, including an original LP 24 filter.
- Extra LFO and arpeggiator shapes use K-Station defaults.
- KS-only parameters, such as level modulation, extended LFO settings and part routing, are not transferred. The report lists lost settings that differ from defaults.
- A Performance transfers only its first part. K-Station cannot reproduce four parts.

K-Station → KS preserves the shared block, unpacks wave fields and adapts additional-field encoding. KS settings start at defaults. In KS Performance mode, the selected part's sound is replaced while its MIDI channel, output and keyboard range are preserved.

A native KS Performance loads the complete setup and enables Performance mode. Native patches preserve their sound payload. Headers are normalized for loading into the current sound, without writing to the synthesizer's Flash. Losses produce a warning and a replacement report.

This version plays and converts K-Station/KS patches. A-Station SYX can be recognized and stored with its model, but conversion remains disabled pending a verified mapping. X-Station import is not verified.

## State and limitations

Project state stores the selected model, ROM paths and hashes, current native sound/Performance, user data, selected part, skin and parameters. Tags and folders live in a separate persistent database. Changing firmware cold-reloads the engine and interrupts sound. UI firmware selection loads and warms up the DSP in the background while showing `Loading firmware`; controls are temporarily unavailable and the host window continues processing events. The previous separate K-Station/KS products and plugin IDs are retained; Silver-K has its own ID, `Tskv`.

Both models have persistent host parameter IDs: KS uses the existing names and `part2_`/`part3_`/`part4_` prefixes; K-Station uses `k_`. The editor exposes the selected model's controls, while the host sees a stable complete parameter set so automation keeps its meaning after ROM selection.

K-Station retains a separate optional mono Vocoder/external input and the shared Gearmulator input converter. KS external audio input is not connected. Sound accuracy against hardware has not been established by this integration. The KS zero-bank limitations still apply.

Patch management, CPU performance, latency and optimization remain unfinished. Keep original banks and verify saved patches by reloading them. Some preset-loading paths reload the Performance and interrupt sound.

Builds, local ROMs, test WAVs and validation states remain outside tracked source. Win64 is built in CI from the same source. Local audio validation was performed on modern macOS arm64; a deployment target of macOS 12 does not establish compatibility with all older releases, because dependencies are built with the current SDK.

## Validation

`silver_check` checks extra-wave/LP 12/arpeggiator conversion, native payload preservation, folder import, model detection, tags/favourites, a shared instance database and required skin controls. With local ROMs, additional checks cover audio, live cutoff on held notes, silent K-Station Aux, both models' state restoration, repeated `prepareToPlay` after K-Station state, and full KS Performance. Checked callbacks had no C++ allocations through `operator new`; this does not separately intercept every C `malloc`.

KST100 conversion was compared with original KS UART import: all 151 synthesizer bytes matched for each of the 16 source keysync values. This validates that patch and field variants, rather than sound identity between physical instruments.

Earlier hosted VST3/AU checks used 48 kHz, 256-sample buffers and four seconds of real-time playback. KS output peaks were 0.0359975 / 0.0359963 / 0.0474905 / 0.0475713 with zero underruns. K-Station stereo peak was 0.105694 with zero underruns. Maximum callbacks on the test machine were: VST3 KS 0.113 ms, AU KS 0.065 ms, VST3 K-Station 0.192 ms and AU K-Station 0.159 ms. These checks exclude physical MIDI/audio-driver latency. See the [release notes](releases/0.2.0-alpha.1.md) for final release validation.
