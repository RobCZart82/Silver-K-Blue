Silver-K Blue brings the built-in Blue Standard and Retina 4K interfaces to K-Station and KS, with Legacy available in Setup.

- Windows x64 includes VST3 and Standalone.
- macOS Universal 2 includes AU, VST3 and Standalone for Apple Silicon and Intel.
- Both Blue skins are embedded. The downloads include source and SHA-256 checksums.
- Compatible firmware and factory data must be supplied by the user.
- Default packages use the original H8 controller for K-Station and the Blue hybrid controller for KS.

The macOS package is ad hoc signed and is not notarized. The Windows binaries have no embedded Authenticode signature.

The maintainer reported successful manual operation on macOS and Windows on 7 November 2026. The report did not enumerate host versions, architectures or individual test cases. Automated candidate checks also covered AU/VST3 external input with both firmware models on Apple Silicon, and Standalone startup on Apple Silicon and under Rosetta. Physical Intel Mac acceptance remains unverified.

See [installation and usage](https://github.com/RobCZart82/Silver-K-Blue/blob/main/blue/Documentation/01-Telepites-es-hasznalat.md) for setup. Firmware-dependent CI checks that were skipped do not establish firmware acceptance.
