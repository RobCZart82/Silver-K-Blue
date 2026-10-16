# KS controller migration status

Scope: KS OS 2.1.02 only, authenticated decoded firmware SHA-256
`7acd2d1a8a56f44522b12327437a27826b64fc49a18851816423546fd6ea085c`.
K-Station OS 2.1.01 is a separate controller and is unchanged.

## Fidelity contract

The target is a live C++ controller with original KS state, Single/four-part
Performance, note ownership/release/stealing, sustain, all supported MIDI/NRPN,
parameter/voice calculations, clock/arp, effects control, SysEx, program and
Performance loading/saving, Flash writes and retained state. Preserve the
original controller/DSP clocks, interrupt acceptance boundaries, 128-clock
coordinator slices, B/A DSP catch-up order, serial service and sample-rate
adapter. DSP firmware and JIT are outside this conversion.

Acceptance compares actual ordered HDI reads/writes, values, responses, cycles
and in-flight context, all 24 native audio streams, controller state, Flash,
MIDI output and DSP/serial counters. Timed jobs must yield before an event can
preempt them; neither polling elimination nor recorded response playback is
an accepted implementation. No allocation, I/O, locking or waiting is added to
rendering. H8 remains an independent offline oracle. Passing finite tests is
not proof for every possible state or input.

## Present implementation versus target

The full semantic conversion is **not complete**. Current live C++ playback
executes fixed, verified C++ control flow without calling the H8 interpreter
after handoff, but retains H8 register/flag/stack/PC and memory storage. The
13,766 compatibility boundaries are not a native event-controller design.
Fresh C++ instances now execute reset/bootstrap and warm-up in C++ too, with
zero H8 interpreter calls. This reset path still uses the same compatibility
register/PC ABI; it is not a semantic typed initializer. Removing the H8 link
or renaming that frame would not finish the migration.

Implemented opt-in live jobs:

- `SILVER_KS_DSP_TRANSPORT=events`: actual HDI requests, readiness/timeout,
  words and replies, within the next event/slice deadline.
- `SILVER_KS_CONTROL_JOBS=1`: part-state selection, part-parameter-table
  selection and fixed-width scaling arithmetic. A complete remaining part
  lookup is computed with a direct formula when its selector/stack observations
  fit before the deadline; partial-entry frames are preserved too. Short
  intervals retain the old phase path. Bounded shifts are computed directly,
  including byte-counter wrap and final carry/INC flags. These jobs retain the
  compatibility frame at yields. They use actual SRAM/return-stack data, exact reference
  flags, memory clocks and wrap behavior. They cannot change interrupt masks.

Select C++ through Setup to exercise either flag. Both flags are independent
and default off. The new diagnostic backend names are
`native-cpp-control-jobs` and, with HDI jobs, `native-cpp-events-control`.
No installed bundle is replaced by this research build.

Clock-only native boundaries also aggregate aligned low-code fetch timing as
three clocks per 16-bit word. This changes no opcode decoding or DSP JIT;
the original byte-clock path remains for all other addresses/lengths/modes.

The C++ paths also snapshot pending UART bytes once per coordinator slice,
rather than attempting two empty atomic queues after every compatibility
boundary. A real pop retains the original priority/order/clock. Board input
is coordinator-only and cannot be fed concurrently with `advance()`; host
threads use Device's event queue. The original H8 tick/queues are unchanged.

For a separate diagnostic build, configure CMake with
`-DSILVER_KS_CONTROLLER_PROFILING=ON`, then set
`SILVER_KS_CONTROLLER_PROFILE=1` to enable boundary-frequency accounting.
Ordinary builds default to OFF and omit both storage and per-boundary profile
checks. Profiling allocates before playback and resets at the passage. It
excludes batched job boundaries and adds overhead; never use it for CPU comparison.
The probe can optionally preallocate `SILVER_KS_AUDIO_CAPTURE` and write all
raw native frames after rendering. That instrumented run is for correctness,
not performance. Probe duration is bounded to 2–30 seconds.

## Latest admitted Mac stage — 2026-10-16

Final AU total-process CPU, 48 kHz/256, effects, 16 held keys total,
three alternating pairs of 16-second windows, sequential DSPs and profiling
compiled OFF (100% = one CPU):

| AU, FX | Frozen accepted event-transport AU | Final candidate | Relative change |
| --- | ---: | ---: | ---: |
| single-fx | 47.097706% | 46.607250% | -1.041% |
| multi-fx | 46.911813% | 46.225413% | -1.463% |

