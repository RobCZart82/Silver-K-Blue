#!/usr/bin/env python3
"""Publish a single release from two verified main-branch CI artifacts."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile

REPO = 'RobCZart82/Silver-K-Blue'
ROOT = Path(__file__).resolve().parents[1]


def run(*args):
    return subprocess.check_output(args, cwd=ROOT, text=True).strip()


def validate_runs(windows, macos):
    for record, workflow in ((windows, 'windows-blue.yml'), (macos, 'macos-blue.yml')):
        if (record.get('conclusion') != 'success' or record.get('status') != 'completed'
                or record.get('head_branch') != 'main'
                or record.get('event') not in ('push', 'workflow_dispatch')
                or record.get('path') != '.github/workflows/' + workflow
                or record.get('repository', {}).get('full_name') != REPO):
            raise ValueError('Expected a successful main-branch platform workflow in the intended repository')
    sha = windows['head_sha']
    if sha != macos['head_sha'] or not re.fullmatch('[0-9a-f]{40}', sha):
        raise ValueError('Platform builds must use exactly the same source commit')
    return sha


def verify_package(path, sha, platform):
    with zipfile.ZipFile(path) as bundle:
        if bundle.testzip():
            raise ValueError('Corrupt package')
        infos = [n for n in bundle.namelist() if n.endswith('/build-info.json')]
        if len(infos) != 1:
            raise ValueError('Expected exactly one build-info.json')
        info = json.loads(bundle.read(infos[0]))
        if (info.get('commit') != sha or info.get('platform') != platform
                or info.get('repository') != 'https://github.com/' + REPO
                or info.get('skinsEmbedded') is not True or info.get('firmwareIncluded') is not False):
            raise ValueError('Package provenance mismatch')
        prefix = infos[0][:-len('build-info.json')]
        sums = json.loads(bundle.read(prefix + 'SHA256SUMS.json'))
        actual = {n[len(prefix):] for n in bundle.namelist()
                  if not n.endswith('/') and n != prefix + 'SHA256SUMS.json'}
        if set(sums) != actual:
            raise ValueError('Incomplete package checksum inventory')
        for name, digest in sums.items():
            if hashlib.sha256(bundle.read(prefix + name)).hexdigest() != digest:
                raise ValueError('Package checksum mismatch: ' + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--windows-run', required=True, type=int)
    parser.add_argument('--macos-run', required=True, type=int)
    parser.add_argument('--version', required=True)
    args = parser.parse_args()
    if not re.fullmatch(r'\d+\.\d+\.\d+', args.version):
        raise ValueError('Use a new numeric release version, e.g. 1.0.1')
    records = [json.loads(run('gh', 'api', f'repos/{REPO}/actions/runs/{n}'))
               for n in (args.windows_run, args.macos_run)]
    sha = validate_runs(*records)
    subprocess.run(['git', 'merge-base', '--is-ancestor', sha, 'origin/main'], cwd=ROOT, check=True)
    tag = 'blue-v' + args.version
    # Refuse accidental replacement of an existing release (drafts included).
    releases = json.loads(run('gh', 'api', '--paginate', '--slurp', f'repos/{REPO}/releases?per_page=100'))
    if any(r['tag_name'] == tag for page in releases for r in page):
        raise ValueError('Release already exists; existing assets will not be overwritten')
    with tempfile.TemporaryDirectory(prefix='blue-release-') as temp:
        stage = Path(temp)
        assets = []
        for record, platform, artifact in zip(records, ('Windows-x64', 'macOS-Universal2'), ('Windows-x64', 'macOS-Universal2')):
            dest = stage / platform
            run('gh', 'run', 'download', str(record['id']), '--repo', REPO,
                '--name', f'Silver-K-Blue-{artifact}-{record["run_number"]}', '--dir', str(dest))
            packages = list(dest.glob('*.zip'))
            if len(packages) != 1:
                raise ValueError('Expected one platform ZIP')
            verify_package(packages[0], sha, platform)
            assets.append(packages[0])
        source = stage / f'Silver-K-Blue-{args.version}-Source.zip'
        subprocess.run(['git', 'archive', '--format=zip', '--prefix=Silver-K-Blue-source/',
                        '-o', str(source), sha], cwd=ROOT, check=True)
        with zipfile.ZipFile(source) as bundle:
            for required in ('CMakeLists.txt', 'tools/embed_blue_skins.py', 'blue/Artwork/Silver-K-Blue-icon.png',
                             'blue/Skins/Silver-K Blue Standard v1.0/skin.json',
                             'blue/Skins/Silver-K Blue Retina 4K v1.0/skin.json'):
                bundle.getinfo('Silver-K-Blue-source/' + required)
        assets.append(source)
        checks = stage / 'SHA256SUMS.txt'
        checks.write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest() + '  ' + p.name + '\n' for p in assets))
        notes = stage / 'release-notes.md'
        notes.write_text(f'''Silver-K Blue {args.version}\n\nSource commit: `{sha}`. Both platforms use this exact commit.\n\n- Windows x64: VST3 and Standalone.\n- macOS Universal 2 (Apple Silicon and Intel): AU, VST3 and Standalone; ad hoc signed, not notarized.\n- Both Blue skins are embedded. Source ZIP and SHA256SUMS.txt included. Build dependencies are fetched from pinned revisions.\n- Own compatible firmware required; proprietary firmware and factory sounds are not included.\n\nPlatform CI and ROM-free tests passed. Firmware-dependent skipped checks are not passes. Automated checks do not establish all DAW/audio or physical Intel acceptance.\n\nValidation: [Windows]({records[0]['html_url']}), [macOS]({records[1]['html_url']}).\n''')
        # Keep partial uploads hidden if publication is interrupted.
        run('gh', 'release', 'create', tag, '--repo', REPO, '--target', sha, '--draft',
            '--title', 'Silver-K Blue ' + args.version, '--notes-file', str(notes))
        run('gh', 'release', 'upload', tag, '--repo', REPO, *(str(p) for p in assets), str(checks))
        run('gh', 'release', 'edit', tag, '--repo', REPO, '--draft=false', '--latest')
        print(f'https://github.com/{REPO}/releases/tag/{tag}')


if __name__ == '__main__':
    main()
