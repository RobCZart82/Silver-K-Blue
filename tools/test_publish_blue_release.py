import copy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
import zipfile
from publish_blue_release import REPO, validate_runs, verify_package


class PublicationTests(unittest.TestCase):
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
