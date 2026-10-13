# Silver-K 0.2 Alpha — installation and first startup

Silver-K combines K-Station and four-part KS emulation. This early alpha still needs work on patch management, CPU performance, latency and optimization.

## 1. Archive contents

| File or folder | Purpose |
| --- | --- |
| `Standalone/Silver-K.app` or `Standalone/Silver-K.exe` | Standalone application |
| `Plugins/VST3/Silver-K.vst3` | VST3 instrument |
| `Plugins/AU/Silver-K.component` | AU instrument, macOS only |
| `KS-Data/ks-wave-zero.bin` | Zero-filled Wave ROM substitute: 129024 bytes, no samples |
| `KS-Data/ks-init-flash.bin` | Our test KS Data: 524288 bytes containing basic programs, performances and startup settings |
| `KS-Data/*.manifest.json` | Test-file sizes, provenance and SHA-256 hashes |
| `INSTALL.md` | This guide |
| `RELEASE_NOTES.md` | Release description and limitations |
| `release-manifest.json`, `SHA256SUMS.txt` | Release contents and checksums |
| `LICENSE.md`, `THIRD_PARTY.md`, `Licenses/`, `SOURCES.txt` | Licenses and corresponding source links |

**Original OS firmware, factory program banks and sampled Wave ROMs are not included.** Supply these separately from your own local files. Our zero Wave ROM and KS Init Flash contain no Novation code, factory presets or sampled waves.

## 2. macOS installation

This archive targets **Apple Silicon (arm64)**. Run your DAW natively for Apple Silicon. The plugins are not Intel/x86_64 or Universal builds.

1. Extract the entire ZIP and close your DAW before replacing plugins.
2. Move `Standalone/Silver-K.app` to `/Applications` or your own applications folder.
3. Move the **entire bundle** `Plugins/VST3/Silver-K.vst3` to:
   ```text
   ~/Library/Audio/Plug-Ins/VST3/
   ```
4. Move the **entire bundle** `Plugins/AU/Silver-K.component` to:
   ```text
   ~/Library/Audio/Plug-Ins/Components/
   ```
5. In Finder, use **Go → Go to Folder…** (`Command+Shift+G`) and paste the path, including `~`. Create the destination folder if it does not exist.
6. When updating, replace the old plugin bundle completely. Keep a separate backup if you want to return to the previous version. Do not extract the executable from `Contents/MacOS`.
7. Start your DAW and rescan plugins. In Logic, open Plug-in Manager and repeat AU validation if needed. Silver-K is an instrument; look in the synthesizer list.

Mac bundles are ad hoc signed, **without Developer ID signing or Apple notarization**. If macOS blocks the application, verify the archive's source and use **System Settings → Privacy & Security → Open Anyway** after attempting to launch it. Do not disable Gatekeeper system-wide. A plugin may also need the normal system approval and another DAW scan. See [Apple's instructions](https://support.apple.com/102445).

Optional Terminal validation of the installed AU:

```sh
auval -v aumu Tskv TusP
```

## 3. Windows installation

This archive targets **Windows x64** and a 64-bit DAW.

1. Extract the entire ZIP. Close your DAW before updating.
2. Keep Standalone in a permanent folder, such as `Documents\Silver-K\Standalone\Silver-K.exe`. Do not run it from inside the ZIP.
3. Move the **entire folder** `Plugins\VST3\Silver-K.vst3` to the standard VST3 directory:
   ```text
   C:\Program Files\Common Files\VST3\
   ```
   Windows may request administrator access. Do not copy just the DLL from inside the `.vst3` folder.
4. When updating, replace the entire old `Silver-K.vst3` folder and rescan VST3 plugins in your DAW.
5. If Windows reports a missing `VCRUNTIME140`, `MSVCP140` or another Visual C++ library, install the official **Microsoft Visual C++ v14 Redistributable x64** from [Microsoft](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).
6. In Standalone, open the audio-device settings and select your audio output and MIDI input. This release uses JUCE's standard Windows backends; an optional locally supplied ASIO SDK is not included.

AU is available only on macOS. This release does not include VST2.

## 4. Where to keep ROM, KS Data and factory presets

Choose a permanent user folder, for example:

```text
Documents
└── Silver-K
    ├── ROMs
    │   ├── your K-Station firmware
    │   └── your KS firmware
    ├── KS-Data
    │   ├── ks-init-flash.bin
    │   ├── ks-wave-zero.bin
    │   └── your original KS Data / Wave ROM, if available
    └── Presets
        ├── K-Station
        └── KS
```

Copy `KS-Data` from the archive into this folder. Put firmware in `ROMs` and sound banks in `Presets`. Avoid temporary download folders that you may delete later. ROM files do not need to go into the system VST3/AU directories or inside `.app`, `.component` or `.vst3` bundles: select their paths in Setup.

Silver-K remembers paths and validates supported firmware contents. OS code and Wave ROM are not embedded in DAW project state. If you move the files, select them again. Moving a project to another computer requires the same local files there.

## 5. OS firmware is a separate required file

Supported versions are **K-Station OS 2.1.01** and **KS OS 2.1.02**. An archive name may differ from its internal version; validation uses contents and checksums.

1. Put your firmware in `ROMs`.
2. Open **Setup → ROM folder** and select that folder. Subfolders are scanned.
3. Supported formats include the corresponding OS/SYX updates, decoded BIN files and ZIP archives containing supported firmware. You can also use **Load firmware** to select a file. A factory-sound ZIP is not a firmware ZIP.
4. For KS, configure KS Data as described below, then select KS in the firmware menu at the top. Wait for `Loading firmware...` to finish.

Do not load `ks-wave-zero.bin` or `ks-init-flash.bin` through **Load firmware**: neither contains an OS. The test files alone cannot start the synthesizer without supported firmware.

Official KS OS and factory-sound downloads: [Novation KS Rack](https://downloads.novationmusic.com/novation/synthesisers-legacy/ks-rack). Original downloaded files remain local to the user and are not bundled in this release.

## 6. First KS startup with our test data

Factory programs are not required for a basic test: KS Init Flash contains our own simple sounds. It initializes user programs, performances and startup settings. It is **KS Data, not Wave ROM**.

1. Copy both files from the archive's `KS-Data` folder into your permanent folder.
2. Under **Setup → KS factory data**, select `ks-init-flash.bin`, **524288 bytes**.
3. Under **Setup → Wave ROM**, select `ks-wave-zero.bin`, **129024 bytes**.
4. Use **ROM folder** or **Load firmware** to supply KS OS 2.1.02 and select it in the firmware menu.
5. Wait for loading to finish. Check the output, audio device and MIDI channel. Single mode uses the global/editor MIDI channel selected in Setup.
6. The initial `Silver Init` program uses a generated saw wave. Play a note. In Standalone, select your MIDI input; in a DAW, create an instrument track, enable monitoring and send MIDI to Silver-K.
7. For four parts, open **Perf**, enable **Performance (4 parts)** and select parts with tabs 1–4. Check **Part active**, MIDI channel, keyboard range and Output. For ordinary stereo operation, choose Main 1+2.

The zero Wave ROM substitutes for missing samples. Generated waves can be tested, but programs requiring sampled waves or drums may be silent or incomplete. It does not recreate original KS sample-based sounds. Silver-K recognizes a zero bank and silences dependent sources.

KS **Program boost (dB)** provides 0–30 dB of program gain. At 0 dB, the program uses its normal unboosted level. Use **Part volume**, 0–127, on Perf for the selected part's level. High program boost can overload the signal path. Incoming MIDI controllers on a shared channel affect all matching parts; editor changes and tuning/level automation address the selected part.

## 7. Original KS Data / factory Data

A full KS factory MIDI set containing SysEx, or a compatible full user Flash `.bin`, can replace `ks-init-flash.bin`:

1. Extract your official Data/factory-sound archive into a permanent folder.
2. Under **Setup → KS factory data**, select a `.mid` / `.midi` file containing KS Program/Performance SysEx, or a compatible **524288-byte** user Flash `.bin`.
3. If KS is running, selecting new Data reloads the engine and interrupts sound. Change Data between playback sessions.
4. Once KS firmware is loaded, the programs and performances appear in the `KS Factory` source in Patches. The same source name is used for test Data; its `Silver Init` programs are not original factory sounds. If changing Data leaves an old source visible, add the desired bank separately through **Patches → Add file / Add folder**.

A normal MIDI song is not KS Data. An OS update is not a user-program bank. An arbitrary short `.bin`, a Wave ROM or a single program SysEx is not a full user Flash. KS Data MIDI files must contain supported Program/Performance SysEx; Setup displays an error for invalid input.

Changing factory Data does not replace Wave ROM. Sample-dependent factory programs can remain silent or incomplete with the zero bank. These sounds require your own compatible Wave ROM, selected separately under **Setup → Wave ROM**. The supported bank is **129024 bytes**. Other sizes or formats are not converted automatically.

## 8. Factory presets and user banks in Patches

Full KS Data initialization and a library of individual patches are separate loading workflows.

1. Put banks and individual patches in `Presets`. Recognized `.syx`, `.mid` and `.midi` files are supported, including ZIP archives containing them. You may extract ZIPs first to keep your folders organized.
2. Open **Patches → Add folder** and select a bank folder. Subfolders are scanned. Use **Add file** for an individual bank or file.
3. Wait for scanning to finish. Use **Rescan** if needed and clear model/source/tag filters.
4. Select a patch row and click **Load**, or double-click the row.
5. In KS Performance mode, select the destination part with tabs 1–4 before loading. A Program loads into the selected part; a Performance contains the full four-part configuration. Some preset-loading paths reload the whole Performance and interrupt sound.
6. Use **Save current** for the current sound, **Export .syx** for a selected library entry and **Save tags** after editing tags or favourites.

K-Station runs with its own OS and does not need KS Init Flash or KS Wave ROM. Add its factory presets through Patches in the same way. K-Station ↔ KS conversion replaces unsupported functions; review the compatibility report when loading. A-Station patches can be recognized and stored, but A-Station conversion and verified X-Station import are not implemented in this version.

Patch management is unfinished. Verify saved sounds by reloading them and keep your original banks separately. Save DAW project state after changes have reached the engine, rather than immediately while moving a control.

## 9. Troubleshooting

- **Missing firmware / version error:** select a supported OS in Setup. Test Data and the zero bank are not an OS.
- **KS requests Data:** select KS Init Flash or compatible full factory Data.
- **Silver Init works but a factory patch is silent:** check whether the patch needs sampled Wave ROM. The zero bank does not contain those waves.
- **A Performance part is silent:** check Part active, MIDI channel, Lowest/Highest note and Output. Enable Aux 3+4 in your DAW, or route the part to Main 1+2 for a two-channel output.
- **Distorted output:** reduce Program boost, oscillator levels or Part volume. Boost at 0 dB is the normal level, not a muted voice.
- **Dropouts or high CPU:** check the audio device and try a larger audio buffer. This can increase latency. Multiple parts and small buffers still need optimization.
- **Noticeable latency:** Setup's Host latency reports plugin latency. Audio-driver, physical MIDI-device and DAW-chain delays are additional. Smaller buffers reduce latency but increase CPU demands.
- **Plugin not listed:** check the installation path and DAW architecture, restart and rescan. On Windows, check the Visual C++ runtime; on macOS, check the normal system launch approval.

This release is a starting point for further development. KS external audio input is not connected, and sound accuracy against physical instruments has not been established.
