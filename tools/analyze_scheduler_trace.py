#!/usr/bin/env python3
"""Offline analysis of decode_scheduler_etw TSV + local capture-index.json.

Reports scheduled-on-CPU time (including interrupts), reconstructed ready delay,
wait time and CPU placement for the named KS worker. No symbols/JIT analysis.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import sys


def filetime(iso):
    delta = datetime.fromisoformat(iso) - datetime(1601, 1, 1, tzinfo=timezone.utc)
    return (delta.days * 86400 + delta.seconds) * 10_000_000 + delta.microseconds * 10


def overlap(start, end, window):
    return max(0, min(end, window[1]) - max(start, window[0]))


def percentile(values, fraction):
    values = sorted(values)
    return values[max(0, math.ceil(len(values) * fraction) - 1)] if values else None


def analyze(events, begin, end, topology):
    window = (begin, end)
    running = None
    last_out = ready = last_cpu = None
    last_state = None
    cpu_time, priorities, reasons = Counter(), Counter(), Counter()
    ready_delays, waits, gaps = [], [], []
    samples, sample_priorities = Counter(), Counter()
    on_cpu = ready_sum = wait_sum = unknown_sum = 0
    migrations = core_migrations = class_migrations = switches = anomalies = 0
    timeline = [dict(cpu_ticks=0, ready_ticks=0, wait_ticks=0, by_cpu={})
                for _ in range(math.ceil((end-begin)/2_500_000))]

    def accumulate(kind, start, stop, cpu=None):
        for i, row in enumerate(timeline):
            ticks = overlap(start, stop, (begin+i*2_500_000, min(end, begin+(i+1)*2_500_000)))
            row[kind] += ticks
            if kind == 'cpu_ticks' and ticks:
                row['by_cpu'][cpu] = row['by_cpu'].get(cpu, 0) + ticks

    for t, cpu, kind, a, b, c, d, e, f in events:
        if kind == 'sample':
            if begin <= t < end:
                samples['kernel' if b else 'user'] += 1
                samples['dpc'] += bool(c & 1)
                samples['isr'] += bool(c & 2)
                sample_priorities[e] += 1
        elif kind == 'out':
            if running:
                start, run_cpu = running
                ticks = overlap(start, t, window)
                on_cpu += ticks
                cpu_time[run_cpu] += ticks
                accumulate('cpu_ticks', start, t, run_cpu)
                running = None
            elif begin <= t < end:
                anomalies += 1
            last_out, last_state = t, c
            ready = t if c in (1, 3, 7) else None
            if begin <= t < end:
                reasons[f'state={c},reason={d}'] += 1
        elif kind == 'ready':
            # Swapped-out stack/address-space flags are not yet dispatch-ready.
            if not running and not (c & 6) and ready is None:
                ready = t
        elif kind == 'in':
            if last_out is not None:
                if begin <= t < end:
                    gaps.append((t-last_out)/10_000)
                if ready is not None:
                    ready_sum += overlap(ready, t, window)
                    accumulate('ready_ticks', ready, t)
                    if begin <= t < end:
                        ready_delays.append((t-ready)/10)  # microseconds
                    if last_state == 5:
                        wait_sum += overlap(last_out, ready, window)
                        accumulate('wait_ticks', last_out, ready)
                        if begin <= t < end:
                            waits.append((ready-last_out)/10_000)
                else:
                    unknown_sum += overlap(last_out, t, window)
            if begin <= t < end:
                switches += 1
                migrations += last_cpu is not None and last_cpu != cpu
                if last_cpu is not None:
                    previous, current = topology.get(last_cpu, {}), topology.get(cpu, {})
                    core_migrations += previous.get('core') != current.get('core')
                    class_migrations += previous.get('efficiency') != current.get('efficiency')
                priorities[e] += 1
            last_cpu = cpu
            ready = last_out = None
            if running and begin <= t < end:
                anomalies += 1
            running = (t, cpu)
    if running:
        start, cpu = running
        ticks = overlap(start, end, window)
        on_cpu += ticks
        cpu_time[cpu] += ticks
        accumulate('cpu_ticks', start, end, cpu)
    classes = Counter()
    for cpu, ticks in cpu_time.items():
        classes[str(topology.get(cpu, {}).get('efficiency', 'unknown'))] += ticks / 10_000
    return dict(duration_ms=(end-begin)/10_000, scheduled_ms=on_cpu/10_000,
                ready_ms=ready_sum/10_000, wait_ms=wait_sum/10_000,
                unknown_off_ms=unknown_sum/10_000, switches=switches,
                logical_cpu_migrations=migrations, state_anomalies=anomalies,
                physical_core_migrations=core_migrations, efficiency_class_migrations=class_migrations,
                cpu_ms={str(k):v/10_000 for k,v in cpu_time.items() if v},
                efficiency_class_ms=dict(classes), priorities=dict(priorities),
                ready_p99_us=percentile(ready_delays, .99),
                samples=dict(samples), sample_priorities=dict(sample_priorities),
                ready_max_us=max(ready_delays, default=None),
                off_cpu_max_ms=max(gaps, default=None),
                wait_p99_ms=percentile(waits, .99), waits_by_reason=dict(reasons),
                timeline_250ms=timeline)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('session', type=Path)
    parser.add_argument('--events', default='events.tsv')
    parser.add_argument('--output', default='scheduler-analysis.json')
    args = parser.parse_args()
    root = args.session
    if (root/args.output).exists():
        parser.error('Choose a new --output filename; preserve previous evidence')
    cases = json.loads((root/'capture-index.json').read_text(encoding='utf-8-sig'))
    topology = {r['logical']:r for r in json.loads((root/'cpu-topology.json').read_text())}
    if any(r.get('group',0) != 0 for r in topology.values()):
        raise ValueError('This analyzer supports one processor group; use WPA for multiple groups')
    names = {}
    # Retain PID/time qualification: Windows can recycle TIDs across cases.
    for line in (root/args.events).open(encoding='utf-8'):
        fields = line.rstrip('\n').split('\t')
        if fields[2] in ('N', 'T'):
            pid, tid = map(int, fields[3:5])
            if fields[2] == 'N' and len(fields)>9:
                names[(pid, tid)] = fields[9]
    targets = {}
    for case in cases:
        matches = [tid for (pid, tid), name in names.items()
                   if pid == case['pid'] and name == 'KS H8S / dual DSP']
        if len(matches) != 1:
            raise ValueError(f"Expected one named KS worker for PID {case['pid']}: {matches}")
        tid = matches[0]
        targets[case['test_id']] = (tid, [])
    selectors = {}
    for case in cases:
        tid, selected = targets[case['test_id']]
        selectors.setdefault(tid, []).append((filetime(case['start_utc']), filetime(case['end_utc']), selected))
    for line in (root/args.events).open(encoding='utf-8'):
        fields = line.rstrip('\n').split('\t')
        t, cpu = map(int, fields[:2]);kind=fields[2]
        if kind not in ('C', 'R', 'P'):
            continue
        a,b,c,d,e,f = map(int, fields[3:9])
        for tid, direction in ((a,'in'),(b,'out')) if kind == 'C' else ((a,'sample' if kind == 'P' else 'ready'),):
            for start, end, selected in selectors.get(tid, ()):
                if start <= t <= end:
                    selected.append((t,cpu,direction,a,b,c,d,e,f))
    result = []
    for case in cases:
        tid, selected = targets[case['test_id']]
        begin = filetime(case['session']['started_at'])
        row = dict(test_id=case['test_id'], pid=case['pid'], tid=tid,
                   missing=case['missing'], returncode=case['returncode'],
                   worker_ratio=case['worker_ratio'])
        row['full'] = analyze(selected, begin, begin+120_000_000, topology)
        row['steady'] = analyze(selected, begin+40_000_000, begin+120_000_000, topology)
        for label in ('full', 'steady'):
            if row[label]['state_anomalies']:
                print(f"WARNING: {case['test_id']} {label} has {row[label]['state_anomalies']} inconsistent transitions; inspect before using it", file=sys.stderr)
        result.append(row)
        print(json.dumps({**{k:v for k,v in row.items() if k not in ('full','steady')},
                          **{k:v for k,v in row['steady'].items() if k not in ('timeline_250ms','waits_by_reason')} }))
    (root/args.output).write_text(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
