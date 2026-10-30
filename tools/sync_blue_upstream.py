#!/usr/bin/env python3
"""Daily upstream check; prepare a review branch, never merge main."""
import json
import os
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = 'temaniak/Silver-K'
BRANCH = 'automation/upstream-sync'
STATE = ROOT / 'blue/upstream-state.json'

def run(*args, check=True):
    return subprocess.run(args, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, check=check)

def api(endpoint):
    return json.loads(run('gh', 'api', endpoint).stdout)

def main():
    repo = os.environ.get('GH_REPO', '')
    if repo != 'RobCZart82/Silver-K-Blue':
        raise RuntimeError('Run only against the intended Blue repository')
    metadata = api('repos/' + repo)
    base = metadata['default_branch']
    state = json.loads(STATE.read_text())
    run('git', 'config', 'user.name', 'github-actions[bot]')
    run('git', 'config', 'user.email', '41898282+github-actions[bot]@users.noreply.github.com')
    run('git', 'remote', 'add', 'upstream', 'https://github.com/' + UPSTREAM + '.git', check=False)
    run('git', 'fetch', 'upstream', 'main', '--tags')
    upstream = run('git', 'rev-parse', 'upstream/main').stdout.strip()
    latest = api('repos/' + UPSTREAM + '/releases/latest')
    release = {key: latest[key] for key in ['tag_name', 'published_at', 'html_url']}
    # Ancestry is authoritative, rather than just a previously observed hash.
    integrated = run('git', 'merge-base', '--is-ancestor', upstream, 'HEAD', check=False).returncode == 0
    if integrated and state['latestRelease'] == release:
        print('No new upstream main commit or stable release. Main unchanged.')
        summary = os.environ.get('GITHUB_STEP_SUMMARY')
        if summary:
            with open(summary, 'a', encoding='utf-8') as stream:
                stream.write('No new upstream main changes or stable release.\n')
        return
    run('git', 'fetch', 'origin', base)
    old = run('git', 'ls-remote', '--heads', 'origin', 'refs/heads/' + BRANCH).stdout.split()
    expected = old[0] if old else ''
    if expected:
        run('git', 'fetch', 'origin', BRANCH)
        author = run('git', 'log', '-1', '--format=%ae', 'FETCH_HEAD').stdout.strip()
        if author != '41898282+github-actions[bot]@users.noreply.github.com':
            print('Proposal branch has human changes; preserve it for manual review instead of resetting it.')
            return
    run('git', 'checkout', '-B', BRANCH, 'origin/' + base)
    conflicts = []
    if not integrated:
        merged = run('git', 'merge', '--no-edit', 'upstream/main', check=False)
        if merged.returncode:
            conflicts = run('git', 'diff', '--name-only', '--diff-filter=U').stdout.splitlines()
            run('git', 'merge', '--abort')
            if not conflicts:
                raise RuntimeError('Merge failed without file conflicts: ' + merged.stderr)
    status = 'Manual conflict resolution required' if conflicts else 'Upstream merged into proposal branch; review and testing required'
    report = ['# Upstream update proposal', '', status, '',
              'Upstream main: `' + upstream + '`',
              'Latest stable release: [' + release['tag_name'] + '](' + release['html_url'] + ')', '',
              'This automation never merges the main branch and never publishes releases.',
              'Keep the Blue skins and local UI changes. Review changes, run the Windows workflow on this branch',
              '(disable Release saving during testing), and test macOS before approving this update.', '']
    if conflicts:
        report += ['## Conflicting files', ''] + ['- `' + path + '`' for path in conflicts]
        report += ['', 'Only this report is proposed; conflicting upstream source changes have not been imported.',
                   'Resolve the upstream merge locally on this proposal branch before review.']
    else:
        state.update(upstreamCommit=upstream, latestRelease=release)
        STATE.write_text(json.dumps(state, indent=2) + '\n')
    (ROOT / 'blue/UPSTREAM-UPDATE.md').write_text('\n'.join(report) + '\n')
    run('git', 'add', 'blue/upstream-state.json', 'blue/UPSTREAM-UPDATE.md')
    if run('git', 'diff', '--cached', '--quiet', check=False).returncode:
        run('git', 'commit', '-m', 'Record upstream update proposal and validation requirements')
    # This is a reserved generated branch. Explicit lease prevents overwriting unseen remote edits.
    unchanged = False
    if expected:
        run('git', 'fetch', 'origin', BRANCH)
        current_tree = run('git', 'rev-parse', 'HEAD^{tree}').stdout.strip()
        old_tree = run('git', 'rev-parse', 'FETCH_HEAD^{tree}').stdout.strip()
        if current_tree == old_tree:
            unchanged = True
    if not unchanged:
        run('git', 'push', '--force-with-lease=refs/heads/' + BRANCH + ':' + expected,
            'origin', 'HEAD:refs/heads/' + BRANCH)
    compare = 'https://github.com/' + repo + '/compare/' + base + '...' + BRANCH
    print(status)
    print('Review changes and open a PR manually:', compare)
    summary = os.environ.get('GITHUB_STEP_SUMMARY')
    if summary:
        with open(summary, 'a', encoding='utf-8') as stream:
            stream.write('## Upstream update proposal\n\n' + status + '\n\n[Review changes / create a PR](' + compare + ')\n')

if __name__ == '__main__':
    main()
