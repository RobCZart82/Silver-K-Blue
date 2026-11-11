#!/usr/bin/env python3
"""Offline checks for the generated Windows installer definition."""
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from package_blue_installer import windows_script, build_windows_installer


def fake_pe(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    data = bytearray(128)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 64)
    data[64:68] = b"PE\0\0"
    struct.pack_into("<H", data, 68, 0x8664)
    path.write_bytes(data)


class WindowsInstallerDefinitionTests(unittest.TestCase):
    def products(self, root):
        standalone = root / "Release" / "Standalone" / "Silver-K Blue.exe"
        vst3 = root / "Release" / "VST3" / "Silver-K Blue.vst3" / "Contents" / "x86_64-win" / "Silver-K Blue.vst3"
        fake_pe(standalone)
        fake_pe(vst3)
        return root / "Release"

    def test_optional_components_keep_blue_identity_and_user_data_separate(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            text = windows_script(self.products(root), root / "output", "1.0.1")
            self.assertIn("AppId={{E214A797-830C-5F86-9384-442F17D88F14}", text)
            self.assertIn('Name: "vst3"; Description: "VST3 plug-in"', text)
            self.assertIn('Name: "standalone"; Description: "Standalone application"', text)
            self.assertIn("IsComponentSelected('vst3') or IsComponentSelected('standalone')", text)
            self.assertIn("RegKeyExists(HKLM64", text)
            self.assertIn("{E214A797-830C-5F86-9384-442F17D88F14}_is1", text)
            self.assertIn("[InstallDelete]", text)
            self.assertIn("Check: IsPreviousInstall", text)
            self.assertIn(r"{commoncf64}\VST3\Silver-K Blue.vst3", text)
            self.assertIn(r"{commonappdata}\The Usual Suspects\Silver-K Blue", text)
            self.assertNotIn("{userappdata}", text)
            self.assertIn("PrivilegesRequired=admin", text)

    def test_rejects_invalid_release_version(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            products = self.products(root)
            with self.assertRaises(ValueError):
                windows_script(products, root / "output", "1.0.1-beta")

    def test_standalone_uses_chosen_directory_for_payload_shortcut_and_launch(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            text = windows_script(self.products(root), root / "output", "1.0.1")
            self.assertIn('DestDir: "{app}"; Components: standalone', text)
            self.assertEqual(text.count('Filename: "{app}\\Silver-K Blue.exe"'), 2)
            # The default may name Program Files; payload/launch must honor {app}.
            self.assertEqual(text.count(r"{autopf}\Silver-K Blue"), 1)

    def test_upgrade_cleanup_preserves_app_folder_and_unselected_vst3(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            text = windows_script(self.products(root), root / "output", "1.0.1")
            cleanup = text.split('[InstallDelete]\n', 1)[1].split('\n[Files]', 1)[0]
            entries = [line for line in cleanup.splitlines() if line.startswith('Type:')]
            self.assertEqual(len(entries), 1)
            self.assertIn(r'Name: "{commoncf64}\VST3\Silver-K Blue.vst3"', entries[0])
            self.assertIn('Components: vst3;', entries[0])
            self.assertNotIn('{app}', cleanup)
            self.assertNotIn('{autopf}', cleanup)

    def test_builder_refuses_to_replace_existing_installer_evidence(self):
        for filename in ('Silver-K-Blue-1.0.1-Windows-x64.exe',
                         'Silver-K-Blue-Windows-x64-installer.json'):
            with self.subTest(filename=filename), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                products = self.products(root)
                output = root / 'output'
                output.mkdir()
                existing = output / filename
                existing.write_bytes(b'existing evidence')
                with patch('package_blue_installer.subprocess.run') as compile_installer, \
                     patch('package_blue_installer.subprocess.check_output') as revision:
                    with self.assertRaises(FileExistsError):
                        build_windows_installer(products, output, '1.0.1')
                    compile_installer.assert_not_called()
                    revision.assert_not_called()
                self.assertEqual(existing.read_bytes(), b'existing evidence')
                self.assertFalse((output / 'build-info.json').exists())

    def test_rejects_non_x64_product_payload(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            products = self.products(root)
            standalone = products / "Standalone" / "Silver-K Blue.exe"
            data = bytearray(128)
            data[:2] = b"MZ"
            struct.pack_into("<I", data, 0x3C, 64)
            data[64:68] = b"PE\0\0"
            struct.pack_into("<H", data, 68, 0x014C)
            standalone.write_bytes(data)
            with self.assertRaisesRegex(ValueError, "x64"):
                windows_script(products, root / "output", "1.0.1")


if __name__ == "__main__":
    unittest.main()
