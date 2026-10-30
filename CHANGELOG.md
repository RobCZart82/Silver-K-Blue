# Silver-K Blue changes

## Unreleased

- Audit follow-up (pending merge): reject invalid substream writes, make rejected grouped jobs exception-safe, repair container callback types/lifetimes, and validate image dimensions before decoding with a total skin pixel budget.


- Prepare English and Hungarian entry points and current installation documentation.
- Support the intended repository's public or private visibility in upstream update automation, while retaining manual review and preserving main.
- Include English installation and operation documentation in future download packages.
- Replace private-only workflow labels and explicitly restrict Release publication to manual dispatch.
- Record development priorities and remaining publication checks.

These changes describe the publication-preparation branch until it is merged. No new binary release has been produced by this documentation update.

### Pending framework fixes

[PR #1](https://github.com/RobCZart82/Silver-K-Blue/pull/1) contains binary/WAV reader validation, FourCC template, multi-worker shutdown, realtime MIDI parsing, SysEx MIDI export, opaque preset parsing and source-packaging fixes. They require successful Windows/macOS checks and merge before being described as released fixes.

## Embedded-skin test builds — 2026-10-29

Windows x64 build 3 and macOS Universal 2 build 2 use source commit `40b68eeebd8e2c7d4a92229aa25fcb1217a652cc`.

- Embed Blue Standard v1.0 and Blue Retina 4K v1.0 in Windows VST3/Standalone and macOS AU/VST3/Standalone.
- Default to Retina for new settings and preserve saved choices.
- Retain support for optional editable external skin packages.
- Record build identity and package hashes.

ROM-free checks and targeted interface checks passed as recorded in the [build report](blue/Documentation/10-Elso-beepitett-kiadas.md). Firmware-dependent checks and physical-machine/DAW acceptance have separate limitations. These downloads do not include PR #1.

Earlier Blue interface history is retained in [Hungarian](blue/CHANGELOG-HU.md); original Silver-K release notes remain under doc/releases.
