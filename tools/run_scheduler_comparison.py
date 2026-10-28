#!/usr/bin/env python3
"""Run eight KS cases inside an already active, bounded native ETW session.

Run this unelevated. The separate recorder alone needs UAC. All evidence stays
in the fresh session directory. Failed audio fixtures are retained for analysis.
"""
import argparse
import ctypes
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time


def cpu_topology():
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    query = kernel.GetSystemCpuSetInformation
    query.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_ulong), ctypes.c_void_p, ctypes.c_ulong]
    query.restype = ctypes.c_int
    size = ctypes.c_ulong()
    query(None, 0, ctypes.byref(size), None, 0)
    data = ctypes.create_string_buffer(size.value)
    if not query(data, size.value, ctypes.byref(size), None, 0):
        raise ctypes.WinError(ctypes.get_last_error())
    rows, offset = [], 0
    while offset < size.value:
        count = int.from_bytes(data.raw[offset:offset+4], 'little')
        if count < 20 or offset+count > size.value:
            raise ValueError('Invalid SYSTEM_CPU_SET_INFORMATION size')
        item = data.raw[offset:offset+count]
        if int.from_bytes(item[4:8], 'little') == 0:
            rows.append(dict(id=int.from_bytes(item[8:12], 'little'), group=int.from_bytes(item[12:14], 'little'),
                             logical=item[14], core=item[15], efficiency=item[18], flags=item[19]))
        offset += count
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('session', 'original', 'candidate', 'k-rom', 'ks-rom', 'flash', 'wave'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--comparison', choices=['wakeup','placement'], default='wakeup')
    parser.add_argument('--untraced', action='store_true', help='Same cases without an ETW session, to assess observer effects')
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('This recorder workflow is Windows only')
    root = args.session.resolve()
    if any((root/name).exists() for name in ('capture-index.json', 'cpu-topology.json', 'STOP')):
        parser.error('Session already contains benchmark evidence')
    inputs = [args.k_rom, args.ks_rom, args.flash, args.wave]
    if any(not p.is_file() for p in [args.original, args.candidate, *inputs]):
        parser.error('All six executable/private input files must exist')
    def recording():
        return args.untraced or (root/'native-status.txt').read_text().strip() == 'recording'
    if not recording():
        parser.error('Start record_scheduler_etw in this fresh directory first')
    (root/'cpu-topology.json').write_text(json.dumps(cpu_topology(), indent=2))
    variants = [('before','default'),('after','address'),('after','poll'),('after','address'),
                ('before','default'),('after','poll'),('before','default'),('after','address')]
    if args.comparison == 'placement':
        variants = [('after','default'),('after','p'),('before','default'),('after','p'),
                    ('after','default'),('after','p'),('after','default'),('before','default')]
    index = []
    try:
        for number, (variant, wait) in enumerate(variants, 1):
            if not recording():
                raise RuntimeError('Recorder ended before the matrix finished; inspect the recording limit')
            tag = f'{number:02d}-{variant}-{wait}-KS-64-low'
            folder = root/tag
            folder.mkdir()
            exe = (args.original if variant == 'before' else args.candidate).resolve()
            env = os.environ.copy()
            env.pop('SILVER_WORKER_WAIT', None)
            env.pop('SILVER_KS_WORKER_CORES', None)
            env.pop('SILVER_KS_DSP_THREADS', None)
            if args.comparison == 'placement':
                if wait == 'p': env['SILVER_KS_WORKER_CORES'] = 'p'
            elif wait != 'default': env['SILVER_WORKER_WAIT'] = wait
            env.update(SILVER_K_DATA=str(folder/'fixture-data'), SILVER_REALTIME_K='0', SILVER_REALTIME_BLOCK='64',
                       SILVER_REALTIME_PREPARED_BLOCK='64', SILVER_REALTIME_PROFILE='0', SILVER_REALTIME_SAVE='0',
                       SILVER_REALTIME_SWEEP='0', SILVER_PERF_MODE='1', SILVER_PERF_REPORT=str(folder/'report.json'))
            row = dict(test_id=tag, variant=variant, comparison=args.comparison, traced=not args.untraced,
                       wait_requested=wait if args.comparison=='wakeup' else 'default',
                       cores_requested=wait if args.comparison=='placement' else 'default', executable=str(exe),
                       sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                       start_utc=datetime.now(timezone.utc).isoformat(), start_monotonic_ns=time.perf_counter_ns())
            print('Running', tag, flush=True)
            with (folder/'fixture.log').open('w', encoding='utf-8') as log:
                process = subprocess.Popen([str(exe), *[str(p.resolve()) for p in inputs]], env=env, stdout=log, stderr=subprocess.STDOUT)
                row['pid'] = process.pid
                index.append(row)
                (root/'capture-index.json').write_text(json.dumps(index, indent=2))
                try: row['returncode'] = process.wait(timeout=60)
                except subprocess.TimeoutExpired:
                    process.kill(); process.wait(); row['returncode'] = 124
            row.update(end_utc=datetime.now(timezone.utc).isoformat(), end_monotonic_ns=time.perf_counter_ns())
            report = json.loads((folder/'report.json').read_text(encoding='utf-8-sig'))
            backend = report.get('worker_wait_backend')
            if args.comparison=='wakeup' and wait == 'address' and backend != 'windows-address':
                raise ValueError('Candidate did not use address waiting; do not mislabel a production KS polling binary')
            if args.comparison=='placement' and wait == 'p' and report.get('worker_placement',{}).get('status')!='p-cpu-sets':
                raise ValueError('P-core CPU Sets not applied; do not label this as P-core placement')
            row.update(session=report['session'], elapsed_ms=report['elapsed_ms'],
                       missing=report['metrics']['missing_frames']['total'], worker_ratio=report['worker_wall_to_audio_ratio'])
            row['summary'] = next((line for line in (folder/'fixture.log').read_text(errors='replace').splitlines() if line.startswith('model=')), '')
            (root/'capture-index.json').write_text(json.dumps(index, indent=2))
            print(json.dumps(row), flush=True)
    finally:
        if not args.untraced:
            (root/'STOP').write_text('Benchmark cases completed; stop only this session.')
    return int(any(row['returncode'] for row in index))


if __name__ == '__main__':
    raise SystemExit(main())
