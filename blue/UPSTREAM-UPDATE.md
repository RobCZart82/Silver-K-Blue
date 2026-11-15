# Upstream update proposal

Manual conflict resolution required

Upstream main: `9079f67d3ecb1743d38131484ef4f41427bf515f`
Latest stable release: [v0.5a](https://github.com/temaniak/Silver-K/releases/tag/v0.5a)

This automation never merges the main branch and never publishes releases.
Keep the Blue skins and local UI changes. Review changes, run the Windows workflow on this branch
(disable Release saving during testing), and test macOS before approving this update.

## Conflicting files

- `CMakeLists.txt`
- `README.md`
- `doc/BUILD.md`
- `doc/USER_GUIDE.md`
- `script/build_and_run.sh`
- `source/novation/ks/ksLib/ksBoard.cpp`
- `source/novation/ks/ksLib/ksDevice.h`
- `source/novation/kstation/kstationLib/CMakeLists.txt`
- `source/novation/kstation/kstationLib/kstationDevice.cpp`
- `source/novation/silverK/buildStandalone/CMakeLists.txt`
- `source/novation/silverK/plugin/CMakeLists.txt`
- `source/novation/silverK/plugin/external_input_host_check.cpp`
- `source/novation/silverK/plugin/silverDiagnostics.cpp`
- `source/novation/silverK/plugin/silverEditor_test.cpp`
- `source/novation/silverK/plugin/silverKEngine.cpp`
- `source/novation/silverK/plugin/silverLatency_test.cpp`

Only this report is proposed; conflicting upstream source changes have not been imported.
Resolve the upstream merge locally on this proposal branch before review.
