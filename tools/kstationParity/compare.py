"""Private-fixture comparison of frozen Blue H8 / upstream H8 / native probes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

SCENARIOS = ['idle', 'held1', 'held8', 'notes', 'sweep', 'arp', 'program',
             'sustain', 'clockburst', 'expression', 'sysexpatch', 'programstress']
FIELDS = ['frames', 'pcm_fnv', 'flash_fnv', 'midi_out_count', 'midi_out_fnv',
          'before_patch', 'after_patch', 'bank', 'program', 'dropped_frames',
          'before_clock_source', 'after_clock_source', 'before_clock_route',
          'after_clock_route']


def digest(path):
    with path.open('rb') as source:
        checksum = hashlib.sha256()
        for block in iter(lambda: source.read(1048576), b''):
            checksum.update(block)
    return checksum.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('firmware', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--variant', action='append', nargs=3,
                        metavar=('LABEL', 'BINARY', 'MODE'), required=True)
    parser.add_argument('--scenarios', nargs='+', choices=SCENARIOS, default=SCENARIOS)
    args = parser.parse_args()
    variants = [(label, Path(binary).resolve(), mode) for label, binary, mode in args.variant]
    if len(variants) < 2 or len({v[0] for v in variants}) != len(variants):
        parser.error('Supply at least two uniquely labelled variants; first is the reference')
    if any(not label.replace('_', '').replace('-', '').isalnum() or mode not in ('original', 'native')
           for label, _, mode in variants):
        parser.error('Use simple alphanumeric labels and original/native modes')
    firmware = args.firmware.resolve()
    if digest(firmware) != '2ad5409f511e96fbd996901f993f8765ac226efc8930253a76f06edf012f2f07':
        parser.error('Expected decoded K-Station OS 2.1.01 fixture')
    identities = {str(path): digest(path) for path in {firmware, *(v[1] for v in variants)}}
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    summary = {'identities': identities, 'runner_sha256': digest(Path(__file__)),
               'seconds': 2, 'warm_seconds': 2, 'block': 256, 'pacing': 'offline',
               'full_ram_parity_required': False, 'results': [], 'passed': False}
    report_path = args.output / 'summary.json'

    def save():
        temporary = report_path.with_suffix('.tmp')
        temporary.write_text(json.dumps(summary, indent=2) + '\n')
        temporary.replace(report_path)

    save()
    try:
        for scenario in args.scenarios:
            runs = []
            row = {'scenario': scenario, 'runs': runs, 'passed': False}
            summary['results'].append(row)
            for label, binary, mode in variants:
                for path, checksum in identities.items():
                    if digest(Path(path)) != checksum:
                        raise ValueError('Fixture or executable changed during comparison')
                prefix = args.output / f'{scenario}-{label}'
                with prefix.with_suffix('.log').open('w') as log:
                    code = subprocess.run([str(binary), str(firmware), scenario, '2', 'capture',
                                           'offline', '256', str(prefix), '2'],
                                          env=dict(os.environ, K_STAGE1_MODE=mode),
                                          stdout=log, stderr=subprocess.STDOUT).returncode
                current = json.loads(prefix.with_suffix('.json').read_text())
                raw = {ext: digest(prefix.with_suffix('.' + ext)) for ext in ('pcm', 'flash', 'midi')}
                valid = (code == 0 and current['controller_mode'] == mode
                         and current['dropped_frames'] == 0
                         and current['frames'] > 0)
                if mode == 'native':
                    valid = valid and current['interpreted_steps'] == 0 and current['cumulative_interpreted_steps'] == 0
                runs.append({'variant': label, 'exit_code': code, 'valid': valid,
                             'report': current, 'raw_sha256': raw})
                save()
            reference = runs[0]
            for run in runs:
                run['different_fields'] = [key for key in FIELDS if run['report'][key] != reference['report'][key]]
                run['raw_equal'] = run['raw_sha256'] == reference['raw_sha256']
            row['passed'] = all(r['valid'] and not r['different_fields'] and r['raw_equal'] for r in runs)
            save()
            print(scenario, 'PASS' if row['passed'] else 'FAIL', flush=True)
        summary['passed'] = all(row['passed'] for row in summary['results'])
    except (OSError, ValueError, KeyError) as error:
        summary['error'] = str(error)
    finally:
        save()
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
