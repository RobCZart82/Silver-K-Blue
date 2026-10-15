# Performance measurements (Windows and macOS)

Diagnostics are opt-in per plugin instance. They do not change guest clocks, JIT
configuration, audio buffers, MIDI scheduling, resampling or thread priorities.
They add instrumentation overhead; compare Off, Summary and Detailed on the same
machine before attributing a marginal dropout to the engine. No audio, firmware,
full file paths or machine/user name is collected. Reports contain CPU/OS/build,
host type, firmware filename and patch label: review these before sharing.

## Capture in a DAW or Standalone

1. Use an optimized build and load K-Station or KS normally. Set the patch, parts,
   buffer profile, DAW buffer and sample rate **before** starting a measurement.
2. Open **Setup > Performance diagnostics > Start new summary measurement**.
3. Play a repeatable MIDI passage for 30–60 seconds. Stop the measurement after
   the passage, including the part where stutters occur.
4. Use **Save performance report...** to write JSON. Saving is on the message
   thread, never on the audio callback or emulation worker. Prefer exporting
   after Stop: the final in-flight operation may finish shortly after Stop.
5. Repeat with **Start new detailed measurement**. Every Start starts a fresh
   session. Deep timers sample every 256th board advance; summary has no deep
   clock reads. Reopening the editor does not stop a recording.

Firmware reload or an engine/audio reconfiguration stops the session; its report
remains available with `stop_reason=engine_rebuilt_or_stopped`. Use a new session
after the change. Parameter/preset/profile/part changes do not create new sessions:
hold them fixed for comparisons, or document the changes as a separate scenario.
Diagnostics mode is not saved in DAW state or preferences, and is off in a new
instance. Recording begins after firmware warm-up, so boot time is excluded.

Record alongside **each** report: test ID, DAW and version or Standalone, audio
driver/backend (ASIO/WASAPI/CoreAudio), device, actual buffer and sample rate,
patch/bank identity, MIDI passage/duration, part count, number of instances,
UI open/closed, power mode, audible outcome and other running CPU-heavy tasks.
Host detection is best-effort and does not determine a DAW version or driver.
Use the same passage and settings on Windows/macOS, but compare within each
machine first; absolute times from different CPUs are not equivalent evidence.

## Reading `silver-k.performance.v1`

Durations are **wall time in nanoseconds**, including OS preemption. They are not
thread CPU usage. Each metric includes observations, total, mean, minimum,
maximum, p99 upper bound, work frames and over-budget observations. Percentiles
use fixed logarithmic bins; the upper bound can be nearly twice the real value.
Use maximum and over-budget counts as well. Zero observations means **not
measured**, not free. Live atomic reads are approximate, not a single instant.

| Metric | Meaning |
| --- | --- |
| `audio_callback` | Complete `processBlock`, deadline = actual block frames / host rate. Includes MIDI/control and rendering. |
| `audio_controls` | MIDI/control/snapshot work before rendering; excludes audio rendering. K host-clock preparation is outside this sub-timer. |
| `audio_render` | Host-side queue consumption; K also includes input/output resampling. Offline waits are included. |
| `worker_segment` | Contiguous worker work between waits, split after at most 256 output frames. K units are native frames; KS units are host frames. |
| `worker_sleep` | Total idle wait: K on Windows waits for progress with WaitOnAddress (10 ms safety timeout); KS, other platforms and the polling override retain the 50 µs sleep. This is not wake-up latency. |
| `worker_wake_latency` | K on Windows only: time from the first notification of an armed wait to the worker returning, including scheduler delay. Available with both address waiting and the polling override. No observation for an unnotified timeout or skipped wait. KS retains its original sleep and records no observations here. |
| `worker_snapshot` | Publishing control/flash snapshots; may overlap inclusive worker time. |
| `h8_sampled` | Sampled controller execution including its peripheral tick work. |
| `dsp_a_sampled`, `dsp_b_sampled` | Sampled DSP execution including peripheral work and any inline JIT compilation. B is absent for K. |
| `serial_sampled` | KS shared serial-clock servicing, measured separately from DSP execution. Absent for K. |
| `dsp_barrier_sampled` | Experimental two-worker KS only, every 256th advance in Detailed: coordinator dispatch plus waiting for both DSP slices. Includes DSP execution and scheduling; overlaps DSP timers and must not be summed with them. Zero observations in sequential mode. |
| `queue_depth` | Ready output frames at entry to device rendering, not minimum depth over all samples. K can render multiple chunks per callback. |
| `missing_frames` | Frames substituted by silence during this session; total is the count, observations are device calls. |
| `stale_frames` | Already-late output frames discarded by the consumer. |
| `host_block_frames` | Actual callback block sizes, useful when prepared maximum differs from live sizes. |
| `offline_callback` | Total counts callbacks marked non-realtime by the host. Interpret their deadlines separately. |

