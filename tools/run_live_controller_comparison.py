#!/usr/bin/env python3
"""Private KS live C++ note/sustain controller vs original H8; no tape playback.

This is a hybrid: C++ handles notes, key state and sustain; original H8 still
handles UART/routing and DSP voice generation. Compare full bus tapes AND all
audio; never label it H8 removal.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import time


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('executable', 'ks-rom', 'flash', 'wave'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--output', type=Path, default=Path('output/perf-live-controller'))
    p.add_argument('--repeats', type=int, default=3)
    p.add_argument('--scenarios', nargs='+', choices=['single', 'multi', 'midi', 'pedal'], default=['single', 'multi', 'midi', 'pedal'])
    p.add_argument('--seconds', type=float, default=4., help='Same bounded 2..4 s work for bus/audio/CPU')
    p.add_argument('--dsp-threads', choices=['1', '2'], default='1')
    p.add_argument('--toggle', action='store_true', help='Disable at 0.65 s and re-enable at 0.95 s')
    args = p.parse_args()
    if args.repeats < 1 or not 2 <= args.seconds <= 4 or any(not v.is_file() for v in [args.executable, args.ks_rom, args.flash, args.wave]):
        p.error('Positive repeats and existing executable/private inputs required')
    root = args.output.resolve() / time.strftime('%Y%m%d-%H%M%S')
    root.mkdir(parents=True, exist_ok=False)
    exe = args.executable.resolve()
    (root / 'metadata.json').write_text(json.dumps(dict(
        executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
        started_utc=datetime.now(timezone.utc).isoformat(), dsp_threads=args.dsp_threads, toggle=args.toggle, seconds=args.seconds,
        comparison='original H8 vs live C++ notes/key/sustain state; original DSP voice parameter generation'), indent=2))
    print('Local results:', root, flush=True)
    rows, summaries = [], []

    def run(mode, scenario, tag):
        folder = root / tag
        folder.mkdir()
        env = os.environ.copy()
        for name in ('SILVER_WORKER_WAIT', 'SILVER_KS_WORKER_CORES', 'SILVER_KS_DSP_THREADS', 'SILVER_KS_CONTROLLER'):
            env.pop(name, None)
        env['SILVER_KS_DSP_THREADS'] = args.dsp_threads
        report, tape = folder / 'report.json', folder / 'bus.kst'
        print('Running', tag, flush=True)
        with (folder / 'fixture.log').open('w', encoding='utf-8') as log:
            r = subprocess.run([str(exe), str(args.ks_rom.resolve()), str(args.flash.resolve()),
                                str(args.wave.resolve()), str(report), mode, scenario, str(tape), str(args.seconds)],
                               stdout=log, stderr=subprocess.STDOUT, env=env, timeout=120)
        row = dict(test_id=tag, returncode=r.returncode, mode=mode, scenario=scenario)
        if report.is_file():
            row['report'] = json.loads(report.read_text(encoding='utf-8-sig'))
        rows.append(row)
        (root / 'index.json').write_text(json.dumps(rows, indent=2))
        if r.returncode or 'report' not in row:
            raise RuntimeError(f'{tag} failed; inspect retained log; comparison invalid')
        return row['report']

    keys = ['frames', 'audio_hash', 'peak', 'h8_start_cycles', 'h8_end_cycles',
            'controller_end_cycles', 'dsp_a_cycles', 'dsp_b_cycles', 'dsp_a_instructions',
            'dsp_b_instructions', 'serial_depth', 'serial_underruns', 'serial_overflows']

    def equal(a, b, fields):
        for key in fields:
            if a[key] != b[key]:
                raise ValueError(f'{key} differs ({a[key]} / {b[key]}); no equivalence claim')

    for scenario in args.scenarios:
        candidate = 'live-toggle' if args.toggle else 'live'
        before = run('record', scenario, scenario + '-h8-bus')
        after = run(candidate + '-record', scenario, scenario + '-live-bus')
        equal(before, after, keys + ['trace_events', 'trace_slices', 'trace_digest', 'bus'])
        original_tape = root / (scenario + '-h8-bus') / 'bus.kst'
        native_tape = root / (scenario + '-live-bus') / 'bus.kst'
        # Explicit bytes, including cycle/PC/read responses/slice clocks, not
        # merely a matching aggregate transaction count.
        if original_tape.read_bytes() != native_tape.read_bytes():
            raise ValueError('Full H8/native bus tapes differ')
        for repeat in range(args.repeats):
            for mode in (['h8', candidate] if repeat % 2 == 0 else [candidate, 'h8']):
                current = run(mode, scenario, f'{scenario}-r{repeat+1}-{mode}')
                equal(before, current, keys)
                if mode == candidate and current.get('native_midi_messages', 0) == 0:
                    raise ValueError('Live mode handled no MIDI')
        summary = dict(scenario=scenario, full_bus_bytes_identical=True,
                       audio_and_guest_counters_identical=True, frames=before['frames'],
                       audio_hash=before['audio_hash'], trace_events=before['trace_events'],
                       tape_sha256=hashlib.sha256(original_tape.read_bytes()).hexdigest(),
                       native_steps=after['native_midi_steps'], native_messages=after['native_midi_messages'],
                       note_on=after['native_note_on'], note_off=after['native_note_off'], cc=after['native_cc'], modes={},
                       note_steps=after['native_note_steps'], note_on_calls=after['native_note_on_calls'],
                       note_off_calls=after['native_note_off_calls'], sustain_calls=after['native_sustain_calls'],
                       sustain_release_calls=after['native_sustain_release_calls'])
        for mode in ['h8', candidate]:
            group = [r['report'] for r in rows if r['scenario'] == scenario and r['mode'] == mode]
            summary['modes'][mode] = dict(runs=len(group),
                cpu_seconds=[r['cpu_seconds'] for r in group], wall_seconds=[r['wall_seconds'] for r in group],
                median_cpu_seconds=statistics.median(r['cpu_seconds'] for r in group),
                median_wall_seconds=statistics.median(r['wall_seconds'] for r in group))
        if os.name == 'nt':
            summary['cpu_change_percent'] = (summary['modes'][candidate]['median_cpu_seconds'] /
                                             summary['modes']['h8']['median_cpu_seconds'] - 1) * 100
        summaries.append(summary)
        (root / 'summary.json').write_text(json.dumps(summaries, indent=2))
        print(json.dumps(summary, indent=2), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
