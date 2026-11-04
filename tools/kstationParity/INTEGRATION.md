# K-Station native integration provenance

The native controller implementation and its transitive headers come from upstream `temaniak/Silver-K` commit `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6`. The existing H8 facade, hardware scheduler and Device interface are updated together. Upstream unrelated test programs, product names, version changes, KS controller migration and GUI changes are excluded. Blue's input-overflow counter behavior and private state tool are retained.

`SILVER_K_NATIVE_CONTROLLER` is a CMake migration-validation option, **OFF by default**. Enabling it selects the native controller for KEngine at build time, without adding a GUI selector. The normal Blue build continues to use original H8. For a local candidate build, configure `-DSILVER_K_NATIVE_CONTROLLER=ON`; do not identify such a build as the default release.

The library compiles both implementations on Windows/macOS. The existing workflows also build and run the ROM-free controller-device-state guard (30,000 operations, bus/cycle/interrupt/immutable-write checks). Native Processor firmware regression is a separate private local check: public CI without firmware does not establish that result. Upstream NATIVE_CONTROLLER documentation describes its own implementation and measurements; it is not a Blue performance guarantee.

Local arm64 evidence includes the 12-scenario hardware parity series, Device state roundtrip, and opt-in integrated Processor latency/rate/buffer/state, external-input filter/vocoder and diagnostics checks. Full DAW, physical-input and Windows/Intel runtime validation remain open. No general CPU speedup has been established. Default selection, GUI layouts, Blue identities and release version are unchanged.