`worker_wall_to_audio_ratio` = measured worker segment wall time / duration of the
frames it generated. Above 1 suggests insufficient throughput in those segments;
it can also reflect preemption. Below 1 with missing frames can suggest scheduling,
bursts, queue reserve or a rare slow path. This ratio is **not total CPU %** and
excludes sleeps, some loop bookkeeping, and an unfinished segment at Stop.
K snapshot work preceding segment start can be outside the ratio. Do not sum the
inclusive worker/callback metrics with their child timers.

Deep sampling is deterministic and may miss a rare cold-JIT spike or repeatedly
sample a particular firmware phase. Repeat runs; do not extrapolate a precise CPU
breakdown from one capture. JIT compilation is not separately instrumented in this
phase; a slow DSP sample alone does not prove that the JIT compiler was responsible.

## Opt-in KS dual-DSP worker experiment

Set `SILVER_KS_DSP_THREADS=2` in the launching process environment **before**
opening Standalone or the DAW. It is read once for each KS device. Unset it or
set it to `1`, then reload the instance to restore sequential execution. The
setting is not saved in presets/preferences/DAW state and has no effect on K.
This is an experiment, not an enabled-by-default optimization.

The original H8, DSP boot and warm-up stay sequential. Afterwards two persistent
helpers execute only each DSP's catch-up at the existing H8 slice boundary.
The coordinator waits for both, then services shared serial IO in the original
order. Guest targets, JIT, buffers, polling and existing worker priorities stay
unchanged; helpers use the same live priority as the existing worker. No Windows
setting or process placement is changed. The previous P-core option targets
the coordinator only; do not interpret it as verified placement of DSP helpers.

JSON `dsp_execution_backend` in the root/session identifies `sequential`,
`two-workers-slice-barrier`, or `not-applicable` for K. In parallel mode the
worker wall/audio ratio measures coordinator wall time including its DSP waits,
not summed CPU usage across three threads. Detailed DSP timers run on their
respective helpers and overlap. Compare process CPU separately and distinguish
steady playback from the unchanged sequential boot/warm-up cost.

`tools/run_performance_matrix.py --dsp-threads 2` verifies the reported backend.
`tools/run_dsp_thread_comparison.py` interleaves frozen original, same-binary
sequential and two-worker cases with ordinary placement, clearing inherited
wait/placement/thread overrides. It records executable SHA-256, diagnostics,
fixture results and, on Windows, whole-process CPU seconds **including boot**.
It runs the existing single-part 12 s synthetic fixture, not a real ASIO/VST3
host or a reproduction on the tester's Ryzen. Raw evidence stays ignored.

Build/run `ksDspWorkers_test` for ROM-free handoff/failure/shutdown checks.
`ksDspEquivalence_test <KS update/decoded OS> <wave> <Flash>` compares exact
24-stream audio, H8/DSP timing, serial counters and state in Single and four-part
passages; this private-input test returns 77 without arguments. Do not commit
firmware, wave banks or captured audio. Windows and macOS require separate
build/runtime verification; portable synchronization code alone is not validation.

