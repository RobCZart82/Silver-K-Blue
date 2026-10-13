# Silver-K 0.2 Alpha

An early alpha release for continued development. Patch management, CPU performance, latency and optimization are still being refined.

[Download 0.2 Alpha](https://github.com/temaniak/Silver-K/releases/tag/v0.2.0-alpha.1). Installation and first-run instructions are provided below in English. See the [detailed installation guide](doc/INSTALL.md) and [release notes](doc/releases/0.2.0-alpha.1.md).

Silver-K combines **K-Station** and **KS** firmware emulation in one desktop synthesizer. The selected firmware determines the engine, available controls, parts and automatic skin. The build supports **Windows x64 Standalone/VST3** and **macOS Standalone/VST3/AU**. Windows builds, DSP JIT tests and ROM-free checks run in GitHub Actions; macOS arm64 is also verified locally.

- K-Station: one part, stereo output; tabs 2–4 are dimmed.
- KS: four parts and four output channels; routing and MIDI settings on **Perf**.
- Shared patch browser: recursive folders, tags, favourites, source model and compatibility reports.
- K-Station ↔ KS patch conversion. KS-only oscillator waves become sine on K-Station, filters become LP 12, and unsupported features use target defaults.
- Automatic KS grey lower panel, built-in skins and custom JSON skins.

A-Station patches can be identified and stored; A-Station conversion and verified X-Station import are not implemented yet. KS external audio input is not connected.

## Release packages

| Platform | Included formats |
| --- | --- |
| [Windows x64](https://github.com/temaniak/Silver-K/releases/download/v0.2.0-alpha.1/Silver-K-0.2.0-alpha.1-Windows-x64.zip) | Standalone, VST3 |
| [macOS Apple Silicon / arm64](https://github.com/temaniak/Silver-K/releases/download/v0.2.0-alpha.1/Silver-K-0.2.0-alpha.1-macOS-arm64.zip) | Standalone, VST3, AU |

The Mac package is an arm64 build. Use an Apple Silicon Mac and a native arm64 DAW; Intel and Universal builds are not included in this release.

Each archive includes our test KS Data, a zero-filled Wave ROM substitute, installation instructions, licenses, source links and SHA-256 checksums. Supported original firmware must be supplied separately.

## Installation

### macOS

1. Extract the ZIP and close your DAW before replacing an existing plugin.
2. Copy `Standalone/Silver-K.app` to `/Applications` or your preferred applications folder.
3. Copy the entire `Plugins/VST3/Silver-K.vst3` bundle to `~/Library/Audio/Plug-Ins/VST3/`.
4. Copy the entire `Plugins/AU/Silver-K.component` bundle to `~/Library/Audio/Plug-Ins/Components/`.
5. Open these locations in Finder using **Go → Go to Folder…** (`Command+Shift+G`). Create the format folder if necessary.
6. Restart your DAW and rescan plugins. Silver-K is an instrument; load it on an instrument track. In Logic, use Plugin Manager to rescan the AU if needed.

Keep each `.app`, `.vst3` or `.component` bundle intact. When updating, replace the old bundle as a whole rather than copying its internal executable.

These Mac builds are ad hoc signed and are not Apple-notarized or Developer ID signed. If macOS blocks the application, follow its per-application **Privacy & Security → Open Anyway** process after checking the download source. [Apple's instructions](https://support.apple.com/102445).

### Windows

1. Extract the ZIP to a permanent folder and close your DAW before updating.
2. Run `Standalone/Silver-K.exe` from the extracted folder.
3. Copy the entire `Plugins\VST3\Silver-K.vst3` folder to `C:\Program Files\Common Files\VST3\`. Windows may request administrator permission for this copy.
4. Restart your 64-bit DAW and rescan VST3 plugins. Load Silver-K on an instrument track.
5. If Windows reports a missing `VCRUNTIME140`, `MSVCP140` or related runtime library, install the official [Microsoft Visual C++ v14 Redistributable x64](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).

Keep the `.vst3` folder intact; do not extract its internal binary. AU is available only on macOS. VST2 is not included.

## Firmware and data locations

Store your firmware and data in a permanent user folder, for example:

```text
Documents/Silver-K/
├── ROMs/       Your supported K-Station and KS firmware
├── KS-Data/    Test KS Data, zero Wave ROM or your own compatible data
└── Presets/    Factory and user patch banks
```

Copy the release archive's `KS-Data` folder there. Keep your original firmware in `ROMs` and patch banks in `Presets`. Select these files through Silver-K's **Setup** page; they do not need to be placed inside plugin bundles or system plugin folders.

Supported firmware versions are **K-Station OS 2.1.01** and **KS OS 2.1.02**. In **Setup → ROM folder**, select your firmware folder, then choose a firmware from the top selector. Alternatively, use **Load firmware** to select a supported file. OS/SYX updates, decoded BIN files and ZIP archives containing supported firmware are accepted. The loader checks the contents, not just the filename.

Firmware and Wave ROM contents are not embedded in DAW project state. Keep the selected files available and select them again if you move them. Official KS OS and factory-sound downloads are available from [Novation](https://downloads.novationmusic.com/novation/synthesisers-legacy/ks-rack).

## KS Data and Wave ROM

These two files have different purposes:

| File | Size | Purpose | Setup control |
| --- | --- | --- | --- |
| `ks-init-flash.bin` | 524288 bytes | Our test KS Data: simple `Silver Init` programs, four-part `Silver Multi` performances and startup settings | **KS factory data** |
| `ks-wave-zero.bin` | 129024 bytes | A zero-filled substitute for the missing sampled-wave bank | **Wave ROM** |

Neither file contains original OS code, DSP firmware, Novation factory patches or sampled waves. They are not firmware updates and cannot be loaded through **Load firmware**.

### First KS startup with the supplied test files

1. Select `ks-init-flash.bin` through **Setup → KS factory data**.
2. Select `ks-wave-zero.bin` through **Setup → Wave ROM**.
3. Select your own KS OS 2.1.02 through **ROM folder / Load firmware** and wait for `Loading firmware...` to finish.
4. In Standalone, select an audio output and MIDI input. In a DAW, enable monitoring on the instrument track and send MIDI to Silver-K.
5. Play the initial `Silver Init` saw program using the selected MIDI channel. For four parts, enable **Perf → Performance (4 parts)** and check each part's active state, channel, keyboard range and output routing.

The zero Wave ROM allows testing generated oscillator waves. Sounds that depend on sampled waves or drums may be silent or incomplete; the substitute does not recreate those original sounds. Use your own compatible 129024-byte Wave ROM through **Setup → Wave ROM** when available.

**Program boost (dB)** is a separate KS gain control ranging from 0 to 30 dB. At 0 dB, the program uses its normal unboosted level. Use **Part volume** on Perf for the part's 0–127 level. High program boost can cause distortion.

Editor changes and automation address individual KS parts, including tuning and volume, even when their MIDI channels are shared. External MIDI notes and controllers still address all matching parts so layered playing is preserved.

### Original KS factory Data

You can replace the test Init Flash with your own compatible full KS Data:

- Use **Setup → KS factory data** to select a `.mid` / `.midi` file containing KS Program/Performance SysEx, or a compatible full 524288-byte user Flash `.bin`.
- A normal MIDI song, a firmware update, a Wave ROM or a single program dump is not a full KS user Flash.
- Selecting new Data while KS is running reloads the engine and interrupts playback. Change these files between playback sessions.
- Factory Data and Wave ROM are independent: loading factory programs does not provide the samples they may need.

Loaded Data is represented by the `KS Factory` browser source. With the test Init Flash, that source contains our own initial programs, not original factory sounds. If changing Data leaves an old browser source visible, add the desired bank separately through **Patches → Add file / Add folder**.

## Factory presets and patch banks

1. Store your banks in a permanent `Presets` folder. The browser accepts supported `.syx`, `.mid`, `.midi` files and ZIP archives containing them.
2. Open **Patches → Add folder** to scan a folder and its subfolders, or **Add file** to add an individual bank. Use **Rescan** if necessary and check the source/model/tag filters.
3. Select a patch and click **Load**, or double-click its row.
4. In KS Performance mode, select the destination part using tabs 1–4 before loading a Program. A Performance contains the full four-part setup. Some preset-loading paths reload the Performance and interrupt sound.
5. Use **Save current** for the current sound, **Export .syx** for a selected library entry, and **Save tags** after editing tags or favourites.

K-Station uses its own firmware and does not require KS Init Flash or KS Wave ROM. Add its factory patches through Patches in the same way. Cross-model conversion can replace unsupported functions; review the compatibility report when loading a converted patch.

## Alpha limitations

- **Patch management is unfinished.** Bank import, organization, tags, compatibility and saving workflows are still being refined. Keep your original banks and verify saved patches by reloading them.
- **Performance and optimization are unfinished.** CPU load, loading/warm-up and multitimbral operation at small audio buffers need further work.
- **Latency needs further improvement.** Setup's Host latency reports plugin latency; audio-driver, physical MIDI and DAW-chain delays are additional. Increasing the audio buffer can reduce dropouts but increases overall latency.
- KS external audio input, A-Station conversion and verified X-Station import are not implemented. Sound accuracy against physical instruments has not been established.

This alpha is a starting point for continued plugin development.

## Source-only repository

This repository contains application and required framework source, skin assets, documentation, and **one synthetic user flash**: [`assets/ks-init-flash.bin`](assets/README.md). That file is generated by us from init settings; it contains no OS/DSP firmware, sampled waves or factory patches.

**No builds, plugin bundles, test WAVs, ROMs or factory banks are committed.** Firmware and optional wave ROMs must be supplied locally by the user. Do not place them in Git. The `.gitignore` and CI audit reject artifacts; the init flash is the only allowed `.bin`.

Release archives include our synthetic KS Init Flash (test KS Data) and a separately generated 129024-byte zero Wave ROM. Neither contains original firmware, factory sounds or sampled waves. The user's supported OS is still required; sample-dependent sounds remain silent/incomplete with the zero bank.

## Build

Requirements for the verified platform: macOS arm64, Xcode command-line tools, CMake 3.22+, Python 3, and internet access for the first configuration. Dependencies are fetched from pinned source revisions and compiled locally; prebuilt libraries are not required.

```sh
git clone https://github.com/temaniak/Silver-K.git
cd Silver-K
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --target silverK_Standalone silverK_VST3 silverK_AU silver_check -j 6
SILVER_K_DATA=/tmp/silver-k-tests ctest --test-dir build --output-on-failure -R '^silver_conversion$'
python3 tools/check_repository.py
```

Products are under `build/source/novation/silverK/buildStandalone/silverPlugin/silverK_artefacts/Release/`. macOS bundles receive an ad hoc signature for local testing. Signing for distribution is a separate step. Deployment target 12.0 is not a claim that all older macOS releases have been tested.

### Windows x64

Install Visual Studio 2022 or later with **Desktop development with C++**, a Windows SDK, CMake 3.22+, and Python 3. In PowerShell:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target silverK_Standalone silverK_VST3 silver_check --parallel 4
$env:SILVER_K_DATA = "$env:TEMP/silver-k-tests"
ctest --test-dir build -C Release --output-on-failure -R '^silver_conversion$'
python tools/check_repository.py
```

Standalone provides JUCE's Windows audio backends (WASAPI/DirectSound); an optional locally supplied ASIO SDK can be enabled with `-DSilverK_ASIO_SDK_PATH=C:/path/to/asiosdk`. The SDK is not redistributed. The core and firmware/patch logic are shared with macOS; AU and codesigning are conditional on macOS.

For offline builds, pass `-DJUCE_SOURCE=/path/to/JUCE -DDSP_SOURCE=/path/to/dsp56300 -DASMJIT_SOURCE=/path/to/asmjit` using the exact revisions in [`cmake/Dependencies.cmake`](cmake/Dependencies.cmake). Those must be **source checkouts**, not binary releases. The macOS JUCE compatibility patch applies to a build-local copy only.

## Run

In **Setup**, select your local ROM folder and firmware. For KS, select the supplied `assets/ks-init-flash.bin` in the KS data field. Optional external wave banks remain local; without them sample-dependent oscillators are silenced through original MIDI commands. Add your own patch folders in **Patches**. Use **Perf** for KS multi mode and output routing.

Details and current limitations: [user guide](doc/silver_k.md). Provenance and licenses: [THIRD_PARTY.md](THIRD_PARTY.md).
