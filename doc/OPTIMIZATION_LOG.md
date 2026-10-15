# Silver-K optimization journal

This versioned journal is shared between Windows and macOS work. Append dated
entries; do not replace earlier measurements with results from another machine.
Raw local reports are not committed. A hypothesis is not a measured result.

## Current state — 2026-10-15

- Full post-boot follow-up: `SILVER_KS_CONTROLLER=native` now runs normal KS
  playback control in C++, with H8 retained for original bootstrap only. Eight
  passages match complete DSP tapes, 24-stream integer audio, state/Flash and
  MIDI output; the interpreter lock and actual counters show **zero H8 calls
  after boot**, including native preparation. Frozen original-H8 CPU controls
  show **2.4–5.3% lower process CPU seconds**, three pairs per passage on the
  local Windows Intel. This is a clock-compatible native port, not a batched
  OsTIrus-style redesign. Default H8 and the earlier hybrid remain available;
  keep the experiment off by default. Mac, real VST3 host and Ryzen pending.
- Historical live note/sustain stage: C++ handles Note On/Off decisions, key and
  velocity memory, held-key/mono priority, CC64 state and part/pedal release
  decisions. Complete DSP bus tapes/audio match H8 in four scenarios, including
  all four voice modes and pedal threshold/repeated-note cases. This remains a
  **hybrid**: original H8 handles UART/part routing and DSP voice parameter
  generation/upload. Default H8 unchanged; opt-in
  `SILVER_KS_CONTROLLER=native-midi`. CPU medians are **0.8–1.6% higher**, no
  saving on the local Windows Intel. Keep off by default; Mac/Ryzen pending.
- Controller follow-up on branch **Optimized**: recorded original H8 traffic to
  both KS DSPs (boot, Single and four-part passage), and added an opt-in checked
  C++ recorded-scenario controller. Original H8 remains the plugin default.
  Three matched pairs per passage show **23.4% / 25.4% lower CPU seconds** with
  identical 24-stream audio hashes and DSP counters. This prototype replays
  precomputed commands; it is not yet a live MIDI/patch/state replacement and
  these savings must not be attributed to a completed native controller.
- Phase: opt-in KS dual-DSP workers implemented with original H8 and exact
  slice barriers. All six Summary two-worker runs and one Detailed run had zero
  missing frames on the local Intel, while sequential controls failed. Exact
  native-audio/state equivalence passed; total process CPU time increased.
  This remains opt-in and unverified on the tester's Ryzen or a real VST3 host.
  The earlier P-core experiment also remains opt-in; its results are preserved
  below. JIT settings and guest scheduling targets are unchanged.
- CPU cost follow-up: equal-work native KS render (six runs, exact audio and
  guest counters) shows **126.3% more total CPU seconds** for two DSP workers
  (5.875 -> 13.297), while wall time falls 23.1% (5.872 -> 4.514 s). This is
  improved throughput at increased CPU cost, not CPU saving. Boot/warm-up and
  host polling are excluded; real DAW/Ryzen CPU cost remains unmeasured.
- Active environment: Windows x64. macOS compilation and measurements pending.
- K Windows notification response improved; active worker wall cost did not
  improve. KS retains its exact original polling path: event parking failed both
  Low and Balanced in extended repeats; later polling trials also failed, so the
  causal explanation remains open. Weak-CPU / DAW checks remain.
- Baseline source: record the revision embedded in each diagnostic report, plus
  dirty state and the patch/commit containing this instrumentation.
- No conclusion yet about the tester stutters or the fastest remedy.
- OsTIrus 2.2.25 source comparison confirms separate TI/TI2 DSP threads,
  a native C++ controller implementation and host-side output summing. KS's
  default runs original H8 and both DSPs serially with shared serial timing;
  native control and parallel DSP scheduling are now separate opt-in modes.
  The tester's successful OsTIrus run is a useful comparison, but no matched
  OsTIrus/Silver-K runtime benchmark exists yet. Follow-up: K-Station works;
  only KS stutters during playback, in VST3 on Windows 11 on a Ryzen 3 3100
  (4 cores / 8 threads, user reports 24 GB RAM and SSD storage). This is not a
  reported boot failure. Intel P/E-core
  placement findings must not be presented as this tester's cause or remedy.
- Scheduler trace follow-up: KS's largest reproduced losses coincide with the
  worker spending about 94% of its scheduled time on E-cores and nearly saturating
  one logical CPU. This occurs in the frozen original, poll override and address
  candidate. Ready-to-run delay is small in these failures. This is strong local
  evidence for a placement/throughput issue, not a proven fix or explanation of
  every tester report. The subsequent placement experiment below changes only
  the KS worker's in-process CPU-set selection; Windows settings remain unchanged.

## 2026-10-15 — source review, before instrumentation

- Reviewed Silver-K 0.2.4 Beta, Gearmulator release 2.2.25 and DSP dependency pins.
- Observed: K/KS use one emulation worker per instance; KS schedules H8 and both
  DSPs there. JIT block limit 32, DO limit 2, block linking disabled. Output
  exchange is per frame, and the worker sleeps for a requested 50 µs at its lead.
- Comparison: OsTIrus uses separate DSP workers and a batched trampoline;
  Xenia links JIT blocks and selectively enables dynamic peripheral addressing.
- Hypotheses: DSP dispatch overhead, worker scheduling/refill bursts, controller
  cost and repeated control snapshots may contribute. No timings or claimed
  speedups. Larger reserves do not fix sustained throughput below realtime.
- Reference: https://github.com/dsp56300/gearmulator/releases/tag/2.2.25

## 2026-10-15 — optional performance instrumentation

- Added Summary and Detailed sessions to Setup, JSON export, per-session
  callback/worker/queue counters and sampled H8/DSP/serial wall timers.
- Detailed probes sample every 256th board advance; summary worker timing spans
  up to 256 generated frames. Export/formatting stays off realtime threads.
- Reports describe CPU, OS, architecture, host/wrapper, compiler/configuration,
  configure-time source revision, model, rates, prepared/actual blocks, buffer
  reserve and missing/stale frames. Original data and personal paths excluded.
- Extended the existing realtime fixture with optional JSON reports. Added
  ROM-free counter/session/concurrent-reader tests. Instructions are in
  `PERFORMANCE_DIAGNOSTICS.md`; future agents should read both documents.
- Emulation algorithms, JIT settings, virtual clocks, buffers, queue algorithms,
  MIDI sequencing, resampler and thread priorities remain unchanged.
- Windows validation: Release Standalone, VST3, realtime fixture, editor, latency
  and diagnostics targets built with MSVC 19.44 / NMake. Counter/session/threaded
  reader tests, host-clock test and native-editor test passed. Setup screenshot
  inspected. Source-only repository audit and diff whitespace check passed.
- Repeating the editor test in its previous settings folder initially failed its
  pre-existing 75% default-scale assertion because the first run saves another
  scale. Re-ran successfully with a fresh isolated data folder. This was not an
  instrumented audio failure. Also corrected the existing latency test's buffer
  type to `synthLib::SysexBuffer` so it builds with Windows' PMR-enabled framework;
  production patch handling was not changed.
- Latency host-reentry test with private K/KS firmware passed (53.72 s): buffer
  and rate changes, state restore, asynchronous worker completion, offline audio
  and realtime transition. macOS compilation/audio remain unverified here.

## 2026-10-15 — first Windows captures (measurement, no optimization)

- Source identity in reports: `5aa9e31d06f5`, dirty instrumentation tree; Release,
  MSVC 1944, DSP pin `44353be2f0156cac970c6ab4f467ea1998b40605`.
- Machine: Intel Core i5-13600KF, x64, 14 physical / 20 logical CPUs, ~32 GB RAM.
  JUCE reports Windows 10. Host/wrapper Unknown/Undefined because this is the
  command-line fixture, not an audio device or DAW.
- Scenario: existing 12 s fixture, 48 kHz, prepared/actual 128 frames, Balanced,
  one instance, single-part K or KS, synthetic zero wave bank for KS, no editor,
  state saving off, first note then eight-note chords. Not a four-part stress
  test. No audio driver/backend is exercised. Power mode was not controlled.
- Sequential Off/Summary/Detailed, one run of each per model. All six exited 0;
  no startup or steady missing frames, no stale frames, no diagnostic callback
  deadline overruns. Note onset and peak agreed within the fixture's printed
  precision across modes; this is not a bit-exact sound-accuracy validation.
- Raw reports/logs/index are local at ignored
  `output/perf/20261015-120329/` (IDs `win32-{K,KS}-128-{off,summary,detailed}-r1`).

| Measurement | K Summary | K Detailed | KS Summary | KS Detailed |
| --- | ---: | ---: | ---: | ---: |
| Worker wall time / generated audio duration | 0.313 | 0.312 | 0.789 | 0.795 |
| Mean callback, ms | 0.02565 | 0.02613 | 0.00612 | 0.00612 |
| Mean actual worker sleep, ms | 1.621 | 1.623 | 1.563 | 1.578 |
| Missing frames | 0 | 0 | 0 | 0 |

Detailed mean time per sampled board advance (microseconds): K H8 1.209,
DSP A 1.481; KS H8 0.981, DSP A 1.648, DSP B 1.561, serial 0.193. These
deterministic samples include OS preemption and are not an exact CPU breakdown.

- Interpretation: this machine sustains the fixture, with materially less worker
  throughput headroom for KS. The requested 50 µs sleep actually spans about
  1.6 ms on this Windows setup. Investigate scheduling/reserve along with DSP
  throughput; neither observation establishes the cause on tester machines.
- One pass cannot establish a precise instrumentation overhead or comparative
  speedup: repeat/reorder captures and test real drivers before drawing that
  conclusion. No stutters were reproduced here. GUI/driver interference and
  low-latency/four-part/multiple-instance scenarios still need captures.
- Next Windows round: identical passage at 128/256/512 in a real DAW, then KS
  four-part and low-latency profiles, UI open/closed, on an affected weak CPU.
  Next macOS round: build the same source, repeat matched captures, append the
  results and mark macOS checks explicitly rather than inheriting Windows results.

## 2026-10-15 — Windows worker wake-up, implementation and comparison

- Authorized scope: replace full-output-queue polling on Windows, measure K and
  KS and keep JIT, guest clocks, buffer reserves, resampling, priorities and
  instruction semantics unchanged. macOS retains its previous polling path.
- Saved pre-change fixture binary at ignored
  `output/perf-wakeup/before/silverRealtime_test.exe`, SHA-256
  `62a747ac1be4f5d857e40bc1eac36668a9547cdcf47cd7baf1cac40d60915907`.
- Fresh pre-change captures: `output/perf-wakeup/before/20261015-121804/`,
  3 runs each for K/KS, 48 kHz, 128 frames prepared/actual, Balanced, Summary,
  12 s fixture, one instance, no editor/state saves, same private assets as above.
  All 6 returned 0, no missing frames. These fresh controls are preferable to
  comparing an old single run: CPU/power/scheduling conditions vary over time.
- Added a shared Windows wait helper: arm/recheck + sequence-based WaitOnAddress,
  notify after consumed blocks, notify pause/stop and offline refill, bounded
  10 ms safety timeout. Resolves APIs at construction and retains Win7/non-Windows
  polling fallback; no process/system-wide timer changes or busy spin.
- Added `worker_wake_latency` and actual `worker_wait_backend` to reports. The
  runner can force `--worker-wait poll` for same-binary A/B measurements. Total
  idle wait duration is no longer interpreted as the notification response time.
- Tests cover notification before parking, repeated progress races, stop while
  parked, timeout and forced-poll fallback. These and diagnostics, host-clock and
  editor tests passed on Windows. Release Standalone/VST3 and the fixture built.
- Measured executable after the change:
  `output/perf-wakeup/after/silverRealtime_test.exe`, SHA-256
  `fe64b2ec9f81445b2d6498420aebf790fcd7c521b3c0f4588347e9ae80843a42`.
  Both poll-control and address measurements below use this exact executable.
- Same machine/compiler/assets/scenario as the fresh controls: i5-13600KF,
  Windows reported as 10, Release/MSVC 1944, source `5aa9e31d06f5` + dirty changes,
  Summary, 48 kHz / 128 prepared and actual frames / Balanced / 12 s / single part,
  no editor, one instance and state saves off. No DAW/driver, controlled power
  mode, thermal control, randomized ordering or weak-CPU restriction.
- Address captures: `output/perf-wakeup/address/20261015-122713/`.
  Same-binary poll controls: `output/perf-wakeup/poll-control/20261015-122837/`.
  Three sequential runs per model per backend (address first, then poll).
  All 12 returned 0, with zero missing/stale frames and zero callback overruns.
  Together with the frozen original controls this is 18 successful main captures.
  A local summary is at ignored `output/perf-wakeup/comparison.json`.

The following table reports medians of the three run means, except worst-case
latency and minimum queue depth, which are the worst observation across all
three. Histogram p99 values are upper bounds, not exact percentiles.

| Metric | K poll control | K address | KS poll control | KS address |
| --- | ---: | ---: | ---: | ---: |
| Notify-to-wait-return mean, microseconds | 1258.810 | 16.346 | 1191.419 | 13.142 |
| Notify-to-wait-return p99 upper bound, microseconds | 2097.151 | 131.071 | 2097.151 | 131.071 |
| Worst notify-to-wait-return, microseconds | 2282.5 | 148.6 | 2078.9 | 157.8 |
| Worker wall time / generated audio duration | 0.261 | 0.281 | 0.524 | 0.593 |
| Callback mean, microseconds | 25.376 | 25.062 | 5.396 | 6.971 |
| Total idle wait mean, ms | 1.526 | 1.925 | 1.559 | 1.134 |
| Minimum ready queue at render entry, frames | 382 | 472 | 348 | 374 |
| Missing/stale frames and callback overruns | 0 | 0 | 0 | 0 |