## Equal-work CPU cost (Windows measurement tools)

Build `ksCpuCost_test` explicitly. Run `tools/run_cpu_cost_comparison.py`
with `--modes native --executable <ksCpuCost_test.exe> --ks-rom <KS update/decoded
OS> --flash <Flash> --wave <wave bank>`. The native target expects an unzipped
supported update or decoded OS. Default three interleaved runs per mode compare
the same binary with `SILVER_KS_DSP_THREADS=1` and `2`, clearing placement/wakeup
overrides. This does not change production code or Windows policy.

The fixture renders 544,085 native frames (about 12 s) through the original H8
and both DSPs, hashes all 24 integer streams, and counts **all process threads'
user + kernel CPU seconds** via GetProcessTimes. Boot/setup, warm-up, helper
startup, formatting and file IO are outside the measured interval. Wall duration
is separate. There is no host queue, resampler, missing-frame fallback, pacing
or diagnostic session in this native cost test. It measures active emulator
cost; it is not a DAW CPU meter or power measurement. Check final guest cycles/
instructions as well as audio hashes, and keep CPU and wall-time changes distinct.

`summary.json` sets `valid_equal_work` only if all runs complete with the expected
frame count, zero missing frames and identical audio hashes. Never claim a saving
from invalid comparisons. Keep raw reports and supplied firmware local/ignored.

`silverCpuCost_test` and the runner's `offline`/`paced` modes are exploratory
full-Processor measurements. They also record caller CPU (callback, hashing,
offline queue waits and pacing); process-minus-caller includes all background
threads and is not a precise emulator-only timer. The current host runs differ
in audio hashes even between sequential repeats, so they do **not** validate a
strict CPU saving. See the dated journal before reusing them. These CPU-accounting
fixtures currently run on Windows only; macOS returns 77 and needs separate work.

## Command-line fixture

Build `silverRealtime_test` explicitly (it is excluded from the default build).
It runs the existing 12-second synthetic passage at 48 kHz, with a first-note
phase, eight-note chords, optional state saves, and a prepare call at second 6.
This is a regression fixture, not a substitute for a real DAW/driver test and not
a four-part KS benchmark. It requires private original firmware arguments; never
commit them. The JSON path must be absolute and its parent must exist.

Windows PowerShell, with paths replaced by your local private files:

```powershell
$env:SILVER_K_DATA = "$PWD/output/perf-fixture-data"
$env:SILVER_REALTIME_K = '1' # 1=K, 0=KS
$env:SILVER_REALTIME_BLOCK = '128'
$env:SILVER_REALTIME_PROFILE = '1' # 0 Low, 1 Balanced, 2 Safe, 3 Maximum
$env:SILVER_REALTIME_SAVE = '0'
$env:SILVER_PERF_MODE = '1' # 1 summary, 2 detailed
New-Item -ItemType Directory -Force output/perf | Out-Null
$env:SILVER_PERF_REPORT = "$PWD/output/perf/win-k-128-summary.json"
& 'build/source/novation/silverK/buildStandalone/silverPlugin/Release/silverRealtime_test.exe' 'C:/private/K.os' 'C:/private/KS.syx' 'C:/private/ks-init-flash.bin' 'C:/private/ks-wave-zero.bin'
```

For a single-configuration NMake build, omit `/Release` from the executable path.
On macOS the same environment variables and arguments apply:

```sh
mkdir -p output/perf
SILVER_K_DATA="$PWD/output/perf-fixture-data" \
SILVER_REALTIME_K=1 SILVER_REALTIME_BLOCK=128 SILVER_REALTIME_PROFILE=1 \
SILVER_REALTIME_SAVE=0 SILVER_PERF_MODE=1 \
SILVER_PERF_REPORT="$PWD/output/perf/mac-k-128-summary.json" \
build/source/novation/silverK/buildStandalone/silverPlugin/silverRealtime_test \
/private/K.os /private/KS.syx /private/ks-init-flash.bin /private/ks-wave-zero.bin
```

