# Private Device state roundtrip

Build the explicit `kstationDeviceState_test` target in the normal Blue build. It is excluded from default builds, CTest and release packaging. The tool requires three arguments: decoded firmware, input state (or `-` to create the positive-control state), and a **new private output path**.

```sh
SILVER_K_DATA=/private/path build/path/kstationDeviceState_test \
  /private/path/K-Station_OS_2.1.01.bin - /private/path/blue.state
```

To investigate the upstream candidate, compile the **same test source** against frozen upstream `kstationLib` at `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6`, defining `PRIVATE_STATE_NATIVE=1`. This explicitly selects `ControllerMode::Native`; the upstream Device constructor otherwise defaults to Original. Restore `blue.state` with that binary and save `native.state`, then restore `native.state` with the Blue binary and save `blue-roundtrip.state`. Compare all three complete files locally. Use fresh output paths; do not overwrite evidence.

The positive controls are CC74=93 and CC71=27, checked in both patch and cached controller snapshots after offline settling. Every run requires successful restore/save and finite stereo output with an audible note. The backend name is printed. This checks the Device state ABI for one program and two controllers, not all preset banks, arp state, Processor/DAW serialization, exact audio phase, realtime capacity or all platforms. State files contain user preset data: keep blobs and raw logs outside the repository and public CI/artifacts. No ROM or preset bank is supplied.
