# Build

macOS requirements: macOS arm64, Xcode command-line tools, CMake 3.22+, Python 3, and internet access for the first configuration. Dependencies are fetched from pinned source revisions and compiled locally; prebuilt libraries are not required.

```sh
git clone https://github.com/temaniak/Silver-K.git
cd Silver-K
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --target silverK_Standalone silverK_VST3 silverK_AU silver_check silverClock_test silverWorkflow_test -j 6
SILVER_K_DATA=/tmp/silver-k-tests ctest --test-dir build --output-on-failure -R '^silver_'
python3 tools/check_repository.py
```

Products are under `build/source/novation/silverK/buildStandalone/silverPlugin/silverK_artefacts/Release/`. macOS bundles receive an ad hoc signature for local testing. Signing for distribution is a separate step. Deployment target 12.0 is not a claim that all older macOS releases have been tested.

## Windows x64

Install Visual Studio 2022 or later with **Desktop development with C++**, a Windows SDK, CMake 3.22+, and Python 3. In PowerShell:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target silverK_Standalone silverK_VST3 silver_check silverClock_test silverWorkflow_test --parallel 4
$env:SILVER_K_DATA = "$env:TEMP/silver-k-tests"
ctest --test-dir build -C Release --output-on-failure -R '^silver_'
python tools/check_repository.py
```

Standalone provides JUCE's Windows audio backends (WASAPI/DirectSound); an optional locally supplied ASIO SDK can be enabled with `-DSilverK_ASIO_SDK_PATH=C:/path/to/asiosdk`. The SDK is not redistributed. The core and firmware/patch logic are shared with macOS; AU and codesigning are conditional on macOS.

For offline builds, pass `-DJUCE_SOURCE=/path/to/JUCE -DDSP_SOURCE=/path/to/dsp56300 -DASMJIT_SOURCE=/path/to/asmjit` using the exact revisions in [`cmake/Dependencies.cmake`](../cmake/Dependencies.cmake). Those must be **source checkouts**, not binary releases. The macOS JUCE compatibility patch applies to a build-local copy only.