Repeat for K/KS and buffers 128/256/512, summary/detailed and Off control runs
(omit `SILVER_PERF_REPORT` for Off). Compare no state-save and state-save runs.
In a DAW add KS Performance with four parts and UI closed/open. Keep the initial
note phase separate from sustained playback in listening notes; this first report
format aggregates the session rather than emitting a sample-by-sample timeline.

For a sequential matrix on either OS, `tools/run_performance_matrix.py` accepts
`--executable`, `--k-rom`, `--ks-rom`, `--flash`, `--wave` (all file paths),
`--output output/perf`, `--blocks 128 256 512`, `--models K KS`,
`--modes off summary detailed`, `--repeats 2`, `--profile 1` and optional
`--save-state`. It creates a fresh timestamped result directory, isolates fixture
preferences, saves JSON/logs and an `index.json`, and continues across failed
fixtures. Exit 1 means at least one failed run or missing/invalid report; inspect
the reports rather than discarding evidence from a failed realtime run. Off
reports have no active session and zero recorded metrics: use their fixture
summary for baseline callback time and missing-frame counts.

## Windows worker wake-up comparison

Windows 8+ uses dynamically resolved [WaitOnAddress](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress) / [WakeByAddressSingle](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle).
The K worker arms its wait, rechecks progress/stop/pause, then parks. KS retains
the original polling wait (`poll-50us`), without the new arm/recheck/notification
path: event parking failed its 64-frame Low and 128-frame Balanced tests in
extended repeats. Polling also failed intermittently in later trials, so causality
is unresolved. A reserve-duration guard was tested and rejected too. Neither
reserve sizes nor plugin latency are increased.
The callback
notifies after publishing the consumed block; offline consumption and control
pause/stop paths also notify. No callback wait, timer-resolution change, busy
spin or thread-priority change is introduced. A 10 ms timeout is a safety path
when no notifications arrive, not the normal refill cadence. Hosts without the
Windows API and non-Windows platforms retain the old polling behavior.

For matched A/B measurements on the same Windows executable use the runner's
`--worker-wait poll` and `--worker-wait address` in separate result folders.
It sets `SILVER_WORKER_WAIT=poll` for the former; the latter selects the default
address backend and verifies `worker_wait_backend` in the JSON. Read the override
once before creating an instance; it is not saved in plugin state or preferences.
Reports record the actual backend even with diagnostics Off. Do not set the poll
override in the DAW's environment for normal production use.
The `address` request does not bypass KS's polling policy: the runner refuses to
label KS polling as address waiting. Use `--worker-wait default` for a mixed K/KS
matrix and `--models K --worker-wait address` for a forced address comparison.

Compare `worker_wake_latency` (mean, histogram p99 bound and maximum), missing
frames, queue depth, callback deadlines and worker ratio. Longer total idle
waits can be healthy: the worker now sleeps until a block is consumed instead
of repeatedly polling a full queue. Do not treat the change in average
`worker_sleep` alone as a speedup. Preserve an executable/hash from before the
change as a further control; the same-binary poll override includes the new
notification telemetry and is not bit-identical to the original polling code.

## Windows scheduler traces for KS

The 2026-10-15 comparison found the sustained failure in the frozen original,
the rejected address prototype and its forced-poll control. See the journal's
individual cases: heavy failures coincide with near-continuous execution on an
E-core, while reconstructed ready delay is small. This is evidence for the next
plugin investigation, not a runtime fix. Keep KS production polling for now.

Use the standalone native ETW controller when built-in WPR 19041 fails to save
with `0x80010106`. The PowerShell WPR wrapper is retained as an unsuccessful
experiment; it does not repair that COM failure. Do not change registry policy,
install components, restart Windows, change power settings or apply affinity
as part of this measurement workflow.

In a Visual Studio developer shell at the repository root, **before recording**:

```powershell
New-Item -ItemType Directory -Force output/perf-scheduler | Out-Null
cl /nologo /std:c++17 /EHsc /O2 tools/record_scheduler_etw.cpp /Fo:output/perf-scheduler/record_scheduler_etw.obj /Fe:output/perf-scheduler/record_scheduler_etw.exe /link advapi32.lib rpcrt4.lib
cl /nologo /std:c++17 /EHsc /O2 tools/decode_scheduler_etw.cpp /Fo:output/perf-scheduler/decode_scheduler_etw.obj /Fe:output/perf-scheduler/decode_scheduler_etw.exe /link advapi32.lib tdh.lib
```

Only elevate the recorder. Continue running the benchmark from the ordinary
shell. The recorder has a private logger GUID/name, automatically stops after
the requested duration (maximum 600 s), accepts a STOP file, caps its sequential
ETL at 512 MB and stops only its own session handle. It never calls a system-wide
WPR cancel, changes the sampling interval or modifies scheduling policy.
Use a fresh directory and retain all existing evidence:

```powershell
$traceDirectory = Join-Path $PWD ('output/perf-scheduler/ks-' + (Get-Date -Format yyyyMMdd-HHmmss))
New-Item -ItemType Directory $traceDirectory | Out-Null
$traceExe = Join-Path $PWD output/perf-scheduler/record_scheduler_etw.exe
Start-Process $traceExe -Verb RunAs -WindowStyle Hidden -ArgumentList ('"' + $traceDirectory + '" 240 sampled')
# Confirm Windows UAC and wait for native-status.txt to say "recording".
# Omit "sampled" for the minimal CSwitch/ReadyThread trace.
```

Then run `tools/run_scheduler_comparison.py` in the unelevated shell with
`--session $traceDirectory`, `--original` and `--candidate` executable paths,
plus `--k-rom`, `--ks-rom`, `--flash` and `--wave` private input paths. It records
CPU topology, PID, process/session boundaries and executable hashes, then runs
the same eight interleaved KS / 64 / Low / Summary / 12 s cases used in the journal.
Exit 1 can be the desired reproduced audio failure: retain its reports.

The candidate must be the archived **rejected KS address prototype**, whose hash
is in the journal. The current production binary always polls for KS, and the
runner refuses to label it as an address run. Do not modify the production
source to recreate that experiment. Both executable versions must support the
existing fixture's diagnostic JSON interface and use the same private assets.

After `native-status.txt` says `complete 0`, check `native-recorder.log` for zero
EventsLost / LogBuffersLost / RealTimeBuffersLost. Failed start/stop or lost events
must not be presented as a valid precise scheduling comparison. Then decode
offline, to new filenames:

```powershell
& output/perf-scheduler/decode_scheduler_etw.exe (Join-Path $traceDirectory scheduler.etl) (Join-Path $traceDirectory events.tsv) (Join-Path $traceDirectory schemas.txt)
python tools/analyze_scheduler_trace.py $traceDirectory
```

The decoder also requires zero unsupported records. Known fixed layouts are
audited in `schemas.txt` when TDH MOF lookup is unavailable; unknown versions or
sizes fail the decode. The analyzer qualifies the worker name by process and
lifetime, reconstructs ReadyThread-to-dispatch and ready preemption intervals,
and separates waiting from scheduled time. Its steady window is diagnostic
seconds 4–12; the fixture's missing-frame warm-up boundary is approximate.
It currently supports x64, one processor group. Use WPA for other layouts.

`scheduled_ms` includes any interrupts during a scheduled slice; it is not exact
thread CPU time. CPU samples separately count user/kernel addresses, DPC/ISR
flags and observed priority, without symbolizing or tuning JIT code. Compare
core placement and 250 ms bins alongside missing frames; avoid interpreting
run/wait totals alone as proof of causation. No per-missing-block timeline is
available in the archived executables. Trace overhead, power/thermal state and
case order remain confounders. Raw ETLs, process metadata and private fixture
data stay under ignored local output, with only sanitized findings in Git.

