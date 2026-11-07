import copy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
import zipfile
from unittest.mock import patch
import subprocess
import publish_blue_release as publication
from publish_blue_release import REPO, validate_runs, verify_package


class PublicationTests(unittest.TestCase):
    def test_reject_draft_changed_during_upload(self):
        with tempfile.TemporaryDirectory() as temp:
            notes = Path(temp) / 'notes.md'
            notes.write_text('Reviewed notes', encoding='utf-8')
            original = dict(id=123, tag_name='blue-v1.0.0', draft=True,
                            target_commitish='a'*40, name='Silver-K Blue 1.0.0',
                            body='Reviewed notes', assets=[])
            for key, value in (('draft', False), ('tag_name', 'blue-v2.0.0'),
                               ('target_commitish', 'b'*40), ('name', 'Changed'),
                               ('body', 'Unreviewed notes')):
                with self.subTest(key=key):
                    changed = dict(original, **{key: value})
                    with patch.object(publication, 'run', side_effect=[
                            json.dumps([[original]]), json.dumps(changed)]) as run:
                        with self.assertRaisesRegex(ValueError, 'changed during asset upload'):
                            publication.publish_assets('blue-v1.0.0', 'a'*40,
                                                       '1.0.0', notes, [])
                        self.assertEqual(run.call_count, 2)

    def records(self):
        return [dict(status='completed', conclusion='success', head_branch='main',
                     event='push', head_sha='a'*40, repository={'full_name': REPO},
                     path='.github/workflows/'+name+'-blue.yml') for name in ('windows', 'macos')]

    def test_only_successful_same_source_main_builds(self):
        records = self.records()
        self.assertEqual(validate_runs(*records), 'a'*40)
        for key, value in [('conclusion', 'failure'), ('head_branch', 'other'),
                           ('head_sha', 'b'*40), ('event', 'pull_request'), ('path', 'other.yml')]:
            bad = copy.deepcopy(records);bad[1][key] = value
            with self.assertRaises(ValueError): validate_runs(*bad)

    def test_resume_interrupted_upload_and_reject_foreign_drafts(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            notes = root/'notes.md'
            notes.write_text('Reviewed notes')
            assets = [root/'windows.zip', root/'macos.zip']
            for asset in assets:
                asset.write_bytes(asset.name.encode())
            release = None
            fail_once = True
            uploaded_names = []
            def fake_run(*args):
                nonlocal release, fail_once
                if args[:3] == ('gh', 'api', '--paginate'):
                    return json.dumps([[release] if release else []])
                if args[:3] == ('gh', 'release', 'create'):
                    release = dict(id=123, tag_name='blue-v1.0.0', draft=True,
                                   target_commitish='a'*40, name='Silver-K Blue 1.0.0',
                                   body=notes.read_text(), assets=[])
                elif args[:3] == ('gh', 'release', 'upload'):
                    asset = Path(args[-1])
                    if asset.name == 'macos.zip' and fail_once:
                        fail_once = False
                        raise subprocess.CalledProcessError(1, args)
                    uploaded_names.append(asset.name)
                    release['assets'].append(dict(name=asset.name, state='uploaded',
                        size=asset.stat().st_size,
                        digest='sha256:'+hashlib.sha256(asset.read_bytes()).hexdigest()))
                elif args[:3] == ('gh', 'release', 'edit'):
                    release['draft'] = False
                elif args[:4] == ('gh', 'api', '--method', 'DELETE'):
                    release['assets'] = [asset for asset in release['assets']
                                         if str(asset.get('id')) != args[-1].split('/')[-1]]
                elif args[:2] == ('gh', 'api'):
                    return json.dumps(release)
                else:
                    self.fail(str(args))
                return ''
            with patch.object(publication, 'run', side_effect=fake_run):
                with self.assertRaises(subprocess.CalledProcessError):
                    publication.publish_assets('blue-v1.0.0', 'a'*40, '1.0.0', notes, assets)
                release['assets'].append(dict(id=99, name='macos.zip', state='starter', size=0))
                publication.publish_assets('blue-v1.0.0', 'a'*40, '1.0.0', notes, assets)
                self.assertEqual(uploaded_names, ['windows.zip', 'macos.zip'])
                self.assertFalse(release['draft'])
                with self.assertRaises(ValueError):
                    publication.publish_assets('blue-v1.0.0', 'a'*40, '1.0.0', notes, assets)
                release['draft'] = True
                release['assets'][0]['digest'] = 'sha256:incorrect'
                with self.assertRaises(ValueError):
                    publication.publish_assets('blue-v1.0.0', 'a'*40, '1.0.0', notes, assets)
                release['target_commitish'] = 'b'*40
                with self.assertRaises(ValueError):
                    publication.publish_assets('blue-v1.0.0', 'a'*40, '1.0.0', notes, assets)

    def test_package_provenance_and_every_file_hash(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/'package.zip'
            info = json.dumps(dict(commit='a'*40, platform='Windows-x64',
                repository='https://github.com/'+REPO, skinsEmbedded=True, firmwareIncluded=False)).encode()
            files = {'build-info.json': info, 'Plugins/test': b'product'}
            def create(tamper=False):
                sums = {k: hashlib.sha256(v).hexdigest() for k,v in files.items()}
                with zipfile.ZipFile(path, 'w') as z:
                    for k,v in files.items(): z.writestr('root/'+k, b'changed' if tamper and k=='Plugins/test' else v)
                    z.writestr('root/SHA256SUMS.json', json.dumps(sums))
            create();verify_package(path, 'a'*40, 'Windows-x64')
            with self.assertRaises(ValueError): verify_package(path, 'b'*40, 'Windows-x64')
            create(True)
            with self.assertRaises(ValueError): verify_package(path, 'a'*40, 'Windows-x64')


if __name__ == '__main__': unittest.main()
