#!/usr/bin/env python3
"""Run the existing K/KS realtime fixture sequentially on Windows or macOS.

Private ROM arguments and raw reports remain local. No third-party dependencies.
Exit status 1 means at least one fixture failed or report was missing, even when
a useful diagnostic report was produced. Off runs supply baseline fixture stats.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    for name in ('k-rom', 'ks-rom', 'flash', 'wave'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--output', type=Path, default=Path('output/perf'))
    parser.add_argument('--blocks', type=int, nargs='+', default=[128, 256, 512])
    parser.add_argument('--models', choices=['K', 'KS'], nargs='+', default=['K', 'KS'])
    parser.add_argument('--modes', choices=['off', 'summary', 'detailed'], nargs='+', default=['off', 'summary', 'detailed'])
    parser.add_argument('--repeats', type=int, default=1)
    parser.add_argument('--profile', type=int, choices=range(4), default=1)
    parser.add_argument('--save-state', action='store_true')
    parser.add_argument('--worker-wait', choices=['default', 'poll', 'address'], default='default',
                        help='Windows A/B override, read once per instance; default uses the production backend')
    parser.add_argument('--worker-cores', choices=['default','p'], default='default',
                        help='Opt-in Windows KS-worker CPU Sets experiment; actual application is verified in JSON')
    parser.add_argument('--dsp-threads', choices=['1','2'], default='1',
                        help='Opt-in KS dual DSP workers; 1 retains the original sequential engine')
    parser.add_argument('--timeout', type=int, default=600)
    args = parser.parse_args()
    if args.repeats < 1 or args.timeout < 1 or any(b < 1 or b > 4096 for b in args.blocks):
        parser.error('repeats/timeout must be positive; blocks must be 1..4096')
    inputs = [args.executable, args.k_rom, args.ks_rom, args.flash, args.wave]
    if any(not p.is_file() for p in inputs):
        parser.error('executable and all four private input files must exist')
    # A new folder prevents overwriting previous evidence and keeps user settings
    # isolated from the standalone/DAW defaults, including firmware-load writes.
    output = args.output.resolve() / time.strftime('%Y%m%d-%H%M%S')
    output.mkdir(parents=True, exist_ok=False)
    command = [str(p.resolve()) for p in inputs]
    results = []
    failed = False
    print('Local results:', output, flush=True)
    for model in args.models:
        for block in args.blocks:
            for mode in args.modes:
                for repeat in range(1, args.repeats + 1):
                    test_id = f'{sys.platform}-{model}-{block}-{mode}-r{repeat}'
                    env = os.environ.copy()
                    env.pop('SILVER_WORKER_WAIT', None)
                    env.pop('SILVER_KS_WORKER_CORES', None)
                    env.pop('SILVER_KS_DSP_THREADS', None)
                    if args.dsp_threads == '2':
                        env['SILVER_KS_DSP_THREADS'] = '2'
                    if args.worker_cores == 'p':
                        env['SILVER_KS_WORKER_CORES'] = 'p'
                    if args.worker_wait != 'default':
                        env['SILVER_WORKER_WAIT'] = args.worker_wait
                    env.update(SILVER_K_DATA=str(output / 'fixture-data'),
                               SILVER_REALTIME_K=str(int(model == 'K')),
                               SILVER_REALTIME_BLOCK=str(block),
                               SILVER_REALTIME_PREPARED_BLOCK=str(block),
                               SILVER_REALTIME_PROFILE=str(args.profile),
                               SILVER_REALTIME_SAVE=str(int(args.save_state)),
                               SILVER_REALTIME_SWEEP='0',
                               SILVER_PERF_MODE=str({'off': 0, 'summary': 1, 'detailed': 2}[mode]),
                               SILVER_PERF_REPORT=str(output / (test_id + '.json')))
                    print('Running', test_id, flush=True)
                    try:
                        run = subprocess.run(command, env=env, capture_output=True,
                                             timeout=args.timeout, encoding='utf-8', errors='replace')
                        code, log = run.returncode, run.stdout + run.stderr
                    except subprocess.TimeoutExpired:
                        code, log = 124, 'Fixture timed out; no performance conclusion.\n'
                    (output / (test_id + '.log')).write_text(log, encoding='utf-8')
                    path = output / (test_id + '.json')
                    row = dict(test_id=test_id, model=model, block=block, mode=mode,
                               profile=args.profile, save_state=args.save_state, returncode=code)
                    row['worker_wait_requested'] = args.worker_wait
                    row['worker_cores_requested'] = args.worker_cores
                    row['dsp_threads_requested'] = args.dsp_threads
                    if path.is_file():
                        try:
                            report = json.loads(path.read_text(encoding='utf-8-sig'))
                            if report.get('schema') != 'silver-k.performance.v1':
                                raise ValueError('Unexpected report schema')
                            if mode != 'off' and not report['metrics']['audio_callback']['observations']:
                                raise ValueError('Recording has no audio observations')
                            row['report'] = path.name
                            row['worker_wall_to_audio_ratio'] = report['worker_wall_to_audio_ratio']
                            row['missing_frames'] = report['metrics']['missing_frames']['total']
                            row['worker_wait_backend'] = (report.get('session') or {}).get('worker_wait_backend', report.get('worker_wait_backend', 'not_recorded'))
                            row['worker_placement'] = report.get('worker_placement')
                            row['dsp_execution_backend'] = report.get('dsp_execution_backend', 'not_recorded')
                            if args.dsp_threads == '2' and model == 'KS' and row['dsp_execution_backend'] != 'two-workers-slice-barrier':
                                raise ValueError('Two DSP workers were not enabled')
                            if args.worker_cores == 'p' and model == 'KS' and (row['worker_placement'] or {}).get('status') != 'p-cpu-sets':
                                raise ValueError('P-core CPU Sets were not applied; inspect fallback status')
                            wake = report['metrics'].get('worker_wake_latency', {})
                            row['wake_observations'] = wake.get('observations', 0)
                            row['wake_mean_us'] = wake['mean'] / 1000 if row['wake_observations'] else None
                            row['wake_max_us'] = wake['maximum'] / 1000 if row['wake_observations'] else None
                            if args.worker_wait == 'address' and row['worker_wait_backend'] != 'windows-address':
                                raise ValueError('Requested address wait is not active; cannot label this an address run')
                        except (ValueError, KeyError, TypeError) as error:
                            row['error'] = str(error)
                    else:
                        row['error'] = 'Missing report'
                    row['fixture_summary'] = next((line for line in log.splitlines() if line.startswith('model=')), '')
                    failed |= code != 0 or 'error' in row
                    results.append(row)
                    (output / 'index.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
                    print(json.dumps(row), flush=True)
    return int(failed)


if __name__ == '__main__':
    raise SystemExit(main())