## Opt-in KS P-core placement experiment (Windows)

`SILVER_KS_WORKER_CORES=p` requests performance-class CPU Sets for the **KS
emulation worker only**, after the original boot/warm-up. Default/unset retains
normal scheduling; K and non-Windows behavior are unchanged. This is an
experiment, not a saved plugin preference or an automatic production policy.
No priority, wait strategy, reserve, JIT or guest timing changes accompany it.

The worker enumerates topology using `GetSystemCpuSetInformation`, selects the
highest [EfficiencyClass](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_cpu_set_information)
on a heterogeneous single-group system, and calls
[`SetThreadSelectedCpuSets`](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadselectedcpusets)
on the current worker. It respects the existing
process/thread hard mask and declines to override an explicit host CPU-set
assignment. Unavailable APIs, homogeneous/multiple-group topology and unavailable
P-cores fall back with a recorded status. CPU-set IDs are discovered at runtime.
No process-default, system policy or hard-affinity setter is used.

Reports contain `worker_placement` in the session and at the top level:
`requested`, `status`, `win32_error`, `selected_cpu_set_ids`. Status `p-cpu-sets`
means the API accepted the request; actual P/E execution must still be checked
in ETW because CPU Sets are compatible with Windows power management.

For matched tests, the scheduler runner accepts `--comparison placement`.
Pass the frozen original as `--original` and the new opt-in-capable fixture as
`--candidate`. It interleaves three same-binary default controls, three P-core
requests, and two frozen controls. All use the original KS polling wait.
`--untraced` runs the identical sequence in a fresh directory without starting
a collector; otherwise use the bounded native ETW recorder as above. A P-core
request that did not apply is rejected rather than mislabeled. Keep every
control and failed report, and verify topology/placement before drawing a result.

The 2026-10-15 matched experiment found zero steady misses in all six P-mode
runs (three traced, three untraced). ETW confirmed P-only execution for the traced
runs. Two untraced P runs still lost 78 and 33 frames in the initial ~4 s window.
Same-binary untraced default controls lost 138817 and 303678 steady frames in
two runs. This supports the placement hypothesis on this machine, while leaving
startup losses, real DAW/multiple-instance behavior and other platforms open.
The general matrix runner also accepts `--worker-cores p`; default clears an
inherited override. It checks KS's applied status and retains fallback reports.

## KS H8 bus capture and recorded C++ controller prototype

The normal plugin still defaults to original H8. Recorded-scenario playback
has no plugin preference or environment switch (the separate live hybrid is
described below). The explicit Board API and
private-input `ksControllerProbe_test` fixture opt into a finite recorded
scenario: `h8` is the uninstrumented reference, `record` captures original H8,
and `replay` executes the captured host-port accesses in C++ without stepping H8.
`boot` captures original boot only; pre-boot playback is not implemented.
Use a fresh Board with original H8 to disable the experiment. Do not switch a
replayed Board back to H8: its CPU/peripheral state has intentionally frozen.

Recording covers both DSP host windows, reads and writes, original controller
cycle/PC, byte offset/value, write acceptance and actual H8 slice boundaries.
The existing DSP catch-up and serial ordering remain common to both modes.
Playback checks every returned DSP byte and write acceptance. Mismatch, trace
exhaustion or a budget other than the original 128 cycles stops playback;
there is no silent fallback. Live MIDI/part-MIDI is rejected, and state/Flash
queries and boot are unavailable in this mode. It cannot yet replace H8 for
arbitrary playing, patch edits, LCD, MIDI output or session restoration.

The starting fingerprint covers firmware/Flash/waves, selected H8 RAM and IO,
clocks, serial state, queue sizes and DSP counters. It is **not** a complete
serialized machine snapshot: registers, DSP memories and queue contents are
not all hashed. Always recreate the identical deterministic setup with the
same executable and inputs; passing the fingerprint alone does not prove
equivalence. Compare all 24 audio streams, DSP counters and port responses.

