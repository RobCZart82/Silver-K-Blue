#!/usr/bin/env python3
"""Offline checks for the generated Windows installer definition."""
import struct
import tempfile
import unittest
from pathlib import Path

from package_blue_installer import windows_script


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
