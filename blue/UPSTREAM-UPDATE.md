# Upstream update proposal

Manual conflict resolution required

Upstream main: `1b2844338299d29cb234793bed4550b557160b2f`
Latest stable release: [v0.8.5b](https://github.com/temaniak/Silver-K/releases/tag/v0.8.5b)

This automation never merges the main branch and never publishes releases.
Keep the Blue skins and local UI changes. Review changes, run the Windows workflow on this branch
(disable Release saving during testing), and test macOS before approving this update.

## Conflicting files

- `CMakeLists.txt`
- `README.md`
- `cmake/JuceCompatibility.cmake`
- `doc/BUILD.md`
- `doc/INSTALL.md`
- `doc/USER_GUIDE.md`
- `script/build_and_run.sh`
- `source/novation/ks/ksLib/ksBoard.cpp`
- `source/novation/ks/ksLib/ksDevice.h`
- `source/novation/kstation/kstationLib/CMakeLists.txt`
- `source/novation/kstation/kstationLib/kstationDevice.cpp`
- `source/novation/silverK/buildStandalone/CMakeLists.txt`
- `source/novation/silverK/plugin/CMakeLists.txt`
- `source/novation/silverK/plugin/external_input_host_check.cpp`
- `source/novation/silverK/plugin/silverDataFolder.cpp`
- `source/novation/silverK/plugin/silverDiagnostics.cpp`
- `source/novation/silverK/plugin/silverEditor.cpp`
- `source/novation/silverK/plugin/silverEditor.h`
- `source/novation/silverK/plugin/silverEditor_test.cpp`
- `source/novation/silverK/plugin/silverKEngine.cpp`
- `source/novation/silverK/plugin/silverKEngine.h`
- `source/novation/silverK/plugin/silverLatency_test.cpp`
- `source/novation/silverK/plugin/silverProcessor.cpp`
- `source/novation/silverK/plugin/silverProcessor.h`
- `source/novation/silverK/plugin/silverSkin.h`
- `source/novation/silverK/plugin/silverSkin_test.cpp`
- `source/novation/silverK/plugin/silverWorkflow_test.cpp`
- `source/novation/silverK/plugin/silver_check.cpp`
- `tools/package_release.py`
- `x-station-editor/README.md`
- `x-station-editor/dist/XStationEditor.html`
- `x-station-editor/web/scripts/build-portable.mjs`
- `x-station-editor/web/src/App.jsx`
- `x-station-editor/web/src/Keyboard.jsx`
- `x-station-editor/web/vite.config.js`

Only this report is proposed; conflicting upstream source changes have not been imported.
Resolve the upstream merge locally on this proposal branch before review.
