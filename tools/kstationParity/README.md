# Private K-Station controller parity

This developer-only runner compares frozen research probes before integrating the v0.5a native controller. It does not change the product backend, GUI, plugin identity or release. Python 3 standard library only; the probes currently require the macOS/POSIX research environment.

Use the `tools/kstationResearch/probe.cpp` harness from upstream commit `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6`. Build its clean target against Blue's `kstationLib`, and its CPU target against a frozen upstream `kstationLib` with the same harness. The upstream harness needs one compatibility guard for Blue: change the `#else` before `using ProbeControllerMode=kstation::H8::Stage1Mode;` to `#elif defined(K_RESEARCH_STAGE1)`. The alias is only used in staged/native builds; Blue's original H8 has no such type. Keep all other stimulus/render code identical. Generate the observer source with `prepare_observer.py` before configuring upstream's CMake, even when building only the clean target. Use Release arm64 and identical DSP/AsmJit dependencies. These binaries are supplied explicitly: this runner does not download, rebuild, or trust a moving upstream branch.

Example (paths refer to private, frozen local research builds):

```sh
python3 tools/kstationParity/compare.py /private/path/K-Station_OS_2.1.01.bin /private/path/new-evidence \
  --variant blue /private/path/blue-build/kstation_research_clean original \
  --variant upstream_h8 /private/path/upstream-build/kstation_cpu original \
  --variant upstream_native /private/path/upstream-build/kstation_cpu native
```

The first variant is the reference. Each of twelve scenarios gets two seconds warmup and two seconds captured audio, offline at the native rate with 256-frame blocks. The runner authenticates the decoded firmware, freezes executable hashes, requires successful execution and zero dropped frames, checks native interpreted steps are zero, compares patch/program/clock reports, and compares full PCM/flash/MIDI SHA-256 values. It returns failure on any mismatch and retains the evidence. Use a new output directory each time. `--scenarios` selects a bounded subset without changing stimuli.

Full RAM equality is intentionally not required across different controller implementations. RAM dumps are still produced by the probes. This is bounded hardware/controller evidence, not Processor/device state-save/restore, host input, GUI, DAW, physical audio, Windows/Intel or performance proof. In particular, this runner makes no CPU claim. Native migration still needs product integration and relevant platform checks.

**Keep the output outside the repository.** It contains firmware-derived RAM/flash, PCM, MIDI, raw logs, and local file paths. Do not attach it to a public PR, Actions artifact, or release. Publish only a concise result and limitations in the development log. The runner contains no firmware or preset bank.