The recorder reserves bounded buffers before capture (3 million events and
1 million slices, about 68.7 MiB payload at the default maximum); overflow makes
the trace incomplete and unusable. There is no file IO in the measured render.
`KSHTR001` files have explicit little-endian fields, bounded counts/length and
a checksum. Existing trace/report files are refused. Raw values can contain
private firmware and presets: keep tapes, reports and binaries under ignored
`output/perf-controller/`, never in Git.

Build `ksControllerProbe_test` explicitly (excluded from the normal build).
The ROM-free `ksControllerTrace_test` / CTest `ks_controller_trace` verifies the
format, both DSP routes and failure guards without private assets. Example:

```powershell
cmake --build build-nmake --target ksControllerProbe_test ksControllerTrace_test
python tools/run_controller_comparison.py --executable build-nmake/source/novation/silverK/buildStandalone/silverPlugin/ksControllerProbe_test.exe --ks-rom output/perf-dsp-threads/private-KSOS2102.SYX --flash "$env:USERPROFILE/Documents/Silver-K/ROMs/ks-init-flash.bin" --wave "$env:USERPROFILE/Documents/Silver-K/ROMs/ks-wave-zero.bin"
```

The runner creates a fresh directory, captures boot, then Single/four-part
passages and three interleaved H8/replay pairs per passage. Every process first
boots and prepares through H8; the measured 90,681-frame native passage is
about two seconds, eight notes total, with sustain, program and control changes.
Boot/setup, trace loading/saving and report generation are excluded. Windows
`GetProcessTimes` counts whole-process user+kernel CPU; wall duration is separate.
On macOS CPU accounting is not implemented: zero in this field is unavailable,
not a measured saving. Build/audio portability still requires validation there.

Default comparison uses sequential DSPs to isolate the controller. Add
`--dsp-threads 2` for an independent compatibility check; do not pool those
results. The runner clears inherited placement/wait overrides and never changes
OS policy. Stop concurrent builds/tests before collecting performance evidence.
It requires matching audio hashes/peaks, frame counts, DSP cycles/instructions,
virtual controller endpoint and serial counts; replay must consume every port
and keep original H8 cycles frozen. These checks do not compare complete DSP
memory. Early reports named their aggregate check `exact_audio_and_dsp_state`;
the runner now accurately names it `exact_audio_and_dsp_counters`.

Recorded commands precompute the original firmware decisions. Any saving is
evidence for this finite playback prototype, **not** an estimate for a completed
live native controller. Next work is to decode the observed flag/data protocol
and implement live MIDI-to-DSP decisions against the H8 reference.

## Opt-in live KS note/sustain controller (hybrid)

`SILVER_KS_CONTROLLER=native-midi` opts a newly created KS instance into the live
C++ MIDI decoder and Note On/Off/sustain handlers after original H8 boot. Unset it and recreate the instance
to return to original H8. K is unchanged. This process-local environment switch
is not a saved preference and does not change Windows settings. The explicit
worker-owned `Board::enableLiveMidiController(bool)` API can also switch at a
quiescent boundary: the guest ABI stays current, including partial messages.
Recorded playback and this live mode are mutually exclusive.
Performance session/root JSON records `controller_backend` as `original-h8`
or `live-cpp-midi-hybrid` (K: not applicable). The existing H8 timing bucket
includes this C++ controller work when the hybrid is selected; it is not an
isolated measurement of H8 instructions in that mode.

This stage ports the OS 2.1.02 byte decoder, Note On/Off decisions and key/pedal
state into C++. It handles keyboard range, velocity-zero release, ordered key
and velocity memory, physical/held bitmaps, mono replacement-note search,
sustain threshold 63/64, pedal-up held-voice release and part release/reset.
It accepts live values and does not load a tape. UART, channel/part routing,
DSP voice allocation/parameter calculations and upload, arpeggiator timing and
other CC-to-parameter conversion still run in original H8.
Other message types (program, pressure, bend, SysEx, system common/realtime)
retain their original H8 paths. It is **not** a complete native DSP controller
and cannot run with H8 stopped. No H8 CPU-core or DSP JIT source/config changed.