This is a small local improvement; earlier trials are not pooled. All 12 AU
windows match output/tails and programs, with zero lost frames or over-budget
callbacks. Nine complete H8 tapes (8,085,451 events), 14 full raw-audio
comparisons (6,075,618 frames per side x 24 streams), UI/state switches and
focused UART/IRQ/arithmetic/fetch-clock checks pass. This does not prove all
possible presets/inputs or physical-device equivalence. Inputs use the
authenticated OS, synthetic Flash and zero Wave ROM; sampled banks are untested.

The exact measured AU and separately built VST3/Standalone are in the ignored
local `build/optimized-mac-ks-control-jobs-2026-10-16/` package, with source
identity/checksums. VST3/Standalone have not been separately hosted this round.
The shared Optimized checkout and installed plugins are unchanged. This is
an isolated verified stage, **not completion of the removal gates below**.

## Remaining removal gates

1. The C++ compatibility reset path now passes independent full-boot command,
   state and timing comparison. Replace this path with semantic typed native
   initialization jobs as part of the compatibility-frame removal.
   The admitted image resets at `0x2ec`, copies code/data from Flash into SRAM,
   executes a copied overlay trampoline at `0xffec00`, then returns through
   `0x180`/`0x36e`. A saved mutable startup image is not a native initialization
   algorithm and must not replace this work.
2. Lift normal foreground, periodic voice/parameter, parser/clock/arp, state
   and Flash lifecycles out of the instruction-boundary compatibility ABI.
   Retain exact data-dependent durations and interruption/yield state.
3. Replace the generic CPU register/flag/stack/address-space frame with typed
   controller state and explicit bounded job storage; eliminate the large
   generic device-map allocation. Do not retain a hidden interpreter fallback.
4. Remove H8 dependencies from the production KS target only after these
   gates and broader musical/state/patch regression checks pass. Keep the
   original separately compiled for differential verification. K-Station still
   requires its own H8 implementation.
5. Build and repeat the comparisons separately on Windows x64, then validate
   the actual host/plugin formats. The control-job source uses portable C++17;
   macOS results alone do not establish Windows correctness or CPU savings.

See `OPTIMIZATION_LOG.md` for measured results and source/build identities.
The isolated checkout is intentionally independent of concurrent preset-manager
work; no browser/library/patch-manager changes belong to this migration.

## Reset compatibility follow-up — 2026-10-16

Fresh C++ Device instances select the C++ reset path before boot. The reset
vector, SRAM initialization/copy/checksum, overlay trampoline, startup DSP
downloads, initial synth state and warm-up no longer execute H8 instructions.
The original H8 path remains separately selectable as the reference while
the semantic conversion is incomplete; this is not yet a production target
with H8 limited to tests. DSP firmware/JIT, clocks, slices and serial order
are unchanged. Mid-session H8/C++ switching retains the existing state ABI.

All 4,128 reset boundary cases pass with arbitrary values/flags and mapped
or clock-only fetches. The actual H8 read8 is now the fetch-clock oracle,
including signed trampoline PC and 24-bit physical address wrap (11,520
cases). The earlier 10,880-case byte-clock oracle did not distinguish this
signed-PC/physical-bus case; it must not be described as proving that case.

Four full cold timelines pass: Single, Performance, irregular slice budgets
and a synthetic nonzero Wave mapping. Each compares controller/IRQ/IO state
through reset, complete DSP-port/slice tapes, full memory/Flash/state/MIDI and
24,000 audible frames x 24 streams after warm-up. Each C++ run executes zero
H8 instructions. The mapping variant is not an authenticated sampled-wave
bank and does not establish sampled-sound accuracy. Final musical/plugin
rechecks and identities are recorded in OPTIMIZATION_LOG.md. Local test ID:
`perf-ks-native-reset-20261016`. The register/PC/flags/stack model still remains.

The final follow-up passes all 13 cold/musical/host/CPU stages. AU whole-process
medians (100% = one CPU, same alternating three-pair method as above):

| AU, FX | Frozen accepted event-transport AU | C++ reset + current jobs candidate | Relative change |
| --- | ---: | ---: | ---: |
| single-fx | 44.863262% | 44.445200% | -0.932% |
| multi-fx | 44.716487% | 43.866525% | -1.901% |

This new series has exact output/tails and zero lost/over-budget frames in all
12 windows. It measures the combined candidate against the earlier accepted
event-transport AU; it is not a measurement of reset alone. The exact tested
AU and source snapshot are in ignored
`build/optimized-mac-ks-native-reset-2026-10-16/`. This is still an intermediate
compatibility implementation. H8 has not been restricted to test targets.
