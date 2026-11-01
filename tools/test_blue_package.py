#!/usr/bin/env python3
"""Exercise the real Windows ZIP path with synthetic PE files, without a build."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

SOURCE = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('package_blue', SOURCE / 'tools/package_blue_windows.py')

class BluePackageTests(unittest.TestCase):
    def package(self, missing_guide=False):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root/'blue/Skins/Test').mkdir(parents=True)
            (root/'blue/Skins/Test/skin.json').write_text('{}')
            (root/'doc').mkdir()
            for name in ['BLUE_PACKAGE_README.md', 'INSTALL.md', 'USER_GUIDE.md', 'SKINS.md']:
                shutil.copy2(SOURCE/'doc'/name, root/'doc'/name)
            for name in ['LICENSE.md', 'THIRD_PARTY.md']:
                (root/name).write_text('synthetic test notice')
            if missing_guide:
                (root/'doc/USER_GUIDE.md').unlink()
            products = root/'products'
            for relative in ['Standalone/Silver-K Blue.exe','VST3/Silver-K Blue.vst3/Contents/x86_64-win/Silver-K Blue.vst3']:
                target = products/relative
                target.parent.mkdir(parents=True, exist_ok=True)
                data = bytearray(128)
                data[:2] = b'MZ'
                struct.pack_into('<I',data,0x3c,64)
                data[64:68] = b'PE\0\0'
                struct.pack_into('<H',data,68,0x8664)
                target.write_bytes(data)
            module = importlib.util.module_from_spec(SPEC)
            SPEC.loader.exec_module(module)
            module.ROOT = root
            argv = ['package_blue_windows.py','--products',str(products),'--output',str(root/'out')]
            with patch.object(sys,'argv',argv), patch.object(module.subprocess,'check_output',return_value='a'*40+'\n'):
                if missing_guide:
                    with self.assertRaises(FileNotFoundError):
                        module.main()
                    self.assertEqual(list((root/'out').glob('*.zip')),[])
                    return
                module.main()
            archive = next((root/'out').glob('*.zip'))
            with zipfile.ZipFile(archive) as bundle:
                base = archive.stem+'/'
                self.assertIsNone(bundle.testzip())
                readme = bundle.read(base+'README.md').decode()
                self.assertIn('separate plugin identity',readme)
                self.assertIn('your own compatible firmware',readme)
                for name in ['INSTALL.md','USER_GUIDE.md','SKINS.md','README-HU.md']:
                    self.assertIn(base+name,bundle.namelist())
                info = json.loads(bundle.read(base+'build-info.json'))
                self.assertEqual(info['commit'],'a'*40)
                self.assertFalse(info['firmwareIncluded'])
                sums = json.loads(bundle.read(base+'SHA256SUMS.json'))
                for name, digest in sums.items():
                    self.assertEqual(hashlib.sha256(bundle.read(base+name)).hexdigest(),digest)
            external = archive.with_suffix('.zip.sha256').read_text().split()[0]
            self.assertEqual(external,hashlib.sha256(archive.read_bytes()).hexdigest())

    def test_english_guides_and_hashes_in_real_zip(self):
        self.package()

    def test_missing_required_guide_does_not_produce_zip(self):
        self.package(missing_guide=True)

if __name__ == '__main__':
    unittest.main()