The C++ controller preserves original instruction boundaries, fetch/data bus
cycles, registers, flags and call ABI. Pending interrupts go through original
H8 arbitration; both DSPs keep the same slice targets and serial ordering.
A fingerprint rejects unverified parser, note/pedal and helper revisions. This
conservative ABI bridge allows the original handlers to remain the oracle
while later stages port DSP voice/parameter logic. It is a compatibility step;
do not reuse the recorded-playback saving as a live-controller result.

`ksLiveMidiController_test` is a private-input isolated differential test:

```powershell
cmake --build build-nmake --target ksLiveMidiController_test ksControllerProbe_test
& build-nmake/source/novation/silverK/buildStandalone/silverPlugin/ksLiveMidiController_test.exe output/perf-dsp-threads/private-KSOS2102.SYX
python tools/run_live_controller_comparison.py --executable build-nmake/source/novation/silverK/buildStandalone/silverPlugin/ksControllerProbe_test.exe --ks-rom output/perf-dsp-threads/private-KSOS2102.SYX --flash "$env:USERPROFILE/Documents/Silver-K/ROMs/ks-init-flash.bin" --wave "$env:USERPROFILE/Documents/Silver-K/ROMs/ks-wave-zero.bin"
```

The isolated test compares C++ against independent original-H8 decoding at
every boundary (registers/flags/PC/clocks and each bus access), all 16 channels,
all 128 data values, varied velocities/CC values and running status. No private
firmware is embedded. The no-argument CTest returns skip 77; run it explicitly
with private OS for evidence. Normal CI cannot validate this private-ROM case.
It also compares all 462 fixed note/pedal transitions, 32 varied operand/flag
sets each, at their real call boundaries. No DSP engine is stubbed to make the
full-Board audio comparison pass.

The runner first compares original-H8 `record` with `live-record`: complete
binary tapes must match byte-for-byte, including reads, writes, acceptance,
cycle/PC and slice boundaries. It then runs three interleaved unrecorded pairs,
requiring exact audio hashes/peaks across all 24 streams, guest and serial
counters. `single`/`multi` retain the earlier passage; `midi` adds fragmented
messages, running status, repeated pitches, velocity zero, CC 1/7/10/11/64/
105/120/123, program and system-message fallback. Eight notes total maximum in
the chord passages; no claim for all possible patches/voices/CC combinations.
`pedal` exercises four parts with Mono, Mono autoglide, Poly and Poly retrigger,
key range 36..84 plus out-of-range notes, repeated keys, velocity zero, pedal
63/64/127/0 and repeated pedal changes, CC120/123 with and without sustain.
Default `--seconds 4` renders 181,362 native frames for recordings and each
CPU/audio run; `--seconds 2` reproduces the shorter work. Recording is bounded
to 2..4 seconds to fit the existing trace capacities, with overflow rejected.
Reports expose native decoder and actual note/pedal-handler counts. Add
`--dsp-threads 2 --repeats 1` for separate helper compatibility validation.
Raw tapes/failed trials stay ignored, following the same private-data rules.
Add `--toggle --repeats 1` to verify disabling at 0.65 s and re-enabling at
0.95 s against the same uninterrupted H8 reference. The isolated differential
test also alternates native/original execution inside partial messages.

## Keeping results current

Update `doc/OPTIMIZATION_LOG.md` for each implementation or measurement round.
Keep raw reports under ignored `output/perf/` or elsewhere locally; commit only
sanitized findings and the exact reproduction conditions. Build metadata records
the Git revision and dirty state at CMake configure time, configuration and
compiler. Reconfigure after source changes; on a dirty tree attach the patch or
commit it before comparisons. Dependencies may be overridden with local source:
the report's DSP revision is the repository pin, not verification of an override.
