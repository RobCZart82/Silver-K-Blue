# Silver-K Blue — embedded-skin test build

Silver-K Blue is an unofficial variant of [temaniak/Silver-K](https://github.com/temaniak/Silver-K), with integrated Blue Standard and Blue Retina 4K skins and additional interface changes. It is based on the original authors' work; see LICENSE.md and THIRD_PARTY.md.

## Install

Close your DAW before updating. Blue uses a separate plugin identity and the name Silver-K Blue, allowing installation alongside upstream Silver-K. Older Blue sessions used the original identity: keep the old build for those projects; they do not automatically migrate to the new Blue identity.

| Platform | Product in this archive | Destination |
| --- | --- | --- |
| Windows x64 | Plugins/VST3/Silver-K Blue.vst3 | C:\Program Files\Common Files\VST3\ |
| Windows x64 | Standalone/Silver-K Blue.exe | A permanent folder; launch directly |
| macOS | Plugins/VST3/Silver-K Blue.vst3 | ~/Library/Audio/Plug-Ins/VST3/ |
| macOS | Plugins/AU/Silver-K Blue.component | ~/Library/Audio/Plug-Ins/Components/ |
| macOS | Standalone/Silver-K Blue.app | Applications or another permanent folder |

Only products for the archive's platform are included. Use a 64-bit host. macOS Universal 2 includes arm64 and x86_64; the build targets macOS 12.0 or later. Physical Intel Macs and older macOS versions require separate runtime validation. Windows builds are unsigned; macOS bundles use ad hoc signing and are not notarized. A missing Windows Visual C++ runtime must be installed separately.

Restart and rescan your host after installation. Standard and Retina are built in and selectable in Setup; new settings default to Retina. Existing saved choices are preserved. The optional Skins folder contains editable external copies; it is not required for the built-in skins.

## First startup

Select your own compatible firmware in Setup. K-Station uses its own firmware. KS also needs compatible program/performance MIDI data and Wave ROM. An automatically created zero Wave ROM supports generated oscillator waves; sounds using sampled waves may be incomplete or silent. Proprietary firmware and factory sound data are not included.

In Standalone select an audio device and MIDI input. In a DAW enable instrument-track monitoring and send MIDI to the plugin. See INSTALL.md and USER_GUIDE.md for details.

## Build identity and verification

build-info.json records the exact repository, source commit and platform. SHA256SUMS.json lists package file hashes; the external .zip.sha256 file verifies the download. ROM-free build tests do not replace interactive DAW and audio acceptance tests with your own firmware. Read the release notes for the validation performed on this particular build.

Source: https://github.com/RobCZart82/Silver-K-Blue — use the commit recorded in build-info.json. The source code is GPLv3; original notices are preserved. See THIRD_PARTY.md for dependency and Blue artwork provenance.
