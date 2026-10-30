#!/usr/bin/env python3
"""Exercise upstream automation against temporary local repositories, no GitHub writes."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location('sync', Path(__file__).with_name('sync_blue_upstream.py'))

def git(folder, *args):
    return subprocess.check_output(['git', '-C', str(folder), *args], text=True, stderr=subprocess.DEVNULL).strip()

class SyncTests(unittest.TestCase):
    def scenario(self, conflict=False, change=True, release_change=False, is_private=True):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, private, upstream = root/'source', root/'private.git', root/'upstream.git'
            source.mkdir()
            git(source, 'init', '-b', 'main')
            git(source, 'config', 'user.name', 'Test')
            git(source, 'config', 'user.email', 'test@example.invalid')
            (source/'code.txt').write_text('baseline\n')
            git(source, 'add', '.');git(source, 'commit', '-m', 'base')
            git(root, 'clone', '--bare', str(source), str(upstream))
            git(root, 'clone', '--bare', str(source), str(private))
            checkout = root/'blue'
            git(root, 'clone', str(private), str(checkout))
            git(checkout, 'config', 'user.name', 'Test');git(checkout, 'config', 'user.email', 'test@example.invalid')
            git(checkout, 'remote', 'add', 'upstream', str(upstream))
            release = {'tag_name':'v1','published_at':'2026-10-01','html_url':'https://example.invalid/v1'}
            state = {'upstreamCommit':git(source,'rev-parse','HEAD'),'latestRelease':release}
            (checkout/'blue').mkdir();(checkout/'blue/upstream-state.json').write_text(json.dumps(state))
            (checkout/'ours.txt').write_text('keep my skin\n')
            if conflict:(checkout/'code.txt').write_text('our local fix\n')
            git(checkout,'add','.');git(checkout,'commit','-m','Blue work');git(checkout,'push','origin','main')
            original = git(checkout,'rev-parse','HEAD')
            if change:
                (source/('code.txt' if conflict else 'upstream-fix.txt')).write_text('new upstream fix\n')
                git(source,'add','.');git(source,'commit','-m','upstream fix');git(source,'push',str(upstream),'main')
            module = importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(module)
            module.ROOT=checkout;module.STATE=checkout/'blue/upstream-state.json'
            calls=[];prs=[];real=module.run
            def fake_run(*args, **kwargs):
                if args[0]=='gh':
                    calls.append(args)
                    if args[1:3]==('pr','list'):
                        return subprocess.CompletedProcess(args,0,json.dumps(prs),'')
                    if args[1:3]==('pr','create'):prs.append({'number':1,'url':'https://example.invalid/pr/1'})
                    return subprocess.CompletedProcess(args,0,'','')
                return real(*args,**kwargs)
            module.run=fake_run
            latest=dict(release)
            if release_change:latest['tag_name']='v2'
            module.api=lambda endpoint: {'private':is_private,'default_branch':'main'} if endpoint=='repos/RobCZart82/Silver-K-Blue' else latest
            previous_repo = os.environ.get('GH_REPO')
            os.environ['GH_REPO']='RobCZart82/Silver-K-Blue'
            self.addCleanup(lambda: os.environ.pop('GH_REPO', None) if previous_repo is None else os.environ.__setitem__('GH_REPO', previous_repo))
            module.main()
            self.assertEqual(git(checkout,'rev-parse','main'),original)
            self.assertEqual(git(checkout,'show','main:ours.txt'),'keep my skin')
            if not change and not release_change:
                self.assertEqual(prs,[])
            else:
                self.assertEqual(prs,[])
                self.assertEqual(calls,[])
                branch=module.BRANCH
                self.assertEqual(git(checkout,'show',branch+':ours.txt'),'keep my skin')
                report=git(checkout,'show',branch+':blue/UPSTREAM-UPDATE.md')
                if conflict:
                    self.assertIn('Conflicting files',report)
                    self.assertEqual(git(checkout,'show',branch+':code.txt'),'our local fix')
                elif change:
                    self.assertEqual(git(checkout,'show',branch+':upstream-fix.txt'),'new upstream fix')
                git(checkout,'checkout','main')
                module.main()
                self.assertEqual(prs,[])
    def test_no_update(self):self.scenario(change=False)
    def test_clean_update_preserves_blue_and_main(self):self.scenario()
    def test_conflict_does_not_overwrite_blue(self):self.scenario(conflict=True)
    def test_release_only_update(self):self.scenario(change=False,release_change=True)
    def test_public_no_update(self):self.scenario(change=False,is_private=False)
    def test_public_clean_update(self):self.scenario(is_private=False)
    def test_public_conflict(self):self.scenario(conflict=True,is_private=False)
    def test_public_release_only(self):self.scenario(change=False,release_change=True,is_private=False)
    def test_wrong_repository_rejected_before_api_or_git(self):
        from unittest.mock import patch
        module = importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(module)
        with patch.dict(os.environ, {'GH_REPO':'someone/another-repository'}), patch.object(module,'api') as api, patch.object(module,'run') as run:
            with self.assertRaisesRegex(RuntimeError,'intended Blue repository'):
                module.main()
            api.assert_not_called()
            run.assert_not_called()

if __name__=='__main__':unittest.main()
