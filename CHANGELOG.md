# Silver-K Blue changes

## 1.0.1 — Unreleased

- Add release verification without publication, a read-only Actions workflow and a preserved JSON inventory report.

- Include verified Windows/macOS installers and provenance sidecars in unified releases, with version/source/platform/hash gates and complete checksums.

- Honor the chosen Windows Standalone installation directory, preserve the application folder and unselected VST3 during upgrades, and refuse to overwrite existing installer outputs.
- Isolate Blue preferences from upstream Silver-K, with one-time migration of existing valid settings and safe handling of migration failure (#75).
- Resume matching interrupted release drafts with SHA-256 asset verification and revalidate draft metadata before publication (#75).
- Preserve manual upstream proposal commits even underneath later bot commits (#75).
- Verify remote lightweight/annotated release tags against the package source commit (#77).
- Correct build commands to compile all selected test executables and update release documentation.
- Planned work and acceptance gates, including Windows EXE and macOS PKG installers, are tracked in [the development plan](blue/Documentation/02-Aktualis-fejlesztesi-terv.md). Installers are still under development.

## 1.0.0 — 2026-11-07

Published from `5adc19eccddc38335b7652be599766823a6d684c`. [Release and downloads](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/blue-v1.0.0).

- Unified Windows x64 VST3/Standalone and macOS Universal 2 AU/VST3/Standalone ZIPs with embedded Standard and Retina skins, source and SHA-256 checksums.
- Integrated framework safety fixes, including binary/WAV input validation, worker shutdown, MIDI/SysEx handling and image dimension/memory limits. The earlier #1 and #3 work is included in this source.
- English/Hungarian entry points and packaged installation/user guides.
- External input support and diagnostics, with optional native-controller research kept separate from the default engines.
- Maintainer reported successful macOS and Windows manual operation. Detailed limits and platform build evidence are recorded in the release notes. macOS is ad hoc signed and not notarized; Windows binaries are unsigned.

## Embedded-skin test builds — 2026-10-29

Windows x64 build 3 and macOS Universal 2 build 2 use source commit `40b68eeebd8e2c7d4a92229aa25fcb1217a652cc`.

- Embed Blue Standard v1.0 and Blue Retina 4K v1.0 in Windows VST3/Standalone and macOS AU/VST3/Standalone.
- Default to Retina for new settings and preserve saved choices.
- Retain support for optional editable external skin packages.
- Record build identity and package hashes.

ROM-free checks and targeted interface checks passed as recorded in the [build report](blue/Documentation/10-Elso-beepitett-kiadas.md). Firmware-dependent checks and physical-machine/DAW acceptance have separate limitations. These downloads do not include PR #1.

Earlier Blue interface history is retained in [Hungarian](blue/CHANGELOG-HU.md); original Silver-K release notes remain under doc/releases.
