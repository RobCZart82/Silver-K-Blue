#!/usr/bin/env python3
"""Interleaved KS dual-worker experiment, ordinary processes and no OS changes.

Uses the existing 12 s, 48 kHz fixture. Original controls and both candidate
modes run serially. No ETW/affinity/priority/power/timer configuration is made.
All private inputs and raw evidence remain local in a fresh output directory.
"""
import argparse
import ctypes
from ctypes import wintypes
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time


def process_cpu_seconds(process):
    if os.name != 'nt':
        return None
    query = ctypes.WinDLL('kernel32', use_last_error=True).GetProcessTimes
    query.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    query.restype = wintypes.BOOL
    times = [wintypes.FILETIME() for _ in range(4)]
    if not query(int(process._handle), *[ctypes.byref(value) for value in times]):
        raise ctypes.WinError(ctypes.get_last_error())
    return sum((value.dwHighDateTime << 32) | value.dwLowDateTime for value in times[2:]) / 1e7


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('original', 'candidate', 'k-rom', 'ks-rom', 'flash', 'wave'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--output', type=Path, default=Path('output/perf-dsp-threads'))
    parser.add_argument('--cases', nargs='+', default=['64:0', '128:1'], help='block:profile pairs')
    parser.add_argument('--mode', choices=['summary', 'detailed'], default='summary')
    args = parser.parse_args()
    inputs = [args.k_rom, args.ks_rom, args.flash, args.wave]
    if any(not value.is_file() for value in [args.original, args.candidate, *inputs]):
        parser.error('All executable/private inputs must exist')
    cases = [tuple(int(value) for value in case.split(':')) for case in args.cases]
    if any(len(case) != 2 or case[0] < 1 or case[0] > 4096 or case[1] not in range(4) for case in cases):
        parser.error('Expected block:profile, block 1..4096 and profile 0..3')
    root = args.output.resolve() / time.strftime('%Y%m%d-%H%M%S')
    root.mkdir(parents=True, exist_ok=False)
    index = []
    variants = [('candidate', '1'), ('candidate', '2'), ('original', '1'), ('candidate', '2'),
                ('candidate', '1'), ('candidate', '2'), ('candidate', '1'), ('original', '1')]
    print('Local results:', root, flush=True)
    for block, profile in cases:
        for number, (variant, threads) in enumerate(variants, 1):
            tag = f'{block}-p{profile}-{args.mode}-{number:02d}-{variant}-t{threads}'
            folder = root / tag
            folder.mkdir()
            exe = (args.original if variant == 'original' else args.candidate).resolve()
            env = os.environ.copy()
            for name in ('SILVER_WORKER_WAIT', 'SILVER_KS_WORKER_CORES', 'SILVER_KS_DSP_THREADS'):
                env.pop(name, None)
            if threads == '2':
                env['SILVER_KS_DSP_THREADS'] = '2'
            env.update(SILVER_K_DATA=str(folder / 'fixture-data'), SILVER_REALTIME_K='0',
                       SILVER_REALTIME_BLOCK=str(block), SILVER_REALTIME_PREPARED_BLOCK=str(block),
                       SILVER_REALTIME_PROFILE=str(profile), SILVER_REALTIME_SAVE='0', SILVER_REALTIME_SWEEP='0',
                       SILVER_PERF_MODE='2' if args.mode == 'detailed' else '1',
                       SILVER_PERF_REPORT=str(folder / 'report.json'))
            row = dict(test_id=tag, variant=variant, dsp_threads=threads, block=block, profile=profile,
                       mode=args.mode, sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                       start_utc=datetime.now(timezone.utc).isoformat())
            start = time.perf_counter()
            print('Running', tag, flush=True)
            with (folder / 'fixture.log').open('w', encoding='utf-8') as log:
                process = subprocess.Popen([str(exe), *[str(value.resolve()) for value in inputs]],
                                           env=env, stdout=log, stderr=subprocess.STDOUT)
                row['pid'] = process.pid
                try:
                    row['returncode'] = process.wait(timeout=90)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
                    row['returncode'] = 124
                row['process_cpu_seconds_including_boot'] = process_cpu_seconds(process)
            row['process_wall_seconds_including_boot'] = time.perf_counter() - start
            report_path = folder / 'report.json'
            if report_path.is_file():
                report = json.loads(report_path.read_text(encoding='utf-8-sig'))
                row.update(worker_ratio=report['worker_wall_to_audio_ratio'],
                           missing=report['metrics']['missing_frames']['total'],
                           stale=report['metrics']['stale_frames']['total'],
                           callback_over_budget=report['metrics']['audio_callback']['over_budget'],
                           backend=report.get('dsp_execution_backend', 'not-recorded'),
                           placement=report.get('worker_placement'))
                if variant == 'candidate' and row['backend'] != ('sequential' if threads == '1' else 'two-workers-slice-barrier'):
                    raise ValueError('Actual DSP execution backend differs from requested mode')
            else:
                row['error'] = 'Missing diagnostic report'
            summary = next((line for line in (folder / 'fixture.log').read_text(errors='replace').splitlines()
                            if line.startswith('model=')), '')
            row['fixture_summary'] = dict(item.split('=', 1) for item in summary.split() if '=' in item)
            index.append(row)
            (root / 'index.json').write_text(json.dumps(index, indent=2), encoding='utf-8')
            print(json.dumps(row), flush=True)
    return int(any(row['returncode'] or 'error' in row for row in index))


if __name__ == '__main__':
    raise SystemExit(main())
