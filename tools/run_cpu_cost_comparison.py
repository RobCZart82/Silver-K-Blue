#!/usr/bin/env python3
"""Compare KS CPU seconds on equal, complete audio work; never change OS policy.

Windows fixture counts user+kernel CPU for all process threads and the caller,
excluding setup and warm-up. Reject comparisons with dropped/different audio.
Private firmware and reports remain local in a fresh directory.
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
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('executable', 'ks-rom', 'flash', 'wave'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--output', type=Path, default=Path('output/perf-cpu-cost'))
    parser.add_argument('--modes', nargs='+', choices=['offline', 'paced', 'native'], default=['offline', 'paced'])
    parser.add_argument('--repeats', type=int, default=3)
    args = parser.parse_args()
    if args.repeats < 1 or any(not p.is_file() for p in [args.executable, args.ks_rom, args.flash, args.wave]):
        parser.error('Positive repeats and existing executable/private inputs required')
    root = args.output.resolve() / time.strftime('%Y%m%d-%H%M%S')
    root.mkdir(parents=True, exist_ok=False)
    exe = args.executable.resolve()
    sha = hashlib.sha256(exe.read_bytes()).hexdigest()
    rows = []
    print('Local results:', root, flush=True)
    for mode in args.modes:
        for repeat in range(args.repeats):
            for threads in (['1', '2'] if repeat % 2 == 0 else ['2', '1']):
                tag = f'{mode}-r{repeat+1}-t{threads}'
                folder = root / tag
                folder.mkdir()
                env = os.environ.copy()
                for key in ('SILVER_WORKER_WAIT', 'SILVER_KS_WORKER_CORES', 'SILVER_KS_DSP_THREADS'):
                    env.pop(key, None)
                env.update(SILVER_K_DATA=str(folder / 'fixture-data'), SILVER_KS_DSP_THREADS=threads,
                           SILVER_CPU_PACED='1' if mode == 'paced' else '0')
                command = [str(exe), str(args.ks_rom.resolve()), str(args.flash.resolve()),
                           str(args.wave.resolve()), str(folder / 'report.json')]
                print('Running', tag, flush=True)
                with (folder / 'fixture.log').open('w', encoding='utf-8') as log:
                    try:
                        result = subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=120)
                        code = result.returncode
                    except subprocess.TimeoutExpired:
                        code = 124
                row = dict(test_id=tag, mode=mode, dsp_threads=threads, returncode=code,
                           executable_sha256=sha, completed_utc=datetime.now(timezone.utc).isoformat())
                path = folder / 'report.json'
                if path.is_file():
                    report = json.loads(path.read_text(encoding='utf-8-sig'))
                    if report['schema'] != 'silver-k.cpu-cost.v1':
                        raise ValueError('Unexpected schema')
                    row['report'] = report
                    expected = 'sequential' if threads == '1' else 'two-workers-slice-barrier'
                    if report['dsp_execution_backend'] != expected:
                        raise ValueError('Actual DSP backend differs from requested mode')
                else:
                    row['error'] = 'Missing report'
                rows.append(row)
                (root / 'index.json').write_text(json.dumps(rows, indent=2), encoding='utf-8')
                print(json.dumps(row), flush=True)
                if 'error' in row:
                    raise RuntimeError('Fixture stopped before measurement; inspect retained fixture.log')
    summary = []
    for mode in args.modes:
        group = [r for r in rows if r['mode'] == mode]
        expected_frames = 544085 if mode == 'native' else 576000
        valid = all(r['returncode'] == 0 and r.get('report', {}).get('frames') == expected_frames
                    and r['report']['missing_frames'] == 0 for r in group)
        hashes = {r.get('report', {}).get('audio_hash') for r in group}
        valid = valid and len(hashes) == 1 and None not in hashes
        item = dict(mode=mode, valid_equal_work=valid, audio_hashes=sorted(str(h) for h in hashes), modes={})
        for threads in ['1', '2']:
            reports = [r['report'] for r in group if r['dsp_threads'] == threads and 'report' in r]
            if reports:
                item['modes'][threads] = {name: statistics.median(r[name] for r in reports if name in r)
                                         for name in ['process_cpu_seconds', 'caller_cpu_seconds', 'background_cpu_seconds',
                                                      'wall_seconds', 'setup_cpu_seconds', 'warmup_cpu_seconds'] if all(name in r for r in reports)}
                item['modes'][threads]['runs'] = len(reports)
        if valid:
            before, after = item['modes']['1'], item['modes']['2']
            item['process_cpu_change_percent'] = (after['process_cpu_seconds'] / before['process_cpu_seconds'] - 1) * 100
            if 'background_cpu_seconds' in before and 'background_cpu_seconds' in after:
                item['background_cpu_change_percent'] = (after['background_cpu_seconds'] / before['background_cpu_seconds'] - 1) * 100
            item['wall_change_percent'] = (after['wall_seconds'] / before['wall_seconds'] - 1) * 100
        summary.append(item)
    (root / 'summary.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
    print(json.dumps(summary, indent=2), flush=True)
    return int(any(not item['valid_equal_work'] for item in summary))


if __name__ == '__main__':
    raise SystemExit(main())
