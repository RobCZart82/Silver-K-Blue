#!/usr/bin/env python3
"""Verify that source archives preserve the native Blue build inputs."""
from pathlib import Path
import tempfile
import unittest
from package_release import source_snapshot_files


class SourceSnapshotTests(unittest.TestCase):
    def test_runtime_blue_resources_are_included_and_design_masters_are_excluded(self):
        included = ['CMakeLists.txt', 'tools/embed_blue_skins.py',
                    'source/novation/silverK/plugin/CMakeLists.txt',
                    'blue/Artwork/Silver-K-Blue-icon.png',
                    'blue/Skins/Silver-K Blue Standard v1.0/skin.json',
                    'blue/Skins/Silver-K Blue Retina 4K v1.0/assets/knob.png']
        excluded = ['blue/Design/Masters/private.png', 'blue/Documentation/History/report.md',
                    'doc/OPTIMIZATION_LOG.md', 'private/firmware.bin']
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in included + excluded:
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'test')
            self.assertEqual(list(source_snapshot_files(root, included + excluded)), sorted(included))

    def test_original_firmware_is_still_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'assets').mkdir()
            (root / 'assets/firmware.bin').write_bytes(b'test')
            with self.assertRaises(RuntimeError):
                list(source_snapshot_files(root, ['assets/firmware.bin']))


if __name__ == '__main__':
    unittest.main()
