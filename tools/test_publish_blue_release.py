import copy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
import zipfile
from unittest.mock import patch
import subprocess
import sys
import publish_blue_release as publication
from publish_blue_release import REPO, validate_runs, verify_package


class PublicationTests(unittest.TestCase):
    def installer_fixture(self, root, platform):
        root.mkdir(parents=True, exist_ok=True)
        info = dict(commit='a'*40, platform=platform, repository='https://github.com/'+REPO,
                    skinsEmbedded=True, firmwareIncluded=False)
        data = json.dumps(info).encode()
        with zipfile.ZipFile(root/'platform.zip', 'w') as bundle:
            bundle.writestr('root/build-info.json', data)
            bundle.writestr('root/SHA256SUMS.json', json.dumps({'build-info.json': hashlib.sha256(data).hexdigest()}))
        suffix = '.exe' if platform == 'Windows-x64' else '.pkg'
        binary = root/f'Silver-K-Blue-1.0.1-{platform}{suffix}'
        binary.write_bytes(b'installer regression fixture')
        record = dict(info, version='1.0.1', installer=binary.name, signature='unsigned',
                      notarized=False, sha256=hashlib.sha256(binary.read_bytes()).hexdigest())
        if platform == 'macOS-Universal2':
            record['payloadSignature'] = 'ad-hoc'
        metadata = root/f'Silver-K-Blue-{platform}-installer.json'
        metadata.write_text(json.dumps(record), encoding='utf-8')
        return binary, metadata, record

    def test_installer_assets_and_metadata_validation(self):
        for platform in ('Windows-x64', 'macOS-Universal2'):
            with self.subTest(platform=platform), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                binary, metadata, record = self.installer_fixture(root, platform)
                self.assertEqual(publication.platform_assets(root, 'a'*40, platform, '1.0.1'),
                                 [root/'platform.zip', binary, metadata])
                for field, value in (('commit', 'b'*40), ('version', '1.0.2'),
                                     ('platform', 'other'), ('repository', 'other'),
                                     ('installer', '../unexpected.exe'), ('signature', 'signed'),
                                     ('firmwareIncluded', True), ('notarized', True),
                                     ('sha256', '0'*64)):
                    with self.subTest(field=field):
                        metadata.write_text(json.dumps(dict(record, **{field: value})))
                        with self.assertRaises(ValueError):
                            publication.platform_assets(root, 'a'*40, platform, '1.0.1')
                metadata.write_text(json.dumps(record))
                binary.write_bytes(b'tampered installer')
                with self.assertRaisesRegex(ValueError, 'checksum'):
                    publication.platform_assets(root, 'a'*40, platform, '1.0.1')

    def test_installer_missing_empty_extra_or_wrong_version_is_rejected(self):
        for platform in ('Windows-x64', 'macOS-Universal2'):
            for fault in ('missing', 'empty', 'extra', 'version', 'sidecar', 'payload-signature'):
                with self.subTest(platform=platform, fault=fault), tempfile.TemporaryDirectory() as temp:
                    root = Path(temp)
                    binary, metadata, record = self.installer_fixture(root, platform)
                    if fault == 'missing': binary.unlink()
                    elif fault == 'empty': binary.write_bytes(b'')
                    elif fault == 'extra': (root/'other.pkg').write_bytes(b'extra')
                    elif fault == 'version': binary.rename(root/binary.name.replace('1.0.1', '1.0.2'))
                    elif fault == 'sidecar': metadata.unlink()
                    elif platform == 'macOS-Universal2':
                        record.pop('payloadSignature')
                        metadata.write_text(json.dumps(record))
                    else: continue
                    with self.assertRaises(ValueError):
                        publication.platform_assets(root, 'a'*40, platform, '1.0.1')

    def test_duplicate_publication_names_fail_before_github_access(self):
        with patch.object(publication, 'run') as run:
            with self.assertRaisesRegex(ValueError, 'Duplicate publication'):
                publication.publish_assets('blue-v1.0.1', 'a'*40, '1.0.1', Path('notes'),
                                           [Path('win/shared.json'), Path('mac/shared.json')])
            run.assert_not_called()

    def test_main_assembles_installers_sidecars_source_and_complete_checksums(self):
        records = [dict(id=1, run_number=1, html_url='https://example.test/windows'),
                   dict(id=2, run_number=2, html_url='https://example.test/macos')]
        def fake_run(*args):
            if args[:2] == ('gh', 'api'):
                return json.dumps(records[int(args[-1].split('/')[-1])-1])
            if args[:3] == ('gh', 'run', 'download'):
                root = Path(args[-1])
                platform = root.name
                self.installer_fixture(root, platform)
                (root/'platform.zip').rename(root/(platform+'.zip'))
                return ''
            self.fail(str(args))
        def archive(command, **kwargs):
            if command[:2] == ['git', 'archive']:
                target = Path(command[command.index('-o')+1])
                with zipfile.ZipFile(target, 'w') as bundle:
                    for name in ('CMakeLists.txt', 'tools/embed_blue_skins.py',
                                 'blue/Artwork/Silver-K-Blue-icon.png',
                                 'blue/Skins/Silver-K Blue Standard v1.0/skin.json',
                                 'blue/Skins/Silver-K Blue Retina 4K v1.0/skin.json'):
                        bundle.writestr('Silver-K-Blue-source/'+name, b'fixture')
        def assembled(tag, sha, version, notes, assets):
            self.assertEqual(len(assets), 8)
            names = {p.name for p in assets}
            self.assertEqual(len(names), 8)
            self.assertIn('Silver-K-Blue-1.0.1-Windows-x64.exe', names)
            self.assertIn('Silver-K-Blue-1.0.1-macOS-Universal2.pkg', names)
            self.assertIn('Silver-K-Blue-Windows-x64-installer.json', names)
            self.assertIn('Silver-K-Blue-macOS-Universal2-installer.json', names)
            sums = dict(line.split('  ', 1)[::-1] for line in assets[-1].read_text().splitlines())
            self.assertEqual(set(sums), names-{'SHA256SUMS.txt'})
            for asset in assets[:-1]:
                self.assertEqual(sums[asset.name], hashlib.sha256(asset.read_bytes()).hexdigest())
        for verify in (False, True):
            with self.subTest(verify_only=verify), tempfile.TemporaryDirectory() as temp:
                report = Path(temp)/'verification/report.json'
                argv = ['publish', '--version', '1.0.1', '--windows-run', '1', '--macos-run', '2']
                if verify:
                    argv += ['--verify-only', '--verification-report', str(report)]
                with patch.object(sys, 'argv', argv), \
                     patch.object(publication, 'run', side_effect=fake_run) as github, \
                     patch.object(publication, 'validate_runs', return_value='a'*40), \
                     patch.object(publication.subprocess, 'run', side_effect=archive), \
                     patch.object(publication, 'publish_assets', side_effect=assembled) as publish, \
                     patch('builtins.print'):
                    publication.main()
                    if verify:
                        publish.assert_not_called()
                        self.assertTrue(all(call.args[:2] == ('gh', 'api') or call.args[:3] == ('gh', 'run', 'download')
                                            for call in github.call_args_list))
                        result = json.loads(report.read_text())
                        self.assertIs(result['published'], False)
                        self.assertEqual(result['commit'], 'a'*40)
                        self.assertEqual((result['windows_run'], result['macos_run']), (1, 2))
                        self.assertEqual(len(result['assets']), 8)
                        self.assertEqual(len({x['name'] for x in result['assets']}), 8)
                        self.assertTrue(all(x['size'] > 0 and len(x['sha256']) == 64 for x in result['assets']))
                    else:
                        publish.assert_called_once()

    def test_report_arguments_fail_before_github_access(self):
        with tempfile.TemporaryDirectory() as temp:
            report = Path(temp)/'existing.json'
            report.write_text('preserved evidence')
            for extra, error in ((['--verification-report', str(report)], ValueError),
                                 (['--verify-only', '--verification-report', str(report)], FileExistsError)):
                with self.subTest(extra=extra), patch.object(sys, 'argv',
                        ['publish', '--version', '1.0.1', '--windows-run', '1', '--macos-run', '2'] + extra), \
                     patch.object(publication, 'run') as github, \
                     patch.object(publication, 'publish_assets') as publish:
                    with self.assertRaises(error):
                        publication.main()
                    github.assert_not_called()
                    publish.assert_not_called()
                    self.assertEqual(report.read_text(), 'preserved evidence')

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
                    with patch.object(publication, 'validate_release_tag'), patch.object(publication, 'run', side_effect=[
                            json.dumps([[original]]), json.dumps(changed)]) as run:
                        with self.assertRaisesRegex(ValueError, 'changed during asset upload'):
                            publication.publish_assets('blue-v1.0.0', 'a'*40,
                                                       '1.0.0', notes, [])
                        self.assertEqual(run.call_count, 2)

    def test_remote_lightweight_and_annotated_tags(self):
        tag = 'blue-v1.0.1'
        ref = 'refs/tags/' + tag
        for refs in ('', 'a'*40+'\t'+ref,
                     'b'*40+'\t'+ref+'\n'+'a'*40+'\t'+ref+'^{}'):
            with self.subTest(refs=refs), patch.object(publication, 'run', return_value=refs):
                publication.validate_release_tag(tag, 'a'*40)
        for refs in ('b'*40+'\t'+ref,
                     'a'*40+'\t'+ref+'\n'+'b'*40+'\t'+ref+'^{}'):
            with self.subTest(refs=refs), patch.object(publication, 'run', return_value=refs):
                with self.assertRaisesRegex(ValueError, 'Git tag'):
                    publication.validate_release_tag(tag, 'a'*40)
        with patch.object(publication, 'run', side_effect=subprocess.CalledProcessError(1, 'git')):
            with self.assertRaises(subprocess.CalledProcessError):
                publication.validate_release_tag(tag, 'a'*40)

    def test_wrong_tag_stops_before_github_mutation(self):
        with patch.object(publication, 'validate_release_tag', side_effect=ValueError('wrong tag')), patch.object(publication, 'run') as run:
            with self.assertRaises(ValueError):
                publication.publish_assets('blue-v1.0.1', 'a'*40, '1.0.1', Path('unused'), [])
            run.assert_not_called()

    def test_tag_change_before_publication_stops_release_edit(self):
        with tempfile.TemporaryDirectory() as temp:
            notes = Path(temp)/'notes.md'
            notes.write_text('Reviewed notes', encoding='utf-8')
            draft = dict(id=123, tag_name='blue-v1.0.1', draft=True,
                         target_commitish='a'*40, name='Silver-K Blue 1.0.1',
                         body='Reviewed notes', assets=[])
            with patch.object(publication, 'validate_release_tag',
                              side_effect=[None, ValueError('tag changed')]) as validate, patch.object(
                                  publication, 'run', side_effect=[json.dumps([[draft]]), json.dumps(draft)]) as run:
                with self.assertRaisesRegex(ValueError, 'tag changed'):
                    publication.publish_assets('blue-v1.0.1', 'a'*40, '1.0.1', notes, [])
                self.assertEqual(validate.call_count, 2)
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
            with patch.object(publication, 'validate_release_tag'), patch.object(publication, 'run', side_effect=fake_run):
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