- The frozen original controls had median active ratios K 0.253 / KS 0.519 and
  mean sleep K 1.532 / KS 1.554 ms, broadly consistent with instrumented polling.
  They have no notify-to-return metric, so the wake-latency comparison uses the
  new executable's poll override, not invented timings for the old binary.
- Result: much faster notification response and more queue reserve in these
  captures. Guest/audio latency stayed K 10.5417 / KS 10.6667 ms; printed note
  onset and peak matched all original/poll/address captures. No claim of a
  bit-exact audio validation or lower end-to-end MIDI/audio latency.
- Tradeoff: active worker wall ratios increased in the address runs, particularly
  KS (0.524 -> 0.593). This is not thread CPU usage and includes preemption, but
  it prevents claiming a throughput/CPU saving. The cause is not established;
  scheduling/core placement/power behavior need separate evidence. The candidate
  addresses timer-induced wake stalls; it does not solve insufficient emulation
  throughput. Keep the poll override for diagnosis if an affected CPU regresses.
- Small-buffer follow-up: 48 kHz / 64 frames / Low latency / Summary, one run per
  model/backend at `output/perf-wakeup/low-poll/20261015-123124/` and
  `output/perf-wakeup/low-address/20261015-123152/`. K poll missed 7 frames
  (6 startup / 1 steady), K address 0. KS poll missed 0, unrestricted KS address
  missed 333330 steady frames and failed. Fast notification response alone did
  not prevent loss of worker throughput; causal attribution remains incomplete.
- A second candidate used a per-worker high-resolution waitable timer for the
  requested 50 us (no global timer changes). Local archive:
  `output/perf-wakeup/timer-prototype/silverRealtime_test.exe`; reports at
  `output/perf-wakeup/low-timer/20261015-123736/`. Two runs each: KS missed 324556
  frames in one run and 0 in the other; K missed 0 in both. Notification response
  was roughly 0.45–0.48 ms. This candidate was rejected, and its code/runner
  option removed; no claim that it solves the regression.
- Reserve-guard candidate (subsequently rejected): keep address waiting for K, and for KS only
  at reserve durations >=512/48000 s. KS below that duration uses the existing
  polling wait, reported as `poll-small-reserve`. This protects the known failing
  scenario without increasing buffers or changing JIT/guest code. The address
  override does not bypass this safety gate. This is a bounded first step,
  not a solution to all weak-CPU / Low-latency failures.
- Guarded-build repeats used archived executable
  `output/perf-wakeup/reserve-guard-prototype/silverRealtime_test.exe`, SHA-256
  `01b37c61805e117152767b7d0ad427762a732e6b64045d3940d08d6b64f9aeb0`.
  Poll controls: `output/perf-wakeup/final-poll/20261015-124451/`, all six passed.
  Guarded-default captures: `output/perf-wakeup/final-default/20261015-124616/`.
  K passed all three. KS Balanced failed two of three, missing 259138 / 99853
  steady frames; the third run passed. Thus the small-reserve guard was not
  sufficient and was rejected, rather than promoted on the first three passes.
  The low-profile follow-up at `output/perf-wakeup/final-low/20261015-124740/`
  passed all four return codes: K event waiting missed 0 frames in both runs;
  KS polling had 15 startup frames in one run and 0 in the other, no steady
  misses. The fixture deliberately permits startup-only misses: return 0 must
  not be read as a guarantee of zero startup dropouts.
- Final implementation policy: event waiting is enabled for K on supported
  Windows hosts. KS uses its exact original 50 us sleep (`poll-50us`), without
  the new arm/recheck/notification path. KS still records total sleep duration,
  but has no notify-to-return observations. No KS improvement is claimed.
  The reserve gate and high-resolution timer were removed. JIT, buffers, guest
  timing, resampling and priorities remain unchanged.
- Cause of the KS regressions is still unproven; fast wake latency accompanied
  worse active wall cost and sometimes large losses. Investigate scheduler/core
  placement and sustained sections with a trace before enabling event parking
  for KS. Do not treat the failed trials as evidence against all event-based
  designs, or as a solved root cause of the testers' stutters.
- A further K-event / KS-helper-poll build (before restoring exact KS code) was
  archived at `output/perf-wakeup/production/silverRealtime_test.exe`, SHA-256
  `b2fc7aada5a562723c0ff2a744d2bee30b957d6d90e9807b0e9f15e8dc03cc23`.
  Reports: `output/perf-wakeup/production-results/20261015-125123/`.
  K passed all three, zero missing frames. KS polling failed once with 97196
  missing frames (60844 startup / 36352 steady), then passed twice with zero.
  Thus the failures cannot be attributed solely to address parking. Restoring
  exact KS polling avoids promoting additional synchronization overhead without
  demonstrated benefit; it is not a claim that the baseline issue is fixed.
- Frozen-original replay `output/perf-wakeup/original-recheck/20261015-125516/`
  failed all three KS runs with 196716 / 317617 / 202633 steady missing frames.
  This replay overlapped a Release build, so it is a contention experiment,
  **not an uncontended performance control**. The original implementation can
  also miss its deadlines under competing CPU load; this does not establish the
  cause of the other failed runs.
- Uncontended frozen-original KS replay after the build finished:
  `output/perf-wakeup/original-idle-recheck/20261015-125802/`. All three passed,
  zero missing/stale frames and callback overruns; median worker ratio 0.524,
  sleep 1.441 ms, callback mean 5.519 us. Do not mix the concurrent-build replay
  with this control or infer a stable regression from their difference alone.
- Final executable after restoring exact KS polling:
  `output/perf-wakeup/final/silverRealtime_test.exe`, SHA-256
  `024a52ae30b9570a42cf3a0d4130ef2e5c88c9ee5b80e4928d018930400395cf`.
  Reports: `output/perf-wakeup/final-restored-results/20261015-125848/`.
  Same 48 kHz / 128 / Balanced / Summary scenario, three runs per model,
  after the three uncontended original KS replays; no concurrent build/test.
  All six passed with zero missing/stale frames and callback overruns.

Final medians of run means (worker ratio is a median of run ratios):

| Metric | Fresh original K | Final K address | Fresh original KS | Final KS original polling |
| --- | ---: | ---: | ---: | ---: |
| Worker wall / audio duration | 0.253 | 0.262 | 0.519 | 0.539 |
| Callback mean, microseconds | 24.829 | 33.819 | 5.151 | 5.474 |
| Total idle wait mean, ms | 1.532 | 1.977 | 1.554 | 1.406 |
| Notify-to-return mean, microseconds | unavailable | 12.102 | unavailable | unavailable |
| Missing/stale frames and callback overruns | 0 | 0 | 0 | 0 |

- Final K's worst notify-to-return observation was 507 us. Its largest callback
  was 1.1761 ms, below the 2.667 ms deadline. Callback mean rose relative to the
  fresh original captures (the initial matched A/B table did not show that rise);
  do not claim lower callback cost or CPU usage. The original binary has no
  notify metric: the earlier same-binary polling comparison (K 1258.810 us vs
  16.346 us) provides the direct wake-latency A/B evidence. A longer final idle
  wait is compatible with sleeping until consumption; it is not a wake penalty.
- Final KS deliberately has zero wake-latency observations, not a measured zero
  latency. Its polling sleep and throughput are broadly consistent with the
  uncontended original replay. The successful final runs do not erase earlier
  intermittent failures or validate the rejected address/timer candidates.
- Windows Release fixture, Standalone and VST3 built successfully. All five final
  tests passed in 33.54 s: diagnostics, worker-wakeup, host-clock, native-editor
  and latency reentry. The last exercised both K and KS with real private OS
  inputs, synchronous host reentry, buffer/rate changes, state restore, firmware
  worker completion, offline audio and transition to realtime; it was not skipped.
  Local test transcript: `output/perf-wakeup/final/regression-tests.log`.
- Source-only repository audit, Python runner syntax check and diff whitespace
  check passed. Raw reports, archived executables and private assets remain
  local under ignored output paths.
- Next step: capture an OS scheduling trace for KS during a failing sustained
  section, including worker CPU time, core migrations and ready-to-run delay,
  with a matched original control and no concurrent build. Repeat on an affected
  weak CPU and in a real DAW before expanding the K change to KS.
- macOS wait behavior is unchanged; macOS build/performance remain unverified.

## 2026-10-15 — KS scheduler traces during reproduced missing audio

- Scope: measurements only. No JIT, guest, buffer, resampler or plugin runtime
  changes in this round; the production KS path remains original polling.
  No registry/policy, driver, power-plan, affinity or process/thread-priority
  changes, installs, reboots or manual deletions. User explicitly permitted UAC
  for the temporary collector; benchmark processes remained unelevated.
- Same Windows x64 / i5-13600KF / 20 logical CPUs / Release MSVC 1944 setup.
  Frozen original: `output/perf-wakeup/before/silverRealtime_test.exe`, SHA-256
  `62a747ac1be4f5d857e40bc1eac36668a9547cdcf47cd7baf1cac40d60915907`.
  Rejected address prototype: `output/perf-wakeup/after/silverRealtime_test.exe`,
  SHA-256 `fe64b2ec9f81445b2d6498420aebf790fcd7c521b3c0f4588347e9ae80843a42`.
  The latter was also run with `SILVER_WORKER_WAIT=poll` to isolate backend
  changes on one executable. It includes notification instrumentation, so it
  is not the same code as the frozen original. No experimental KS code was
  restored into the production source. Production fixture hash remains
  `024a52ae30b9570a42cf3a0d4130ef2e5c88c9ee5b80e4928d018930400395cf`.
- All cases: KS, one part, same private OS / synthetic init flash / zero wave
  assets, 48 kHz, prepared/actual 64 frames, Low profile, Summary diagnostics,
  12-second existing passage, eight-note chords after the initial note, no
  editor/state saves, one instance, isolated fixture data. Existing prepare call
  at second 6 remains. No concurrent compilation or other test suite.
  Case order: original, address, candidate-poll, address, original,
  candidate-poll, original, address. This is interleaved, not randomized.

Collector and evidence quality:

- Built-in WPR 10.0.19041 needed administrator profiling rights; ordinary and
  Codex elevated attempts failed with `0xc5585011`. After user-approved UAC it
  recorded, but `-stop` failed with COM error `0x80010106`, leaving no usable ETL.
  A 15-second smoke repeated that failure. First fixture reports are retained
  at `output/perf-scheduler/ks-20261015-130516/` but are **not scheduler evidence**.
  Microsoft documents this WPR failure in its
  [start/stop notes](https://devblogs.microsoft.com/performance-diagnostics/wpr-start-and-stop-commands/).
  No registry repair, toolkit installation or reboot was attempted.
- Used a small standalone controller via `StartTraceW`/`ControlTraceW`, with a
  private system logger GUID/name, QPC clock, CSwitch, ReadyThread, process/thread
  and DPC/interrupt events. Optional CPU samples use the existing OS interval;
  no `TraceSetInformation`, stack walk or symbol/JIT tuning. Only the collector's
  process token temporarily enables its existing profiling privilege. Automatic
  stop at 240 s or a STOP file; sequential ETL limit 512 MB. Only its own session
  handle is stopped. A five-second smoke saved a valid ETL with zero lost events.
- Minimal trace: `output/perf-scheduler/ks-native-20261015-131453/`,
  ETL 298778624 bytes, SHA-256
  `b33a7b0b880d83d3adb72c62f454c85329650548c690e2178e9727fda19405d8`.
  All eight fixture return codes were 0. Seven had no missing frames; candidate
  polling case 06 had 132 startup-only misses and zero steady misses. Thus this
  trace did not reproduce the large sustained failure. In successful steady
  windows, original had 3004–3291 dispatches vs address 6139–6436, and original
  1820–2153 logical-CPU migrations vs address 4157–4613. More frequent parking
  changes scheduling behavior; these counts alone do not establish causation.
- Trace including CPU samples: `output/perf-scheduler/ks-sampled-20261015-131949/`,
  ETL 322174976 bytes, SHA-256
  `3552be337f80bfae668f7edfa201d7e1afb679b73dfef54b36839a3c2d04a430`.
  All eight cases completed; six fixtures failed their realtime criteria and
  two passed. Both native sessions stopped successfully (`ControlTrace=0`),
  with EventsLost / LogBuffersLost / RealTimeBuffersLost all 0. WPR is stopped.
- Local `capture-index.json` records PID, exact executable hashes, UTC/monotonic
  process boundaries and diagnostic session start. Worker identified by ETW
  thread name `KS H8S / dual DSP`, qualified by process and lifetime. CPU topology
  was read, not changed: group 0, logical 0–11 EfficiencyClass=1 (P), 12–19
  EfficiencyClass=0 (E). No affinity was applied.
- Offline decoder uses `OpenTrace`/`ProcessTrace` with 100 ns system timestamps.
  TDH MOF lookup returned 1168 here; the fallback accepts only known version/size
  layouts, checked against Microsoft's [CSwitch](https://learn.microsoft.com/en-us/windows/win32/etw/cswitch),
  [ReadyThread](https://learn.microsoft.com/en-us/windows/win32/etw/readythread)
  and [PerfView parser](https://github.com/microsoft/perfview/blob/main/src/TraceEvent/Parsers/KernelTraceEventParser.cs).
  Final sampled decode emitted 5772609 scheduler/name/sample records with zero
  unsupported records. All sixteen 8-second steady windows across the two
  captures have zero worker state inconsistencies and full time accounting.
  An audit found two inconsistent transitions in the first four seconds of
  minimal case 08, leaving 8.9 us unaccounted in its full 12-second reconstruction;
  that full-window result is flagged and excluded from comparisons. The sampled
  trace's full windows have no such inconsistencies. Raw events were preserved;
  no durations were imputed. A synthetic known timeline also verified durations,
  migrations and clipping.

The following are **individual runs**, not medians or a benchmark ranking.
Steady-window scheduler metrics cover diagnostic seconds 4–12 (8 s), excluding
firmware boot. Missing frames are whole-fixture totals; startup/steady counts
come from the fixture's separate approximately four-second warm-up boundary.

| Case / backend | Missing frames (startup / steady) | Scheduled on CPU, ms | E-core share of scheduled time | Ready-to-run total, ms | Max ready delay, us |
| --- | ---: | ---: | ---: | ---: | ---: |
| 01 original polling | 0 / 0 | 4238.0 | 4.31% | 25.895 | 112.2 |
| 02 prototype address | 0 / 0 | 4515.9 | 10.04% | 48.311 | 144.2 |
| 03 prototype forced poll | 64 / 649 | 4319.4 | 4.71% | 12.487 | 588.1 |
| 04 prototype address | 0 / 29539 | 4844.5 | 23.99% | 48.994 | 122.0 |
| 05 original polling | 82 / 326299 | 7543.7 | 93.53% | 4.348 | 106.7 |
| 06 prototype forced poll | 0 / 337097 | 7561.3 | 93.65% | 2.064 | 86.0 |
| 07 original polling | 0 / 319768 | 7538.2 | 93.55% | 3.307 | 106.1 |
| 08 prototype address | 0 / 334234 | 7593.0 | 93.66% | 8.508 | 99.5 |

- Strong measured pattern: in cases 05–08 the worker spends about seven seconds
  almost continuously on E-cores (CPU 12, or CPU 13 in case 08), starting around
  diagnostic second 5. In case 04 the same pattern appears late, around second
  11.25. The 250 ms timeline is in `scheduler-analysis-with-samples.json`.
  Scheduled occupancy reaches 94.2–94.9% of a single logical CPU over the last
  8 s, versus 53.0% in successful original case 01. No change was made to force
  or prevent these placements.
- Heavy-failure ready delays are only 2.1–8.5 ms total over 8 s; their maxima
  are <=106.7 us. Thus prolonged runnable-but-unscheduled starvation is not the
  dominant time category in these captures. The worker is mostly executing.
  Slow notification is not sufficient to explain the failure across original,
  forced-poll and address variants. This does not rule out separate rare stalls.
- Scheduled time includes interrupts and is not exact thread CPU accounting.
  CPU samples provide a cross-check: 99.56–99.80% of samples attributed to the
  heavy-failure workers point to user space; only 15–33 samples per case point
  to kernel space. DPC/ISR flags are recorded separately. Observed dispatch and
  sampled priorities remain 15 in good and bad runs. No evidence here for a
  priority drop or dominant interrupt storm; sampling cannot exclude rare IRQs.
- Interpretation / hypothesis: KS lacks throughput headroom when this workload
  stays on an E-core. Address parking increases dispatches/migrations in good
  runs, but the frozen original has the same E-core failure; changing the sleep
  alone does not resolve it. Why Windows settles the busy worker there remains
  open. Placement might also be influenced by the workload, power/thermal state
  or tracing overhead. No controlled placement counterfactual has been run.
- Limitations: synthetic host, zero wave bank, single part and one machine;
  not a real DAW/driver or weak tester CPU. Sampled and minimal captures were
  sequential; enabling profiling may perturb scheduling, and the late failures
  are not a randomized estimate of failure frequency. Current reports do not
  timestamp each missing audio block, so 250 ms scheduler bins cannot prove the
  exact first missing sample. Older untraced failures remain separate evidence.
- Added reusable standalone recorder/decoder and offline analyzer plus a
  parameterized comparison runner. Production plugin targets were not rebuilt
  or changed for this measurement. Tools compiled on Windows; Python syntax,
  known-timeline calculations and whitespace audit passed. The capture used a
  local driver with the same case sequence; the parameterized wrapper has CLI
  validation, not an additional full capture matrix. Earlier raw files remain
  untouched. Raw ETLs and binaries are local/ignored, never committed.
- Next plugin investigation: read the worker's existing QoS/placement state and
  test a narrowly scoped per-worker placement policy in a separate controlled
  A/B round. Keep an unchanged original control and avoid hard-coded CPU indices.
  Do not change Windows policies, affinity or priorities as a machine-wide fix.
  No such runtime change was made now. JIT remains outside this round.
- macOS code and wait behavior are unchanged; no macOS validation in this round.

## 2026-10-15 — opt-in KS worker P-core CPU Sets A/B

- Authorized scope: test P-core placement inside the plugin only. No Windows
  settings/power/registry/driver changes, process-wide placement changes,
  new thread priorities, timer changes, JIT changes or guest/resampler/buffer
  modifications. No files were deleted. Temporary ETW used the already approved
  UAC collector; all benchmark processes remained unelevated.
- Added `ksWorkerPlacement` and an opt-in `SILVER_KS_WORKER_CORES=p` flag, read
  once per KS instance. After original boot/warm-up and existing realtime-thread
  setup, the worker enumerates Windows CPU Sets and requests the highest
  EfficiencyClass through `SetThreadSelectedCpuSets(GetCurrentThread(), ...)`.
  No fixed CPU IDs, no hard-affinity setter and no process-default setter.
  Existing explicit host CPU-set assignments are left untouched; selection
  intersects the existing process/thread hard mask and excludes CPUs reserved
  for another process. Missing APIs, non-hybrid/multiple-group topology or no
  eligible P-cores yield a diagnostic fallback, with ordinary scheduling intact.
  K and macOS retain their prior behavior. Default/unset does not call the
  placement APIs. The flag is not stored in project state or preferences.
- Added `worker_placement` JSON: requested mode, applied/fallback status,
  Win32 error and CPU-set IDs. API success alone is not proof of actual CPU
  execution: that was checked separately with ETW. On this i5-13600KF the
  discovered selection was IDs 256–267: 12 logical processors / 6 physical
  P-cores, EfficiencyClass 1. These values are observations, not constants.
  The same-binary control records status `default` and no selected IDs.
- Release/MSVC 1944, Windows x64, source `5aa9e31d06f5` + dirty changes, same
  DSP dependency pin. Fixture, Standalone and VST3 built locally. No installed
  DAW/Standalone copy was replaced. Candidate archive:
  `output/perf-placement/candidate-20261015-134427/silverRealtime_test.exe`,
  SHA-256 `7e5d583269d417ffd08abdbbedc1e966fde2329a68bf80decfd3e5e635e6b678`.
  Default and P-core tests use this exact executable. Frozen-original control
  remains SHA-256 `62a747ac1be4f5d857e40bc1eac36668a9547cdcf47cd7baf1cac40d60915907`.
- Same fixture/assets/CPU as the scheduler investigation: KS single part,
  synthetic init flash, zero wave bank, eight-note chord passage, 48 kHz,
  64 prepared/actual frames, Low, Summary, 12 s, one instance, no editor/state
  saves, original prepare at second 6. All modes retain KS's original 50 us
  polling wait. No compilation, ETL decoding or other test suite during captures.
  Each series interleaves: candidate default, P, frozen original, P, candidate
  default, P, candidate default, frozen original. No forced E-core placement.
- Sampled ETW series: `output/perf-scheduler/ks-pcores-20261015-134445/`.
  `scheduler.etl` = 315424768 bytes, SHA-256
  `0aaab01093e8f77e0d0933167bfea9bfb47faa0ea5f12b7723f4527478cf7d21`.
  Own collector stopped successfully, no lost events or buffers. Decoder emitted
  5557844 records, zero unsupported records. All full and steady reconstructions
  have zero state inconsistencies and complete time accounting (16 windows).
  The large sustained collapse did not recur during this traced series; its
  controls had smaller misses. Do not substitute older severe traces for it.
- Repeated the identical sequence without ETW:
  `output/perf-placement/untraced-20261015-134714/`. Severe misses recurred in
  two same-binary default controls and one frozen-original control, while each
  interleaved P-core run had zero steady misses. No actual per-core execution
  trace exists for this second series; successful CPU-set application is in
  each JSON report. This avoids claiming unobserved placement from API success.

Counts below are **per-run sequences**, not estimates of population failure
rates. Startup is the fixture's initial approximately four-second window;
steady is its remaining passage. Worker ratio is the median of whole-run wall
time / generated-audio ratios, not total CPU usage.

| Series / mode | Runs | Startup missing frames, each run | Steady missing frames, each run | Median worker ratio |
| --- | ---: | --- | --- | ---: |
| ETW, candidate default | 3 | 0 / 0 / 133 | 63 / 0 / 32 | 0.534 |
| ETW, candidate P | 3 | 0 / 0 / 0 | 0 / 0 / 0 | 0.524 |
| ETW, frozen original | 2 | 0 / 0 | 79 / 10 | 0.564 |
| No ETW, candidate default | 3 | 0 / 0 / 0 | 0 / 138817 / 303678 | 0.655 |
| No ETW, candidate P | 3 | 78 / 33 / 0 | 0 / 0 / 0 | 0.514 |
| No ETW, frozen original | 2 | 15 / 0 | 0 / 333411 | 0.671 |

- Direct placement evidence: all three traced P-core workers spent **100% of
  their scheduled time on P-cores**, in both full and seconds 4–12 windows, with
  zero cross-efficiency-class migrations. Steady scheduled occupancy was
  52.0–53.9% of one logical CPU. Same-binary default controls spent about
  5.1–5.4% of their scheduled time on E-cores in this particular traced series.
  Observed dispatch and sampled priority stayed 15 in both modes.
- P-mode steady dispatch counts were 2588–2762 and logical CPU migrations
  1051–1222, compared with default 3064–3162 and 1930–2108. Different placement
  changes scheduling behavior without changing the polling algorithm. Ready
  delay maxima were <=567.8 us for the P runs; no claim that every latency
  percentile improved. These are scheduled durations, including any IRQ time.
- Across all six P runs: zero steady misses, zero stale frames and zero timed
  callback overruns. Four had zero missing frames throughout; two untraced runs
  still lost 78 and 33 frames in the initial window. Fixture return 0 permits
  startup-only misses; the result is **not a claim of complete dropout freedom**.
  Reported plugin latency remained 5.33333 ms and note onset 9.52083 ms in all
  modes. Largest P-mode timed callback was 0.1210 ms vs a 1.333 ms block deadline.
  Printed P-mode peak stayed 0.123063. No bit-exact audio comparison was performed.
- Interpretation: this targeted intervention materially improves the reproduced
  steady-playback failure on this hybrid CPU. It supports the previous
  placement/throughput hypothesis more strongly than observation alone: the
  same executable with ordinary placement loses large amounts of audio, while
  P-mode's steady part remains clean. It does not isolate every factor or prove
  a fix for all tester machines. Raw summary: `output/perf-placement/comparison.json`.
- Six final regression tests passed in 34.10 s with `SILVER_KS_WORKER_CORES=p`:
  CPU selection (hybrid classes, host mask, reservations, homogeneous/default
  no-op), diagnostics, worker wakeup, host clock, native editor and latency
  reentry. The last used both private OS inputs and exercised state restore,
  rate/block changes, firmware completion, offline rendering and transition to
  realtime; it was not skipped. Transcript is beside the archived candidate as
  `regression-tests.log`. CI build/test lists include the new selection test;
  CI itself has not run here. Source audit, Python syntax and whitespace checks
  passed. Raw reports/ETLs/binaries remain local and ignored.
- Decision: retain this as an **explicit opt-in experiment**, with default
  scheduling unchanged. Startup-window losses remain to investigate. A small
  sequential single-instance synthetic test is not a production-wide rollout
  gate: no real DAW/driver, long session, four-part/multiple-instance, constrained
  host CPU-set, multi-group or weak-CPU validation yet. Power/thermal state and
  order are not controlled; CPU Sets may be overridden by OS power management.
  Do not infer a universal emulation speedup or total-system CPU saving.
- Next step: test longer real-DAW sessions and several KS instances with this
  opt-in mode, then separately investigate initial-window misses. Keep the
  ordinary-scheduling control; combine P-core placement with another wait
  strategy only as a separate experiment. No JIT work in this round.
- macOS build/performance remain pending; no Windows CPU IDs are persisted into
  plugin state, and the macOS placement path performs no Windows action.

## 2026-10-15 — OsTIrus comparison (source review only)

- Trigger: the affected tester reports good OsTIrus performance on the same
  computer. This is user-reported evidence; CPU/model, host, buffer, OsTIrus
  version/ROM model and exact Silver-K symptom are still unknown. Asked whether
  "problem starting" means failure/hang during loading or playback stutters.
- Local source: `5aa9e31d06f5efb203a80193a74a1a9fa9caeeee`, dirty diagnostic/
  wakeup/placement tree described above. Windows x64 source review; no new
  builds, runtime changes, ETW sessions or performance captures in this round.
- External reference: Gearmulator **tag 2.2.25**, commit
  `06d0c0c9c02e6b3e0b9768295d003905cc8e4e08`, DSP submodule
  `e643fa89f0c11147963b315c0bd8afb283bba468`. Retrieved tagged files through
  GitHub; did not substitute moving `main` for the release. Local ignored
  snapshots, source URLs and Git blob identities:
  `output/research/ostirus-2.2.25-20261015/manifest.json`.

| Area | OsTIrus TI/TI2, tagged source | Current KS |
| --- | --- | --- |
| DSP ownership | `DspSingle::startDSPThread` creates one `DSPThread` per DSP; `Device::bootDSPs` starts both | One `Device` worker owns H8, DSP B and DSP A |
| Controller | `virusLib::Microcontroller` implements MIDI, patches, state and HDI08 protocol in native C++ | Original H8S OS executes via `h8->step()` with peripheral ticks, 25 MHz virtual clock |
| DSP coupling/output | `processAudioTI` supplies inputs to both DSPs and unpacks/adds their ESAI outputs on the host | A/B exchange serial frames; `Board::advance` advances B then A and services a shared ESAI slot clock |
| Work granularity | DSP worker calls trampoline `exec(&m_dsp, 128)`; device processes audio in blocks | H8 budget defaults to 128 guest cycles, then DSPs catch up through `step(1)`; output is exchanged per frame |
| Startup | Boots DSPs and initializes native controller state; factory patches read directly from ROM | Boots H8/DSPs and renders single/multipart warm-up passages before signaling readiness |

- Model qualification: TI and TI2 create `DspMultiTI`, with A and B; Snow creates
  `DspSingleSnow`, so an OsTIrus Snow run is not a dual-DSP control.
- The native controller is a major distinction: the name `Microcontroller`
  does not mean OsTIrus runs an instruction emulator for the original controller
  in this path. It parses MIDI/SysEx, queues DSP words and manages patches in C++.
  KS's controller cost is additional to its two DSPs. Do not infer that all
  Gearmulator products use the same controller strategy.
- KS's `step(1)` is an instruction-budget request, **not proof of one emulated
  instruction per call**: `execJit()` can execute a whole compiled block.
  Frequent C++ scheduling/peripheral checks are a source-level candidate cost;
  no new timing establishes their share of a failed run.
- JIT differences were inspected read-only: TI sets DO limit 32 and inherits
  linked blocks / uncapped instruction blocks from the pinned defaults; KS
  explicitly uses DO limit 2, block limit 32 and linking off. The two projects
  also pin different DSP revisions (Silver-K `44353be2f0156cac970c6ab4f467ea1998b40605`).
  No JIT edits, dependency updates or performance attribution to these settings.
  Batched trampoline execution is part of that execution path and remains
  outside the currently authorized non-JIT experiment.
- Interpretation: good OsTIrus behavior argues against dismissing this tester's
  computer as generally unsuitable. Two DSPs do not establish equivalent host
  work or a comparable single-core deadline. OsTIrus can use two cores for DSP
  work; KS adds both DSP costs and H8 work on one core. This fits the local
  throughput/placement evidence, but is not a measured explanation of this
  tester's failure and does not establish a speedup factor.
- Do not copy OsTIrus's output mixing or split KS into independent workers
  directly: KS has live inter-DSP serial dependencies and H8/HDI08 interactions.
  Such a change needs a separate timing/ownership design and audio correctness
  validation. Replacing H8 with native controller logic would change the
  emulation approach and is not proposed within this scope.
- Startup distinction: KS's constructor waits on `startCV` until boot and
  warm-up complete; current performance sessions attach only afterwards.
  Neither the previous P-core steady-state result nor its startup-window audio
  misses measure plugin construction/boot time. P-core selection currently
  happens after warm-up, so it cannot establish a faster firmware boot.
- Next non-JIT measurement: on the tester's machine, hold host/driver, host
  sample rate, actual block, instance count and UI state fixed. Record OsTIrus
  version, TI/TI2/Snow model, DSP clock percentage, internal latency and resampler
  settings; compare silence and a fixed eight-note MIDI passage with KS, marking
  different patches/firmware as different workloads. Capture per-thread CPU,
  actual core placement, callback deadlines, audible losses and KS diagnostics.
  Identical MIDI alone does not make different synth algorithms equally costly.
  If failure is before ready, first measure boot/warm-up phases separately;
  current diagnostics cannot observe them. No OsTIrus ROM or matched capture was
  supplied for this review, so no OsTIrus CPU/dropout/startup numbers are claimed.
- macOS: source architecture comparison only; no compilation or runtime result.
- Primary sources: [DSP creation, boot and configuration](https://github.com/dsp56300/gearmulator/blob/2.2.25/source/axel/virusLib/device.cpp),
  [DSP worker creation](https://github.com/dsp56300/gearmulator/blob/2.2.25/source/axel/virusLib/dspSingle.cpp),
  [TI output routing](https://github.com/dsp56300/gearmulator/blob/2.2.25/source/axel/virusLib/dspMultiTI.cpp),
  [native controller](https://github.com/dsp56300/gearmulator/blob/2.2.25/source/axel/virusLib/microcontroller.cpp),
  [pinned DSP worker](https://github.com/dsp56300/dsp56300/blob/e643fa89f0c11147963b315c0bd8afb283bba468/source/dsp56kEmu/dspthread.cpp).

## 2026-10-15 — tester clarification: KS playback on Ryzen

- User report: K-Station has no problems on the affected computer. KS opens
  but its audio stutters as if the processor were overloaded. Format: VST3;
  OS: Windows 11; CPU described as an older Ryzen, exact model unspecified.
  OsTIrus runs well there. DAW, driver, sample rate, actual block, patch/parts,
  plugin versions and OsTIrus TI/TI2/Snow model are not yet known.
- This resolves the earlier startup ambiguity: prioritize sustained/episodic
  playback losses rather than firmware boot diagnostics for this report.
  Preserve the previous source review as historical context.
- The Intel i5-13600KF measurements establish a local placement improvement;
  they do not explain the Ryzen report. On a homogeneous CPU, the current
  P-core experiment deliberately does nothing. Do not recommend enabling it
  as a general Ryzen fix or assume the unidentified Ryzen has Intel P/E classes.
- Working hypothesis: KS's single worker has less throughput headroom because
  it executes H8 and both DSPs sequentially. K working and OsTIrus working are
  useful controls, but neither proves the cause: firmware, synth workloads,
  internal latency and execution architecture differ. Aggregate CPU percentage
  alone cannot distinguish one busy worker from idle capacity on other cores.
- Next capture on the affected VST3 setup: matched K/KS summary sessions,
  then KS detailed probes, with fixed host/driver/rate/block and a repeatable
  passage. Correlate missing frames with queue depth, worker wall/audio ratio,
  H8/DSP timings and sleep duration. Ratios/timers include descheduling; if they
  are high, distinguish actual thread CPU work from scheduler delay before
  attributing the result to computation. Use OsTIrus as an additional control
  with its exact model, clock and internal latency recorded.
- No new measurements or reproduction on Ryzen; no code, JIT, priorities,
  buffers or Windows settings changed. Updated journal only; whitespace check
  passed. Windows/local Intel and pending Ryzen/macOS evidence remain separate.

## 2026-10-15 — affected machine identified: Ryzen 3 3100

- User supplied: AMD Ryzen 3 3100, AM4 motherboard, 24 GB RAM, SSD storage,
  Windows 11. Previous clarification identifies VST3, working K-Station and
  OsTIrus, with playback stutters only in KS. DAW/driver, rate/block, patch/parts,
  plugin versions, OsTIrus ROM model and actual runtime counters remain unknown.
- Verified manufacturer specifications: Zen 2, 4 physical cores / 8 hardware
  threads, base 3.6 GHz / maximum boost up to 3.9 GHz. These are specifications,
  not observed operating frequencies or a benchmark of the affected computer.
  Source: [AMD launch specifications](https://ir.amd.com/news-events/press-releases/detail/942/amd-expands-3rd-gen-amd-ryzen-desktop-processor-family-unleashing-powerful-zen-2-core-for-the-mainstream).
- This model has no Intel-style P/E-core distinction. The existing opt-in P-core
  selector is not a remedy for this report. Keep the Intel experiment's result
  separate; do not attribute the Ryzen failure to E-core migration.
- Four physical cores make a two-DSP-worker experiment relevant if sufficient
  independent DSP work exists. Eight SMT threads are not eight physical cores,
  and synchronization/dependency costs still determine the achievable gain.
  Native-controller replacement is not a prerequisite for parallel DSP workers.
- Current single-worker architecture makes a single-core throughput constraint
  plausible; hardware identification alone does not prove it. No evidence yet
  identifies RAM capacity or SSD throughput as the cause. Do not infer memory
  channel layout, timings, thermal state or background load from capacity alone.
- Continue with the affected-machine K/KS captures described above, measuring
  per-thread CPU versus wall time and queue starvation before choosing H8
  replacement or a multi-worker implementation. No performance result on Ryzen,
  new build or code/JIT/system change in this update; journal whitespace checked.

## 2026-10-15 — opt-in KS dual-DSP workers with original H8

- Authorized scope: user explicitly requested two KS DSP workers, original H8,
  an off switch, original controls, measurements and journal updates. No JIT,
  dependency revision, firmware, Windows settings, buffers, virtual clocks,
  priorities of existing threads, resampling or MIDI scheduling changes.
- Architecture: default stays exactly the sequential DSP B -> A catch-up path.
  `SILVER_KS_DSP_THREADS=2` enables two persistent DSP helpers after sequential
  boot/warm-up and initial queue fill. H8 remains on the coordinator. At each
  existing `advance(128)` guest-cycle boundary, both helpers independently
  reach their original per-DSP target using the unchanged `step(1)`/JIT path.
  The coordinator joins both, then runs shared ESAI TX/RX in the original order.
  It never advances H8, accesses HDI08, snapshots state or services serial IO
  while a DSP slice is active. Helpers never directly run the shared serial
  callbacks (their ESAI clocks remain disabled as before).
- New `ksDspWorkers` uses release/acquire command/completion handoffs, bounded
  256-pause spinning then blocking. Windows dynamically resolves WaitOnAddress/
  WakeByAddressSingle; unavailable APIs and non-Windows use a condition variable.
  Idle helpers block when the coordinator sleeps. Existing live KS priority and
  realtime parameters are used for the new helpers; no process/global scheduler
  policy is changed. Rate/block parameter updates are handed off between slices.
  Both helpers are drained before propagating an execution exception, and joined
  before Board/DSP/queue destruction. No unbounded busy spin or larger guest slice.
- Off switch: unset the variable or set `1` before creating/reloading the KS
  instance. No persisted preference/state change; K ignores the flag. No helper
  threads are created in default mode. The previous P-core option targets only
  the coordinator; it is not a claim of DSP-helper placement. All captures in
  this round clear placement/wakeup overrides and use ordinary scheduling.
- Diagnostics: root/session `dsp_execution_backend` verifies `sequential` or
  `two-workers-slice-barrier`. New `dsp_barrier_sampled` measures dispatch plus
  both-DSP completion wall time in Detailed (stride 256), including execution
  and scheduler delays. DSP timers now have their respective helper as sole
  writer in parallel mode. Timers overlap; do not sum them. Worker ratio remains
  coordinator wall/audio duration, including waits, not total CPU usage.
- Build identity: local Windows x64 Release, MSVC 19.44/NMake, configured source
  `5aa9e31d06f5`, dirty tree, DSP pin unchanged at
  `44353be2f0156cac970c6ab4f467ea1998b40605`. Standalone, VST3, realtime,
  worker/equivalence and regression targets built. Frozen pre-experiment fixture
  `output/perf-dsp-threads/before-20261015-141612/silverRealtime_test.exe` SHA-256
  `7e5d583269d417ffd08abdbbedc1e966fde2329a68bf80decfd3e5e635e6b678`.
  Candidate `output/perf-dsp-threads/candidate-20261015-142039/silverRealtime_test.exe`
  SHA-256 `93f4274bbc1442d4abc3f14610ed477fa50722947599073f98775c37d5005ef4`.
- Correctness before speed testing: `ksDspEquivalence_test` with private KS OS
  2.1.02, synthetic Flash and zero wave bank compared **65,536 native frames,
  all 24 streams, bit for bit** between sequential and parallel Boards. Passed
  Single, sustain/release, Program Change, four-part program load/notes and All
  Sound Off. H8 cycles/PC, both DSP cycle/instruction counters and serial queue/
  underrun/overflow counters agreed every 1,024 frames; final controller state,
  saved user Flash and consumed MIDI counts agreed. Non-silent peak 1,356,564;
  rolling word hash `3852514813786200426`. Log: `equivalence.log` in that root.
  This passage is not exhaustive firmware/patch/audio-input validation.
- ROM-free worker test verifies two distinct overlapping helper threads,
  10,000 handoffs, payload visibility, failure drain/recovery, wake after idle
  and immediate idle destruction. Regression run with threads=2: seven tests
  passed in 54.14 s (worker, placement, diagnostics, wakeup, clock, native editor,
  latency/reentry). Private latency test was **not skipped**: both K and KS,
  rate/block changes, state restore, asynchronous completion, offline rendering
  and return to realtime passed. Transcript: `regressions.log`. CI target/test
  lists include workers and equivalence (the latter skips without private args).

Measurement scenario: local i5-13600KF, 14 physical / 20 logical CPUs, Windows
x64 (JUCE OS label Windows 10), ~32 GB RAM. Existing 12 s fixture, 48 kHz, actual
block equals prepared block; 64/Low and 128/Balanced; one KS instance/part, fixed
initial note and eight-note chords, synthetic zero wave bank, no editor, state
save off, prepare again at second 6. No real audio driver/DAW is exercised.
For each block, interleave candidate sequential, parallel, frozen, parallel,
candidate sequential, parallel, candidate sequential, frozen. No concurrent
build/tests/trace analysis during captures. Power/thermal state not controlled.

| Scenario / variant | Steady missing frames per run (after initial ~4 s) | Median worker wall/audio ratio | Median whole-process CPU seconds, including boot |
| --- | --- | ---: | ---: |
| 64 Low / frozen original | 208103, 308468 | 0.8030 | 11.883 |
| 64 Low / candidate sequential | 328408, 315637, 335827 | 0.8056 | 11.984 |
| 64 Low / candidate two workers | **0, 0, 0** | **0.5664** | 18.063 |
| 128 Balanced / frozen original | 269430, 283221 | 0.8404 | 12.828 |
| 128 Balanced / candidate sequential | 299239, 306434, 316764 | 0.8086 | 12.109 |
| 128 Balanced / candidate two workers | **0, 0, 0** | **0.5678** | 19.250 |

- All six Summary parallel runs had zero **startup as well as steady** misses,
  stale frames and timed callback overruns. All 16 runs had zero timed callback
  overruns. One frozen 128 run additionally lost 88,472 frames in the initial
  window; other controls' startup counts were zero. Failed controls returned
  fixture code 1 for audio misses, not crashes. Reported latency stayed 5.33333
  ms (64/Low) or 10.6667 ms (128/Balanced) across all variants, with note onset
  9.52083 / 14.7292 ms respectively. Parallel peak printed 0.123063 in all six.
- Important CPU limitation: process CPU seconds include identical sequential
  boot/warm-up plus all threads for the 12 s passage. Parallel costs more CPU
  in these captures. Failed controls also produce/deliver less useful audio;
  these numbers are not equal-work CPU-efficiency or power measurements, and
  the lower worker wall ratio is not a claim of lower total CPU usage.
- Raw matrix: `output/perf-dsp-threads/20261015-142039/`; derived sanitized local
  summary `output/perf-dsp-threads/summary.json`. Reproducer:
  `tools/run_dsp_thread_comparison.py` with private input arguments. It records
  hashes, PID, UTC, report backend, process CPU time and fixture summaries;
  runs ordinary unelevated processes and changes no system configuration.
- Detailed follow-up at 128/Balanced, one run per candidate mode:
  `detailed/20261015-142504` sequential lost 294,542 frames, ratio 0.8082;
  `detailed/20261015-142520` parallel lost zero, ratio 0.5643. Mean sampled wall
  microseconds: sequential H8/A/B/serial = 0.971/1.690/1.569/0.198;
  parallel = 0.879/1.562/1.464/0.248, with dispatch-and-both-DSP barrier
  **2.169 us** (max 98.3 us). Barrier overlaps A/B execution. These deterministic
  samples include descheduling and are not a precise instruction-cost breakdown.
  No ETW capture in this round; actual core placement and scheduler contribution
  are not isolated from computational parallelism. No universal speedup factor.
- Decision: retain the implementation **off by default**, as an experiment worth
  testing on the Ryzen 3 3100 in Windows 11/VST3. Seven parallel captures were
  clean locally, including one Detailed; correctness and regression checks pass.
  This does not establish that the tester's failure is fixed. Need longer real
  DAW/driver sessions, varied patches/four-part/multiple-instance load, startup/
  shutdown stress and actual per-thread placement/CPU evidence on that machine.
  H8 replacement is not required to try this architecture.
- Repository audit (537 tracked files), Python syntax and whitespace checks
  passed. Source snapshots/reports, extracted private update and binaries stay
  local/ignored. Windows/macOS share these notes, but macOS compilation/runtime
  and non-Windows CV performance remain **unverified**. No release/install/commit
  or Windows configuration change was performed.

## 2026-10-15 — equal-work CPU cost of the two-DSP-worker experiment

- Request: measure CPU saving, distinguishing wall-time speed from total CPU
  work. Production emulation and JIT were **not modified** in this round.
  Added measurement-only targets `ksCpuCost_test` and `silverCpuCost_test`, plus
  `tools/run_cpu_cost_comparison.py`. Windows settings, placement, existing
  priorities, guest clocks/slices, buffers and DSP dependency pin are unchanged.
- Primary valid measurement: native Board executes original H8 and both DSPs,
  in one ordinary benchmark process. Same source/binary for both modes; read
  `SILVER_KS_DSP_THREADS=1`/`2`, boot sequentially, send identical patch setup and
  eight-note warm-up, enable helpers only after that warm-up, settle another
  4096 native frames, then measure the passage. No host consumer busy-wait,
  resampler, pacing, diagnostics session, report writing or file IO inside the
  interval. The native coordinator uses the existing live KS priority, with
  helper priorities unchanged from the experiment. No affinity or OS policy set.
- Work: **544,085 native frames**, all 24 integer streams, exactly
  12.0000039385 s at 45,340.4017857 Hz. Single-part synthetic Init/zero-wave-bank,
  original private KS OS 2.1.02. First note at 0.1 s, release at 1.1 s, then
  eight-note chords every 40 ms from 1.12 s, fixed absolute native-frame events.
  Original UART, H8, serial clocks and DSP catch-up remain in use. This passage
  is separately defined from the earlier 12 s host fixture; do not merge their
  numbers or claim an identical patch/MIDI benchmark across the two harnesses.
- Counter: **GetProcessTimes user + kernel**, summing CPU consumed by every
  process thread, including H8/coordinator, both DSP workers, their synchronization
  and the identical small integer-hash loop. Reads only; no scheduler/timer
  configuration. Independent steady-clock wall time. Boot/setup and warm-up
  measured separately and excluded. GetProcessTimes accounting is quantized;
  these multi-second intervals avoid interpreting sub-millisecond differences.
- Platform: same local Intel i5-13600KF (14 physical / 20 logical), Windows x64,
  JUCE label Windows 10, MSVC 1944 Release/NMake, source `5aa9e31d06f5` dirty,
  DSP pin `44353be2f0156cac970c6ab4f467ea1998b40605`. Native fixture SHA-256:
  `63a552cd6345297598d75726493bca3dd529f966d8432f740eced7470acbc54e`.
  Archived alongside reports at `output/perf-cpu-cost/20261015-144250/`.
  `source-sha256.json` records measurement/source identities. No concurrent
  build, other benchmark, test suite or trace analysis during these six captures.
- Order: sequential, parallel, parallel, sequential, sequential, parallel;
  three runs per mode, ordinary Windows core selection, no P-core override.

| Mode | Total process CPU seconds per run | Wall seconds per run | Median CPU / median wall |
| --- | --- | --- | --- |
| Sequential | 5.875, 5.875, 5.875 | 5.8861, 5.8643, 5.8724 | **5.875 CPU s / 5.8724 s** |
| Two DSP helpers | 13.1094, 13.2969, 13.5000 | 4.5138, 4.4745, 4.5203 | **13.2969 CPU s / 4.5138 s** |

- Result: **no CPU saving**. Median CPU cost increased **126.33% (2.2633x)**.
  Median wall duration decreased **23.14%**, equivalent to **1.3010x throughput**
  for this complete fixed-work render. Three active threads finishing sooner
  can consume more CPU seconds than one thread. These are not total-PC CPU
  percentages, power readings or a prediction of DAW CPU-meter values.
- Equal work verified in **all six** reports: no discarded/substituted host
  frames (native pull rendering has no missing-frame fallback); hash of all
  24 streams `8211242178193234287`, peak 5,145,840; identical final H8 cycles
  400,303,881 / PC 14,196; DSP A/B cycles 2,537,244,788 / 2,530,291,470 and
  instructions 1,912,864,755 / 2,148,277,550. `verification.json` stores the
  common signature. `summary.json` marks `valid_equal_work=true` and computes
  percentage changes only for complete, hash-matching captures.
- Interpretation: the experiment has a substantial CPU-efficiency cost even
  when both modes render the same complete audio. Frequent per-slice barriers
  and bounded active waits are plausible sources of extra work, along with
  placement/frequency/cache effects. Their individual shares are **not measured**;
  no ETW/core-placement or per-function CPU profile was collected in this round.
  Do not attribute all 126% to one mechanism or infer the same overhead on Ryzen.
- Exploratory full-Processor harness: `silverCpuCost_test` measures 576,000 host
  frames at 48 kHz, separately counts the caller via GetThreadTimes and excludes
  boot/warm-up. Ran offline 128/Low and paced 512/Maximum, three per mode each,
  with diagnostics off. Raw `output/perf-cpu-cost/20261015-143810/` is retained,
  but **excluded from the strict saving calculation**: hashes differ even across
  repeated sequential runs despite zero missing frames. Exact host-state/event
  reproducibility needs investigation; no cause is asserted. Offline caller
  queue-yield time also dominates total CPU, so process-minus-caller is recorded
  as background work, not an exact emulator-only counter. Both host scenarios
  have `valid_equal_work=false`;
  their apparent savings/speed ratios must not be promoted to a result.
- Earlier preflight at `20261015-143542` stopped before measurement because the
  new fixture requested a missing parameter (`filter_freq` instead of `cutoff`).
  Corrected only the measurement fixture; retained logs, no result inferred.
- Validation: native measurement target built; all six native cases returned 0,
  passed frame/hash/guest-counter checks. Python syntax, whitespace and tracked
  source-only audit passed. No production code changed, so the prior eight
  worker/audio/regression checks remain the production validation; they were
  not needlessly repeated here. Full-Processor fixture is exploratory, not a
  validated DAW CPU benchmark. Binaries/private assets/reports remain ignored.
- Decision: keep two DSP workers **off by default**, as a deadline/throughput
  experiment with a measured CPU cost. Next optimization candidate is reducing
  synchronization/active-wait cost while preserving exact guest barriers and
  audio; this round does not implement it. Real VST3/driver, multiple-instance,
  four-part, Ryzen 3 3100 and macOS measurements remain pending.

## 2026-10-15 — H8-to-both-DSP capture and first C++ controller prototype

- Request: branch **Optimized**, record KS H8 exchange with both DSPs, start an
  optional native-controller prototype and compare behavior/audio/CPU against
  original H8. Created that exact branch from the existing dirty checkout;
  previous work preserved. No JIT, DSP dependency, Windows policy, priority,
  affinity, guest slice target, serial order or audio buffer change.
- Scope delivered: bounded original-bus recorder plus a **finite recorded C++
  controller**. Playback executes the original port transactions and checks
  every read result and write acceptance without executing H8 instructions.
  Actual H8 slice endpoints drive the existing DSP/serial scheduler. This
  establishes a comparison fixture and protocol corpus, not a finished live
  controller like OsTIrus's. No plugin setting enables it; normal K/KS still
  run their original controller. Opt-in is through explicit Board API/private
  fixture. A fresh original-H8 Board disables it.
- Guards: original H8 boot/preparation required, matching deterministic prefix
  fingerprint, trace validation/checksum, original 128-cycle budget, terminal
  mismatch/exhaustion, rejected live MIDI and unavailable state/Flash/boot API.
  H8 stays allocated and frozen during replay; no switch back to its stale
  state. Fingerprint is a partial compatibility check (selected RAM/IO, assets,
  clocks, serial data and queue sizes), **not a full machine snapshot**.
  Complete DSP register/memory equivalence is not claimed.
- Recorder retains controller cycle/PC, DSP A/B, host byte offset/value,
  read/write/acceptance and slice boundaries. It reserves at most 3,000,000
  events and 1,000,000 slices (~68.7 MiB vector payload); overflow invalidates
  the tape. File IO and checksumming happen outside the measured interval.
  Portable explicit little-endian `KSHTR001` format; existing files refused.
  Raw tapes contain private firmware/preset material and remain ignored.
- Source identity: `5aa9e31d06f5` dirty, branch Optimized, Windows x64 local
  i5-13600KF (14 physical / 20 logical), MSVC 1944 Release/NMake, unchanged DSP
  pin `44353be2f0156cac970c6ab4f467ea1998b40605`. Main comparison binary SHA-256
  `bc587c65064763a535eabf94bc9fa5dc22201cce8a981b65e0d6b3e14faeb837`.
  Binary archived with `source-sha256.json` and raw reports at
  `output/perf-controller/20261015-150503/`. Normal private KS OS 2.1.02,
  synthetic Init Flash/zero wave fixture; no private asset committed.
- Work: **90,681 native frames**, 2.0000116 s at 45,340.4017857 Hz, all 24
  integer streams. Single and four-part passages, eight notes total per chord,
  100 ms chord changes, sustain, original program change and filter control.
  H8 boots/prepares/warms each fresh process; both DSPs sequential for this
  primary comparison. No host pacing, resampling or missing-frame fallback.
  Distinct workload from the earlier 12 s CPU/DSP-thread fixture.
- CPU is Windows **GetProcessTimes user + kernel for the whole process**;
  wall uses steady_clock. Boot/setup, loading/saving tapes, fingerprinting,
  statistics/report formatting excluded. Recording run excluded from savings.
  All 15 processes succeeded: boot capture, two passage captures, six H8 and
  six replay runs. Three interleaved pairs per passage: H8/replay, replay/H8,
  H8/replay. No concurrent build/test/trace analysis during these captures.

| Passage / mode | CPU seconds per run | Median CPU s | Median wall s |
| --- | --- | --- | --- |
| Single / H8 | 1.000000, 0.984375, 1.046875 | **1.000000** | 1.0034873 |
| Single / recorded C++ | 0.750000, 0.765625, 0.765625 | **0.765625** | 0.7583863 |
| Four parts / H8 | 0.968750, 0.984375, 0.984375 | **0.984375** | 0.9891197 |
| Four parts / recorded C++ | 0.734375, 0.750000, 0.734375 | **0.734375** | 0.7389352 |

- Result: recorded playback CPU cost fell **23.4375% Single / 25.3968% four
  parts**, wall duration fell **24.4249% / 25.2937%**. CPU accounting is
  quantized (15.625 ms observed), intervals short and repeat count three.
  These are CPU seconds for equal rendered work, not total-PC or DAW meter
  percentages. No ETW placement/frequency or per-function profile collected.
- Audio/behavior: every H8/replay run matched its recorded reference's frame
  count, all-24-stream integer hash/peak, virtual controller endpoint, both DSP
  cycles/instructions and serial queue depth/underflow/overflow. Single hash
  `58781405289897816`, peak 6,653,235; four-part hash `13074848859789201745`,
  peak 4,715,204. Every replay checked **736,364 / 537,112** port transactions
  and kept original H8 cycles frozen. Serial depths [1,1], no under/overflow.
  This is exact sample comparison by hash, not a listening test or complete
  live-state comparison. The early raw aggregate label `exact_audio_and_dsp_state`
  means these counters/checks only; new runner uses `exact_audio_and_dsp_counters`.
- Captures: boot 1,663,623 events / 570,993 slices, cycle 0..75,460,599;
  Single 736,364 / 374,913, 20,779,792 bytes; four-part 537,112 / 372,896,
  17,543,352 bytes. Boot recording is retained for protocol analysis only;
  it has not been replayed without H8. Boot recording CPU/wall includes
  instrumentation and is not an improved-boot result.

| Capture | DSP A status reads / accepted 24-bit low-byte writes | DSP B status reads / accepted low-byte writes |
| --- | --- | --- |
| Boot | 80,820 / 8,146 | 1,277,461 / 82,028 |
| Single | 207,032 / 17,714 | 285,982 / 23,899 |
| Four parts | 121,365 / 10,434 | 251,427 / 20,854 |

- Observed: status offset 4 polling, ICR offset 0 writes and 24-bit data
  bytes at offsets 10/12/14 dominate this corpus. Zero accepted CVR bit-7
  commands in these traces. Do not assume all controller operations are CVR
  interrupts; next work must decode ICR flags/data handshakes and actual
  payloads. Register counts do not yet identify high-level command meanings.
- Interpretation: eliminating H8 execution is a promising CPU candidate in
  these passages. Recorded commands precompute firmware decisions and read
  contiguous tape memory; they do not implement native MIDI/patch decisions.
  A live controller may have different CPU/cache/synchronization costs and
  different functional coverage. No claim that KS stutters disappear, no
  predicted Ryzen 3 3100 saving, and no extrapolation to real VST3/multiple
  instances, arbitrary patches/waves or macOS.
- Completion validation: Release Standalone and VST3 rebuilt successfully.
  Original-H8 `ksDspEquivalence_test` passed with its prior reference hash
  `3852514813786200426` / peak 1,356,564 for 65,536 native frames (Single,
  sustain, program change, four parts, H8/DSP clocks, serial and saved state).
  Eight CTests passed: controller trace, DSP workers, worker placement,
  diagnostics, wake-up, host clock, native editor and latency/reentry (33.99 s
  total); private K/KS OS provided, fresh local data directory, DSP helpers
  requested. No private-ROM skip used for the latency check. Python syntax,
  whitespace and source-only tracked-file audit passed; raw files ignored.
  NMake's localized include scanning does not reliably rebuild header consumers:
  explicitly refreshed Board consumer timestamps and rebuilt before regression
  and plugin checks. The primary native fixture directly constructs Board and
  was already freshly compiled when measured; archived binary unchanged.
- Independent compatibility check, `output/perf-controller/20261015-151301/`:
  both passages also passed H8/recorded comparison with **two DSP workers**,
  one pair each, identical audio hashes, port counts, DSP and serial counters.
  Rebuilt fixture SHA-256
  `69efae8d558ac6c1460dc3611e2c2e791c7588f8088a8593c43be5abfac0f70b`.
  Do not pool these single pairs into the primary sequential-DSP CPU estimate.
  Pure trace test added to both CI build/CTest lists; CI itself not run here.
  Windows build/test results do not validate macOS; source/file format portable
  by construction, macOS compile/audio pending and CPU counter not implemented.
- Next step: infer the observed ICR/flag/24-bit protocol and implement live
  MIDI-to-DSP command logic incrementally, with original H8 as oracle and
  shadow comparison before enabling any replacement in the normal plugin.
  LCD, MIDI output, arbitrary SysEx, state/Flash and save/restore remain
  unimplemented in recorded playback.

## 2026-10-15 — first live C++ Note/CC decoder, original H8 handlers retained

- Request: start live KS MIDI notes/basic CC controller, compare commands and
  sound with H8, keep opt-in, no JIT changes. Work stays on **Optimized** and
  preserves previous changes. Implemented `ksLiveMidiController`: a handwritten
  C++ port of the verified OS 2.1.02 byte-decoding stage for Note On/Off and CC.
  It processes new live bytes/running status, not precomputed recordings.
- Explicit boundary: this is a **hybrid native front end**, not a finished
  native voice/controller implementation. Original H8 UART, channel/part
  dispatch, keyboard range, velocity-zero handling, sustain actions, voice
  allocation, CC-to-parameter calculations and DSP writes still execute.
  Unsupported message types retain their original H8 path, including program,
  pressure, bend, SysEx and system messages. No claim that H8 can be stopped,
  that it was removed from the live render or that the old replay saving applies.
- Why this stage: a Note On produces a substantial DSP voice-parameter upload,
  not a simple MIDI packet. The earlier tapes also contain periodic DSP state
  reads. Replacing those decisions prematurely would confound command/audio
  comparison. The live decoder preserves firmware ABI, guest registers/flags,
  call state, instruction fetch/data clocks and interrupt boundaries, then
  hands completed messages to original handlers. A Board-local CPU adapter
  exposes existing bookkeeping; shared H8 core and DSP JIT code/config unchanged.
- Enable: worker-local `Board::enableLiveMidiController()` after original boot,
  or process-local `SILVER_KS_CONTROLLER=native-midi` for a fresh plugin instance.
  Normal/unset keeps original H8. Explicit Board disable is valid at quiescent
  boundaries because partial message state stays in the guest ABI. Environment
  switch is not saved or persisted to Windows. Recreate an instance without it
  for the default. Native/replay are mutually exclusive. No clocks, buffers,
  resampling, priorities, placement or OS settings changed.
- Diagnostics records `controller_backend` at root/session, independently of
  DSP-worker mode. Existing H8 wall bucket includes native decoding in hybrid
  mode; it does not isolate H8 instruction execution. Private fixture reports
  native bytes/messages/boundaries and Note On/Off/CC coverage.
- Private differential test: `ksLiveMidiController_test` runs C++ and independent
  original-H8 decoding against isolated memory buses. Initial exhaustive pass:
  **12,336 bytes / 6,144 messages / 308,688 boundaries**, all 16 channels,
  all 128 data values with varied velocities/control values, running status
  and varied starting flags. Every register/flag/PC/clock, call bookkeeping,
  bus-pair state and each read/write match. Interrupt arbitration and unknown
  firmware/status guards checked. Later completion pass adds alternating
  native/original execution inside messages (results below).
- Development failures retained: `20261015-152522` differed in audio/commands
  from an extra two guest cycles on a short subroutine call. `primary-run2`
  then caught a trace discrepancy; isolated differential testing found an
  incorrect 6/8-byte boundary for a status store. Both corrected before valid
  results. No failed case was counted as equivalent or as a speedup.
- Initial correct matrix `output/perf-live-controller/20261015-152907/` passed
  all bus/audio comparisons, but native dispatch was checked by function call
  on every H8 instruction. CPU medians were 1.6–3.2% above H8, with a 2.0625 s
  multi H8 outlier retained. Moved the native/default branch outside the loop,
  restricted native calls to the decoder range; default H8 loop now has no
  per-instruction native dispatch. The following matrix measures that revision.
- Primary valid matrix: **`output/perf-live-controller/20261015-153151/`**,
  local Windows x64 i5-13600KF (14 physical/20 logical), MSVC 1944 Release/NMake,
  source `5aa9e31d06f5` dirty, branch Optimized, unchanged DSP dependency pin
  `44353be2f0156cac970c6ab4f467ea1998b40605`. Archived native fixture SHA-256
  `90674c03ecaf19b7bdd856e3cfd23d1dfc3d173eaa2cab106f86e36fdd5568b7`,
  controller source hashes retained alongside it. Private KS OS 2.1.02,
  synthetic Init/zero wave fixture. Raw tapes and binaries stay ignored.
- Protocol/audio: original and live modes both receive the same actual MIDI.
  `record` and `live-record` tapes match **byte for byte**, including every
  read response/write acceptance, cycle/PC and slice clock. This compares
  736,364 Single / 537,112 four-part / 579,064 MIDI-scenario bus events. Single
  and multi tapes also equal the prior original-H8 reference tape hashes.
  All 24 integer audio streams match by hash, peaks, frames and final
  H8/DSP clocks/instructions/serial counters in all unrecorded comparisons.
- Work: 90,681 native frames (~2 s) per case. Same Single/four-part, eight-note
  chord passage as prior controller fixture (sustain/program/cutoff). New `midi`
  passage adds fragmented messages, running status, repeated pitches, zero
  velocity, CC 1/7/10/11/64/105/120/123, program and F1/F8 fallback paths.
  Single/four-part each exercise 299 native messages (152 On, 144 Off, 3 CC),
  24,219 native boundaries. MIDI case exercises 29 messages (9 On, 3 Off,
  17 CC), 2,349 boundaries. Original handlers remain responsible for their
  sonic effect; decoder coverage is not proof those handlers are ported.
- Audio hashes: Single `58781405289897816`, four-part
  `13074848859789201745`, MIDI `3819855541230884524`. Original/native recordings
  and all three unrecorded pairs agree; no substitute/dropped native frames.
- CPU: **whole-process GetProcessTimes user+kernel**, separate steady-clock
  wall duration. Excludes original boot/setup/warm-up, trace/report IO and
  recording runs. Sequential DSPs isolate the live controller; three pairs
  each, interleaved H8/live, live/H8, H8/live. No concurrent compilation, test
  suite or trace analysis during valid captures. All 24 fixture processes
  succeeded (six recorded controls and eighteen unrecorded renders).

| Scenario | H8 CPU s, three runs | Live CPU s, three runs | Median CPU H8 / live | Median wall H8 / live s |
| --- | --- | --- | --- | --- |
| Single | 1.000000, 1.000000, 1.000000 | 1.000000, 1.000000, 1.000000 | **1.000000 / 1.000000** | 0.999491 / 1.001034 |
| Four parts | 0.984375, 0.984375, 1.062500 | 1.000000, 0.984375, 0.984375 | **0.984375 / 0.984375** | 0.997120 / 0.992286 |
| MIDI/CC | 1.015625, 1.015625, 1.000000 | 1.031250, 1.015625, 1.000000 | **1.015625 / 1.015625** | 1.024410 / 1.011463 |

- Interpretation: **no measurable CPU saving**, identical medians at the
  observed 15.625 ms process-accounting granularity. Short intervals/repeat
  count do not establish exact zero overhead. Wall differences are small and
  not attributed to a controller speedup. H8's heavier voice/DSP-control work
  remains; prior 23–25% recorded-playback saving is a different experiment.
  No DAW stutter, actual core placement/frequency or listening test result.
- Completion validation: Release Standalone/VST3 and all requested test targets
  built. Refreshed Board/Device consumers before rebuilding because localized
  NMake include scanning is unreliable (same workaround as the previous round).
  Final isolated differential pass, including alternating native/original
  execution inside messages: **12,626 bytes / 6,288 messages / 315,950
  boundaries**, every bus access/state/clock equal. Default H8 audio regression
  retains hash `3852514813786200426` for 65,536 frames and passes saved-state,
  serial and H8/DSP-clock checks.
- Additional complete-Board validation:
  **`output/perf-live-controller/20261015-153603/`**, two DSP workers, native
  disabled at 0.65 s and re-enabled at 0.95 s, all three scenarios, one pair
  each plus recorded controls. All twelve processes succeeded; complete bus
  tapes and audio hashes still match uninterrupted original H8. Native
  execution resumes for the remaining live notes/CC. These are separate
  compatibility/switching checks, not additional primary CPU repeats. Final
  fixture (adds switching case/report wording; unchanged decoder) SHA-256
  `06e73bd6ed30399605b9c401bd1812853bafc9c4218b4edfdc641c90db311756`;
  archived there with source hashes.
- Full-Processor checks: eight CTests passed with
  `SILVER_KS_CONTROLLER=native-midi`, `SILVER_KS_DSP_THREADS=2`, fresh data and
  private K/KS OS (55.37 s). Controller trace, DSP workers, placement,
  diagnostics, wake-up, host clock, native editor and latency/reentry passed;
  the firmware-dependent latency test was executed, not skipped. Separate
  default-H8 latency/reentry also passed (53.15 s), with a fresh data directory.
  Python syntax, whitespace and source-only tracked-file audit passed; raw
  captures/private assembly/binaries remain ignored. Shared H8/JIT source and
  DSP pin have no changes. macOS, Ryzen 3 3100, real DAW/multiple instances and
  arbitrary patches/waves remain untested. macOS CPU accounting still unavailable.
- Next: port actual note/voice and selected CC actions behind this ABI bridge,
  including range/velocity/sustain/all-notes-off state, then voice parameter
  generation. Continue exact command/audio comparison with H8 before replacing
  its periodic DSP reads or removing H8 from the live path. Keep this hybrid off
  by default; no CPU-efficiency claim from functional coverage alone.

## 2026-10-15 — live C++ Note On/Off and sustain/key-state handlers

- Request: move actual note/pedal handling beyond MIDI decoding, compare DSP
  commands, audio and CPU with original H8, retain opt-in/no JIT changes,
  update diary and push verified changes to GitHub **Optimized**. Starting
  checkout clean at `865b2f8e85b3`, tracking origin/Optimized at the same commit.
  Shared H8 CPU source, DSP source/JIT settings/dependency pin, clocks, slice
  targets, serial ordering, buffer sizes, resampling, priorities and Windows
  policy remain unchanged. Only in-plugin native controller dispatch changes.
- Implemented `ksLiveNoteController`: C++ note handlers for range checks,
  velocity-zero release, note/velocity list, per-key velocity, physical-down
  and sustain-held bitmaps, ordered-key removal, mono replacement-key search,
  pedal threshold (below 64 / 64..127), repeated pedal state, held-voice
  release and part all-notes-off/reset decisions. Four voice modes retain
  their original control flow. Remaining original voice code still performs
  DSP allocation, parameter calculations and upload; boot/UART/part routing,
  arpeggiator timing and other CC conversion remain H8. No claim of complete
  native controller/H8 removal or recovered DSP-parameter implementation.
- Conservative implementation: 462 fixed C++ state transitions preserve the
  original interruptible guest ABI and fetch/data bus clocks. They execute
  without decoding/executing the corresponding H8 instructions. Live control
  state remains in guest memory, allowing original/native alternation inside
  calls/messages. Remaining H8 voice-engine calls retain exact targets and
  timing; the DSP comparison does not stub or shortcut that engine. Extended
  firmware fingerprint rejects unknown note/pedal/helper revisions.
- Default original loop retains no per-instruction native dispatch. Opt-in
  uses a cheap page filter before exact controller-PC dispatch. Initial
  private preflight (`20261015-155105`, one pair, 2 s pedal case) matched all
  bus/audio checks but CPU was ~7.9% higher; excluded from final estimate.
  Removed a redundant 128-vector interrupt scan when no IRQ is pending and
  replaced the hot multi-range filter with a page bitmap. Neither changes
  interrupt arbitration or guest semantics. Actual IRQ delivery still runs
  through the unchanged H8 core.
- Isolated differential verification: original H8 is the independent oracle
  for 12,626 live bytes / 6,288 decoded messages / 315,950 decoder boundaries,
  plus **14,784 note/pedal boundaries** (all 462 transitions, 32 varied
  operand/flag sets each). Registers/flags/PC/clock, stack/call and bus-pair
  bookkeeping, every bus read/write and final RAM match. Calls stop at their
  actual ABI destination in the one-boundary test. Pending-interrupt,
  firmware/status guards and alternating native/original execution checked.
  First harness run had an unsynchronized starting CPU clock after its guard
  test; corrected test setup before collecting this passing result.
- Primary native comparison: **`output/perf-live-controller/20261015-155316/`**,
  Windows x64 i5-13600KF (14 physical / 20 logical), MSVC 1944 Release/NMake,
  source `865b2f8e85b3` dirty, DSP pin unchanged
  `44353be2f0156cac970c6ab4f467ea1998b40605`. Archived fixture SHA-256
  `2d6bf78483cf27ddaaeaf082f111c502a26b99d9a8aed88ddf4f7218b5355bf7`;
  source hashes retained. Private KS OS 2.1.02, synthetic Init Flash and zero
  wave bank. Raw tapes/binaries/private disassembly stay ignored and are not
  part of the GitHub upload.
- Work: **181,362 native frames** (~4.0000231 s), sequential DSPs for both
  modes, all 24 integer streams. Same live MIDI to original H8 and native;
  each fresh process boots/prepares/warms through original H8 before selecting
  the measured path. `single` and `multi` extend the earlier chord passage to
  four seconds; `midi` extends the earlier control passage with its release
  tail. New `pedal` has four parts: Mono / Mono autoglide / Poly / Poly
  retrigger, key range 36..84 with notes outside it, repeated notes, Note On
  velocity zero, release velocity, CC64 values 63/64/127/0 and repeated values,
  all-notes-off/all-sound-off interactions and two event cycles. No sampled
  wave-bank/drum result inferred from the zero-wave fixture.
- Exact comparison: original/native recorded tapes are **byte-identical**,
  including host reads, writes, acceptance, guest cycle/PC and slice clocks.
  All unrecorded pairs match audio hash/peak/frames, H8/DSP cycles/instructions
  and serial depth/underflow/overflow. All 32 cases succeed (eight recorded
  controls plus three interleaved H8/native pairs per scenario). No concurrent
  build, other test suite or trace analysis during the measured renders.

| Scenario | DSP bus events compared | All-24-stream audio hash | Native note-state boundaries |
| --- | --- | --- | --- |
| Single | 1,450,157 | `2387145891285364963` | 698,075 |
| Four parts | 1,061,786 | `3176463666096680485` | 1,300,613 |
| MIDI/CC | 1,125,213 | `1205626428641651500` | 785,283 |
| Pedal / four voice modes | 703,901 | `3541542743908256581` | 1,548,572 |

- Pedal corpus executes 120 Note On handler entries, 64 explicit Note Off
  entries, 80 sustain actions and 24 pedal-up voice-release walks in native
  code. Velocity-zero release branches inside Note On, so decoded Note Off
  counts and explicit Note Off entry counts intentionally differ. Coverage
  assertions require note, note-off and sustain handlers to run, not merely
  that the option was requested. Audio equality is verified by hashes; no
  listening or real DAW dropout test performed.
- CPU: whole-process Windows GetProcessTimes user+kernel; wall via steady_clock.
  Original boot/setup/warm-up, recording, file IO and formatting excluded;
  common sample hashing and native execution counters included. Order per
  scenario H8/native, native/H8, H8/native; three runs per mode. Process CPU
  accounting is quantized (15.625 ms observed). Same equal-work render, no
  host pacing/resampling/underrun substitute. New four-second figures are not
  pooled with the earlier two-second decoder/replay measurements.

| Scenario | H8 CPU seconds per run | C++ CPU seconds per run | Median H8 / C++ | CPU change | Median wall H8 / C++ s |
| --- | --- | --- | --- | --- | --- |
| Single | 1.984375, 2.000000, 1.968750 | 2.078125, 2.015625, 2.000000 | **1.984375 / 2.015625** | **+1.57%** | 1.984394 / 2.026747 |
| Four parts | 1.937500, 1.953125, 1.968750 | 2.015625, 1.953125, 1.968750 | **1.953125 / 1.968750** | **+0.80%** | 1.986578 / 1.983301 |
| MIDI/CC | 1.968750, 1.937500, 1.984375 | 1.984375, 2.015625, 1.984375 | **1.968750 / 1.984375** | **+0.79%** | 1.985780 / 1.992141 |
| Pedal | 1.937500, 1.937500, 1.953125 | 1.968750, 1.968750, 1.968750 | **1.937500 / 1.968750** | **+1.61%** | 1.955155 / 1.975182 |

- Result: functional note/sustain port verified, **no CPU saving**. Prototype
  medians are slightly higher by one/two CPU-accounting quanta; three short
  repeats do not establish an exact overhead or cause. Conservative boundary
  dispatch/counters, cache/placement/frequency and remaining H8 work are not
  separately profiled. No ETW in this round. Keep native mode off by default;
  no claim that the Ryzen 3 3100 stutter is fixed or that a live H8-free
  controller would match these costs. Previous replay CPU saving is unrelated.
- Final validation: Release Standalone and VST3 built successfully. Refreshed
  header consumers before the build because localized NMake dependency scanning
  is unreliable; no shared H8/DSP source was edited. Final private differential
  test passed, including new handler fingerprint and all native page-filter
  entries. Default H8 regression retains 24-stream hash `3852514813786200426`
  / peak 1,356,564 for 65,536 native frames, and passes saved-state, serial and
  H8/DSP-clock checks.
- Switching/helper validation: **`output/perf-live-controller/20261015-160226/`**,
  Single and four-part pedal scenarios, two DSP workers, native disabled at
  0.65 s and re-enabled at 0.95 s. All eight processes (recorded controls plus
  one pair per scenario, two seconds each) passed full-tape/audio comparisons
  with uninterrupted H8. Actual native note/pedal handlers ran after re-enable.
  These compatibility/switching runs are separate from the primary CPU sample.
  Rebuilt fixture archived with source hashes; SHA-256
  `e04ef645a7c73331750b156bdeb75c7dffb1e7234aeb83bd65ffc2619da2280f`.
- Full Processor checks: eight CTests passed with native mode and two DSP
  workers (53.91 s): trace, DSP workers, placement, diagnostics, wake-up, host
  clock, editor and latency/reentry. Private K/KS OS and fresh data directory
  supplied; firmware checks executed, not skipped. Separate default-H8
  latency/reentry also passed (51.87 s). Python syntax, staged whitespace and
  source-only repository audit passed (574 tracked files). Private traces,
  disassembly, firmware, executables and personal reports are all ignored and
  excluded from the source commit. Normal CI cannot execute private-ROM checks.
- Publication requested to existing `origin/Optimized` (source/tests/docs only);
  final push receipt/commit identity is reported in the chat. Windows evidence
  does not validate macOS; Mac compile/audio, real VST3 host/multiple instances,
  other patches/waves and Ryzen remain open.
- Next: port/measure the remaining DSP voice parameter engine or reduce ABI
  transition cost while preserving exact clocks/interrupts. Note state now has
  an independent C++ implementation and a strict H8 oracle for that work.

## 2026-10-15 — Native KS voice lifecycle and DSP host transport (migration incomplete)

- Request: finish the entire C++ controller, keep it optional, leave JIT/Windows
  policy unchanged, update this shared diary and publish to `Optimized`.
  **The full-controller request is not complete in this change.** This is a
  verified additional migration stage; it still requires H8 during playback.
  The open scope question is whether original H8 boot alone may remain in the
  eventual complete mode. Neither answer changes the unfinished live work below.
- Source: parent `ab97a51d9fa5792a39d4c560b41c96844683dc1c`, branch `Optimized`,
  this entry/source patch. Windows x64, Core i5-13600KF, 32 GB; MSVC 19.44,
  Release/NMake. DSP dependency/configuration and JIT unchanged from the prior
  entry (`44353be2f0156cac970c6ab4f467ea1998b40605`). No Windows settings,
  system timer policy, process/thread priority or affinity edits, UAC collection,
  installs, deletions or changes to the user's ROM/Flash files.
- Added `ksLiveVoiceController`: 640 fixed C++ state transitions for DSP host
  address/status/24-bit read/write on A/B, selected voice/part address windows,
  voice allocation/reuse/stealing for mono/auto/poly/retrigger, velocity curves,
  voice slot initialization, note-start/glide/retrigger sequence, and matching
  note/pedal release. Original timing/polling and timeout calls retained. This
  is an interruptible guest-ABI migration, not a new opcode decoder or JIT.
  The bulk synthesis parameter calculators and periodic voice service still
  execute H8 callees. Threshold tables are read from the private input ROM;
  no ROM image, table dump or raw recording is included in the repository.
- Original H8 stays default. Existing `SILVER_KS_CONTROLLER=native-midi` opts in;
  unset/recreate to disable. The worker-only toggle remains available and the
  `live-cpp-midi-hybrid` backend name is preserved. No K engine behavior changes.
  A separate fingerprint verifies every newly ported range before activation.
- Shared boundary helper retains registers/flags, call stack, fetch/data clocks,
  pairing and in-flight PC visible to mapped devices. The latter matters now
  that C++ accesses actual DSP ports: original H8 exposes entry PC + 1 during
  the remainder of each instruction. The isolated oracle now compares PC and
  cycle at **every** bus access, as well as final machine state and RAM.
- Validation actually run: Windows Release `ksLiveMidiController_test`,
  `ksControllerProbe_test`, VST3 and Standalone built successfully. Private
  OS 2.1.02 differential test passed: 12,626 live bytes, 6,288 messages,
  315,950 decoder boundaries, 14,784 note/pedal boundaries and **20,480 new
  voice/transport boundaries** (640 x 32 varied operand/flag sets). Revision
  guard rejects changed voice code; pending-interrupt guards cover host access,
  voice selection/start and allocation. No DSP voice engine is stubbed.
- Internal whole-Board smoke ID: **`output/perf-live-controller/20261015-174241/`**.
  Probe EXE SHA-256:
  `d8fd0ddb22c44fb761d5fa3f11ec89fd6294c757813c50990de6e2c309185117`.
  Private synthetic Flash and zero wave bank; same setup as prior entry;
  single process at a time, sequential DSPs. Four scenarios, 2 s / **90,681
  native frames** each; original/native bus controls plus one unrecorded pair
  each (16 processes). Full tapes are byte-identical, including read responses,
  writes/acceptance, PC, cycle, slice and checkpoint; all 24-stream integer
  audio hashes/peaks, guest/DSP counters and serial state match exactly.

| Scenario | 24-stream hash | DSP port events | Native allocation/start/release calls | Remaining original-core calls |
| --- | --- | ---: | --- | ---: |
| Single | `58781405289897816` | 736,364 | 152 / 152 / 167 | 2,987,534 |
| Multi | `13074848859789201745` | 537,112 | 152 / 152 / 212 | 3,019,849 |
| MIDI | `3819855541230884524` | 579,064 | 9 / 9 / 9 | 3,389,358 |
| Pedal | `17487164177108066277` | 352,175 | 40 / 40 / 35 | 3,312,608 |

- The probe now reports native voice/transport execution and actual calls to
  original H8. Detailed page counts are explicitly enabled only in recording
  fixtures; normal plugin and unrecorded timing runs keep that vector empty.
  Counts are calls to the core, including interrupt arbitration, **not CPU
  time, instructions or percentage of features ported**. Toggle resets the
  current controller/counter segment. These nonzero calls demonstrate that
  this stage is not a standalone replacement.
- Preflight retained privately at `output/perf-live-controller/20261015-174041/`:
  audio, data, cycles, slices and counts matched, but all port records showed
  PC one byte behind the original. This run failed exact-tape comparison and
  is excluded from passing evidence. Fixed the shared native boundary's
  in-flight PC, strengthened the isolated oracle, rebuilt and reran all four
  scenarios successfully; no recorder normalization or comparison relaxation.
- Toggle/helper smoke ID: **`output/perf-live-controller/20261015-174405/`**,
  Pedal scenario, 2 s, two DSP workers, disable at 0.65 s/re-enable at 0.95 s;
  four processes (bus pair + unrecorded pair). Full bus tapes and all audio,
  clocks/serial counters match uninterrupted original H8. Native voice code
  executed after re-enable. Counter/page totals describe only that final
  enabled segment; this check does not measure a CPU benefit.
- Performance is **pending**: the short, single-pair smoke runner records CPU
  seconds but is only correctness evidence here. No CPU saving, DAW stability,
  real-time dropout fix or Ryzen 3 3100 improvement is claimed. User-facing
  listening/host tests and repeated performance measurements remain the next
  validation phase. No macOS build, audio or performance evidence this round.
- Remaining implementation before full migration: oscillator/filter/envelope/
  LFO/effects calculators; periodic voice maintenance and DSP feedback; all
  channel/part and remaining CC/NRPN/bend/pressure handling; clock/arpeggiator;
  preset/performance/SysEx/state/Flash loaders; main-loop, interrupt/UART
  servicing, plus native boot if required. These need original-H8 command,
  state and audio oracles; stopping H8 now would break supported behavior.
- Publication: source/tests/docs only to the existing `origin/Optimized`.
  Final commit/push identity and verification are reported in the chat.

## 2026-10-15 — Complete native KS playback, original H8 bootstrap retained

- Request clarified: retain H8 for DSP loading/initial setup, move all normal
  playback control to C++, keep it optional, compare original commands/audio/
  CPU, publish source and this diary to `Optimized`. This supersedes the
  previous entry's open boot question and incomplete playback migration.
- Source: parent **`6d9369456c92fb333cc40cac4309e384658c7ed4`**, branch
  `Optimized`, this patch. Windows x64, Core i5-13600KF, 32 GB; MSVC 19.44,
  Release/NMake. DSP pin **`44353be2f0156cac970c6ab4f467ea1998b40605`**, JIT
  and shared H8 core unchanged. No Windows settings, system timer policy,
  priorities, affinity, buffers, resampling or serial/slice targets changed.
  No system trace collection, installs or deletions in this round; private
  input ROM/Flash/wave files are read only. Test state lives in ignored output.
- New `SILVER_KS_CONTROLLER=native` / `native-cpp-runtime` takes over immediately
  after `Board::boot()` returns, before Device program restore/voice warm-up.
  Original H8 remains the default; unset and recreate the instance to disable.
  The earlier `native-midi` hybrid and recorded-fixture prototype stay separate.
  K remains unchanged. The worker-owned handoff API also supports explicit
  disable/re-enable at quiescent boundaries, retaining current register/RAM
  state so original H8 can resume without resetting the sound.
- Implemented fixed C++ flow for **13,766 playback boundaries** (12,664 new,
  reusing 1,102 existing note/voice boundaries): UART/interrupt/main-loop work;
  all channel/system/realtime MIDI, CC/NRPN/bend/pressure and SysEx routing;
  notes/sustain/voice modes/stealing; oscillator/filter/envelope/LFO/FX parameter
  calculations and DSP feedback; clock/arpeggiator; program/performance editing,
  state and user Flash handling. This consumes live input and DSP replies;
  it does not replay captured commands or call H8 for remaining callees.
- This is a **native compatibility port**, retaining the controller's register/
  RAM ABI and original clock grid. It removes runtime opcode decoding, not the
  original polling/state-machine frequency. It is not an event-batched
  OsTIrus-style controller and does not inherit the earlier tape-replay saving.
  C++ fetch boundaries account for clocks without reading immutable opcodes;
  ordinary RAM/stack bypasses device-map dispatch while actual MMIO/DSP/Flash/
  wave access remains mapped. Native TPU0 uses an O(1) counter advance with
  exact reference wrap/compare/IRQ behavior; original H8 retains its old loop.
- Guards: fingerprint the entire verified OS 2.1.02 and compare active SRAM
  executable ranges at handoff. Reject executable writes/unknown native states
  explicitly; lock the original interpreter after handoff, with **no implicit
  fallback**. The H8 object retains timing/frame storage, not interpreted work.
  Firmware-update/bootloader execution is outside the native playback range;
  loading firmware through the plugin recreates the engine and uses original
  boot. Private firmware/tables are still required externally, never embedded.
- Production VST3 and Standalone, probe and affected Processor test consumers
  rebuilt successfully in Release, including consumers of the changed Board/
  Device headers. Frozen final probe SHA-256:
  **`d2ab50c74031ae22f8084510967ee637f499836c89ebab8bac398f68ece47e13`**.
  Local evidence root: **`output/perf-native-controller/20261015-185859-final/`**.
  `source.json` records per-file source hashes/configuration; comparison metadata
  records executable identities (and the final runner hash for frozen controls).
- Differential validation passed against original H8: **440,512 boundaries**
  (13,766 x 32 full-width operand/flag sets) plus **80** interrupt/mask/tie
  entries. Registers, CCR/EXR, PC, clocks, stack bookkeeping, pending interrupts,
  data/IO access values/order/cycles/in-flight PC and final RAM match. Alternate
  samples use mapped instruction fetch and production clock-only fetch; an
  independent reference disassembler identifies only the fetch callbacks that
  intentionally disappear. Actual data/IO accesses are never filtered away.
  ROM/image changes, unknown PCs and interpreter-lock guards were exercised.
  Additional **150,000** RAM/ROM-overlay operations match mapped byte order,
  clocks and IO side effects; **101,232** timer cases match the original loop,
  including wrap, compare zero, clearing and coalesced repeated matches.
  Existing hybrid decoder, note and voice differential tests also passed.
- Preflight caught an incorrect indirect bit-store translation; fixed its
  register-indirect mask and reran the complete boundary oracle successfully.
  The first whole-Board smoke already matched commands/audio but used 3.1%
  more CPU in one Single pair; this was not a performance result. Timer/fetch/
  RAM paths above were then optimized only inside native mode and revalidated.
- Final correctness ID: **`correctness/20261015-185859`**, 8 scenarios x 2 s /
  **90,681 native frames**; bus pair plus one unrecorded pair each, 32 processes.
  Full binary bus tapes are byte-identical, including all A/B read responses,
  writes/acceptance, PC, clocks, slices and checkpoints. All 24 integer audio
  hashes/peaks, guest/DSP clocks/instruction counters and serial state match.
  Final state/Flash/MIDI-output digests and received/transmitted MIDI counts
  match as well. `state` completes one real in-memory Flash page write; input
  files are not modified. All native preparation/playback reports show actual
  post-boot interpreter calls **0**, and positive native execution.

| Passage | 24-stream audio hash | DSP port events | H8 calls after boot |
| --- | --- | ---: | ---: |
| Single | `58781405289897816` | 736,364 | 0 |
| Multi | `13074848859789201745` | 537,112 | 0 |
| MIDI | `3819855541230884524` | 579,064 | 0 |
| Sustain / voice modes | `17487164177108066277` | 352,175 | 0 |
| All CC / NRPN / bend / pressure | `1977305875963167696` | 388,261 | 0 |
| Program / performance / state / Flash | `2430842298004455044` | 362,922 | 0 |
| Clock / arpeggiator / transport | `6672608335291489240` | 308,108 | 0 |
| Voice stealing / unison | `6863044123991515266` | 654,626 | 0 |

- CPU baseline audit: a new per-step interpreter guard/counter could penalize
  the current H8 control. Therefore headline numbers use an **archived original
  EXE without that new instrumentation**, same compiler/Release configuration,
  DSP pin/JIT, inputs and exact passage. Frozen H8 SHA-256:
  **`2d6bf78483cf27ddaaeaf082f111c502a26b99d9a8aed88ddf4f7218b5355bf7`**
  (`output/perf-live-controller/20261015-155316/ksControllerProbe_test.exe`).
  Runner now accepts `--reference-executable` and records both binary hashes.
- Headline performance ID: **`frozen-reference/20261015-190920`**, sequential
  DSPs, four scenarios x 4 s / **181,362 native frames**, three interleaved
  unrecorded pairs per scenario (H8/native, native/H8, H8/native), plus separate
  trace controls, 32 processes. One fixture process at a time, no concurrent
  builds/tests. CPU is whole-process user+kernel seconds for equal work;
  bootstrap/setup, trace/report file IO are excluded. No DAW CPU-meter estimate.
  Full command tapes, audio and guest/serial counters also match the frozen
  original. That older fixture lacks state/Flash/MIDI-output snapshots; those
  are explicitly marked unavailable and verified with the current H8 above.

| Passage | Original H8 CPU s, median | C++ CPU s, median | CPU reduction | Wall s H8 -> C++ |
| --- | ---: | ---: | ---: | --- |
| Single | 2.062500 | 1.953125 | **5.30%** | 2.0982 -> 1.9907 |
| Multi | 1.968750 | 1.921875 | **2.38%** | 1.9840 -> 1.9124 |
| MIDI | 2.000000 | 1.921875 | **3.91%** | 2.0076 -> 1.9335 |
| Sustain | 1.937500 | 1.875000 | **3.23%** | 1.9710 -> 1.8822 |

- Raw CPU seconds in order, original / native: Single
  `[2.046875, 2.062500, 2.062500]` / `[1.937500, 1.953125, 2.000000]`;
  Multi `[1.968750, 2.015625, 1.968750]` / `[1.937500, 1.921875, 1.890625]`;
  MIDI `[1.953125, 2.000000, 2.000000]` / `[1.875000, 1.921875, 1.937500]`;
  Sustain `[1.968750, 1.937500, 1.937500]` / `[1.875000, 1.859375, 1.906250]`.
  These are modest local savings; three short pairs and quantized process
  timing do not establish a precise universal percentage or a dropout fix.
- Separate same-current-EXE control ID: **`primary/20261015-190218`**, same
  4 s/three-pair protocol. CPU medians H8 -> native: Single 2.062500 -> 1.921875
  (-6.82%), Multi 2.015625 -> 1.921875 (-4.65%), MIDI 2.031250 -> 1.937500
  (-4.62%), Sustain 2.000000 -> 1.875000 (-6.25%). Commands/audio/state/Flash/
  MIDI snapshots all match. Do not prefer these larger numbers over the frozen
  baseline; different runs also contain normal timing variance. The short
  2 s single-pair correctness series is not CPU evidence (it includes noisy
  reversals). Earlier **23–25% tape-replay** results are not live-controller
  performance and are not carried forward.
- Two-worker/toggle ID: **`toggle/20261015-191155`**, final native probe, Pedal,
  2 s, disable at 0.65 s /
  re-enable at 0.95 s, separate bus and unrecorded pair. Complete commands,
  audio, clocks/serial/state/Flash/MIDI snapshots match original H8. The disabled
  window executes 797,572 original-core calls as intended; calls since final
  native handoff remain **0**. This is compatibility evidence, not a CPU-saving
  claim for parallel DSPs.
- Production Processor integration: `silverLatency_test` passed with native
  control and two DSP workers, private K/KS firmware and a fresh isolated data
  directory. Both models passed synchronous host reentry, 44.1/48 kHz and
  256/512/1024 blocks, buffer profiles, state restore, asynchronous firmware
  completion, nonsilent offline rendering and return to realtime; 9 latency
  notifications each. `silverClock_test` passed host tempo/transport/sample
  offset checks. `silverEditor_test` passed geometry/navigation with actual
  K/KS firmware and the same experimental flags in its own isolated directory.
  Logs are `latency.log`, `clock-test.log`, `editor-test.log` in the evidence
  root; no user's existing data directory was used for these writes.
- Publication: source/tests/docs only to existing `origin/Optimized`; raw tapes,
  private ROM/banks, disassembly/generators, reports, audio and executables stay
  ignored. Final commit/push identity is reported in the chat. Public CI skips
  private-ROM checks and cannot reproduce those validations without inputs.
  Staged whitespace/UTF-8/Python syntax checks passed; repository audit passed
  for **590 tracked files**, with no private data or build artifacts. All C++
  source hashes still match the measured binary snapshot; only the comparison
  runner subsequently gained the frozen-reference option documented above.
- Limitations: private synthetic Flash and zero wave bank, one OS revision and
  local Intel only. No macOS build/audio/CPU validation, real VST3-host listening,
  multi-instance deadline test, full real-wave/patch corpus or Ryzen 3 3100
  result. Exact offline audio equality is not a real-time dropout measurement.
  No conclusion that the tester's stutter is fixed. Next: real host A/B tests,
  then profile native control vs DSP cost before considering any further
  controller batching; keep original H8 as the oracle.

## Entry template

Copy for each next round:

- Date, platform and CPU:
- Source revision/configuration/compiler and diagnostics mode:
- Test ID and local raw report reference:
- Host/version/backend/device, rate, actual/prepared block and reserve:
- Model, patch, parts/voices, instances, UI state, power mode and passage:
- Hypothesis and exact change (or measurement only):
- Validation actually run and result:
- Metrics (units), audible outcome and interpretation:
- Limitations / untested platform / instrumentation overhead:
- Next step:
