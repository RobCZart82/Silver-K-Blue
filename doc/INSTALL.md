# Installation

Blue now uses a separate plugin identity and the product name Silver-K Blue. It can be installed alongside upstream Silver-K. Projects saved with older Blue builds used the upstream identity and do not automatically switch to this new identity; keep the old build for those sessions. Install the complete Silver-K Blue bundle; do not rename the original Silver-K to migrate projects. Both Blue skins are built in and selectable in Setup, with Retina as the default for new settings. The external Skins folder is optional.

## macOS

Universal 2 builds include Apple Silicon and Intel code for Standalone, AU, VST3 in the current Blue download packages. Wave ROM Builder is a separate build target. Use a 64-bit host. The build target is macOS 12.0 or later; runtime validation on older macOS versions and physical Intel Macs is still required. Older arm64-only archives require a native Apple Silicon host.

1. Extract the archive and close the DAW.
2. Copy the complete AU component to ~/Library/Audio/Plug-Ins/Components/.
3. For VST3, copy the complete bundle to ~/Library/Audio/Plug-Ins/VST3/.
4. Place the Standalone application in Applications or another permanent folder.
5. Restart the DAW and rescan plugins. Load Silver-K on an instrument track.

Keep only one AU copy across the user and system Components folders. Replace the whole bundle when updating.

Mac builds are ad hoc signed and not notarized. If macOS blocks a verified build, use its per-application Privacy & Security approval.

## Windows

Use a 64-bit host.

The published 1.0.0 download is a ZIP and uses the manual steps below. Windows EXE and macOS PKG installers are being prepared for 1.0.1; neither has been published or manually accepted yet. The candidates offer Windows VST3 and Standalone, and macOS AU, VST3 and Standalone, as selectable components. Both installer candidates are unsigned at present; the macOS app/plugin payloads are ad hoc signed and not notarized. Do not treat CI package creation as installation or DAW-compatibility acceptance. Installer upgrade/removal and preservation of user settings, presets, firmware and data must be confirmed before release.

The macOS PKG candidate installs the Standalone app under `/Applications`, VST3 under `/Library/Audio/Plug-Ins/VST3`, and AU under `/Library/Audio/Plug-Ins/Components`. The shared license and build record go under `/Library/Application Support/The Usual Suspects/Silver-K Blue`. The user's `~/Library/Application Support/The Usual Suspects/Silver-K Blue` data is outside those installer paths. If this candidate is accepted for 1.0.1, remove the app and selected plug-in bundles through Finder (authenticate when prompted); do not remove the user Application Support folder when uninstalling the products. The exact release instructions must be rechecked against the manually tested candidate.

1. Extract the archive to a permanent folder and close the DAW.
2. Copy the complete VST3 bundle to C:\Program Files\Common Files\VST3\.
3. Run Standalone from the extracted folder.
4. Restart the DAW and rescan plugins.

A missing Visual C++ runtime must be installed separately. AU is available only on macOS.

## Required data

Select a compatible firmware BIN in Setup. KS also requires Wave ROM and a MIDI file containing the complete program and performance data, or several compatible MIDI files selected together. The required zero Wave ROM is created automatically.

Required assets belong in ROMs. Additional patch libraries belong in Patches; custom skins belong in Skins. Firmware and original sound data are not included.

[First startup and plugin operation](USER_GUIDE.md)
