#!/usr/bin/env python3
"""Offline tests for macOS installer choices, validation and metadata."""
import hashlib
import json
from pathlib import Path
import plistlib
import tempfile
import unittest
from unittest.mock import patch
import xml.etree.ElementTree as ET

import package_blue_macos_installer as installer


class MacOSInstallerTests(unittest.TestCase):
    def test_distribution_exposes_three_system_components_and_shared_support(self):
        text = installer.installer_distribution("1.0.1")
        xml = ET.fromstring(text)
        self.assertEqual(xml.tag, "installer-gui-script")
        self.assertEqual(xml.findtext("title"), "Silver-K Blue 1.0.1")
        self.assertEqual(xml.find("os-version").get("min"), "12.0")
        self.assertEqual(xml.find("domains").get("enable_localSystem"), "true")
        choices = {choice.get("id"): choice for choice in xml.findall("choice")}
        self.assertEqual(set(choices), {"shared-support", "standalone", "vst3", "au"})
        self.assertEqual(choices["shared-support"].get("start_visible"), "false")
        self.assertEqual(choices["shared-support"].get("start_enabled"), "false")
        self.assertEqual(choices["shared-support"].find("pkg-ref").get("id"), installer.SUPPORT_ID)
        for choice_id in ("standalone", "vst3", "au"):
            self.assertEqual(choices[choice_id].get("start_selected"), "true")
            self.assertTrue(choices[choice_id].find("pkg-ref").get("id").startswith(
                "com.robczart.silverkblue.installer."))
        self.assertIsNone(xml.find("installation-check"))
        self.assertIsNone(xml.find("script"))
        packages = {ref.get("id"): ref.text for ref in xml.findall("pkg-ref")}
        self.assertEqual(packages[installer.SUPPORT_ID], "Silver-K-Blue-Shared-Support.pkg")
        self.assertEqual(len(packages), 4)

    def test_rejects_non_numeric_installer_versions(self):
        for version in ("1.0", "1.0.1-beta", "../1.0.1", ""):
            with self.subTest(version=version), self.assertRaises(ValueError):
                installer.installer_distribution(version)

    def test_refuses_to_replace_existing_installer(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp)
            existing = output / "Silver-K-Blue-1.0.1-macOS-Universal2.pkg"
            existing.write_bytes(b"keep")
            with patch.object(installer.subprocess, "check_output") as git:
                with self.assertRaisesRegex(FileExistsError, "Refusing to overwrite"):
                    installer.build_macos_installer(output / "missing-products", output, "1.0.1")
                git.assert_not_called()
            self.assertEqual(existing.read_bytes(), b"keep")

    def create_products(self, root):
        paths = []
        for _, _, _, _, category, bundle_name, _ in installer.COMPONENTS:
            bundle = root / category / bundle_name
            executable = bundle / "Contents/MacOS/silver-blue"
            executable.parent.mkdir(parents=True, exist_ok=True)
            executable.write_bytes(b"universal-test-binary")
            with (bundle / "Contents/Info.plist").open("wb") as stream:
                plistlib.dump({"CFBundleExecutable": "silver-blue"}, stream)
            paths.append(bundle)
        return paths

    def test_validate_rejects_missing_architecture_and_verifies_code(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle = self.create_products(Path(temp))[0]
            with patch.object(installer, "run", return_value="arm64"):
                with self.assertRaisesRegex(ValueError, "Universal 2"):
                    installer.validate_universal_bundle(bundle)
            with patch.object(installer, "run", side_effect=["arm64 x86_64", ""] ) as run:
                installer.validate_universal_bundle(bundle)
                self.assertEqual(run.call_count, 2)
                self.assertIn("--deep", run.call_args_list[1].args[0])

    def test_verifies_installer_evaluated_component_choices(self):
        items = [{
            "choiceIdentifier": "shared-support",
            "choiceIsEnabled": False,
            "choiceIsVisible": False,
            "choiceIsSelected": 1,
            "pathsOfActivePackagesInChoice": ["file:///tmp/pkg#Silver-K-Blue-Shared-Support.pkg"],
        }]
        items.extend({
            "choiceIdentifier": choice_id,
            "choiceIsEnabled": True,
            "choiceIsVisible": True,
            "choiceIsSelected": 1,
            "pathsOfActivePackagesInChoice": ["file:///tmp/pkg#" + package_name],
        } for choice_id, _, _, package_name, *_ in installer.COMPONENTS)
        payload = plistlib.dumps([{"childItems": items}]).decode("utf-8")
        with patch.object(installer, "run", return_value=payload) as run:
            installer.verify_installer_choices("Silver-K-Blue.pkg")
        self.assertEqual(run.call_args.args[0], [
            "installer", "-pkg", "Silver-K-Blue.pkg", "-target", "/", "-showChoicesXML"
        ])

    def test_rejects_broken_or_incomplete_installer_choice_tree(self):
        with self.assertRaisesRegex(ValueError, "unexpected choice tree"):
            installer.verify_installer_choices("broken.pkg", runner=lambda *_args, **_kwargs: "<plist/>")

        items = [{
            "choiceIdentifier": choice_id,
            "choiceIsEnabled": True,
            "choiceIsVisible": True,
            "choiceIsSelected": 1,
        } for choice_id, *_ in installer.COMPONENTS]
        payload = plistlib.dumps([{"childItems": items}]).decode("utf-8")
        with self.assertRaisesRegex(ValueError, "choices differ"):
            installer.verify_installer_choices("incomplete.pkg", runner=lambda *_args, **_kwargs: payload)

    def test_rejects_installer_that_does_not_default_select_components(self):
        items = [{
            "choiceIdentifier": "shared-support",
            "choiceIsEnabled": False,
            "choiceIsVisible": False,
            "choiceIsSelected": 1,
            "pathsOfActivePackagesInChoice": ["file:///tmp/pkg#Silver-K-Blue-Shared-Support.pkg"],
        }]
        items.extend({
            "choiceIdentifier": choice_id,
            "choiceIsEnabled": True,
            "choiceIsVisible": True,
            "choiceIsSelected": 0 if choice_id == "au" else 1,
            "pathsOfActivePackagesInChoice": ["file:///tmp/pkg#" + package_name],
        } for choice_id, _, _, package_name, *_ in installer.COMPONENTS)
        payload = plistlib.dumps([{"childItems": items}]).decode("utf-8")
        with self.assertRaisesRegex(ValueError, "au must be selected"):
            installer.verify_installer_choices("bad-default.pkg", runner=lambda *_args, **_kwargs: payload)

    def test_build_creates_pkg_and_provenance_sidecar(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            products = root / "Release"
            bundles = self.create_products(products)
            output = root / "out"
            pkgbuild_calls = []

            def fake_run(command, *, capture=False):
                if command[0] == "lipo":
                    return "arm64 x86_64"
                if command[0] == "codesign":
                    return ""
                if command[0] == "pkgbuild":
                    payload_root = Path(command[2])
                    package_id = command[command.index("--identifier") + 1]
                    if package_id == installer.SUPPORT_ID:
                        support = payload_root / installer.APP_SUPPORT
                        self.assertEqual(
                            {path.name for path in support.iterdir()},
                            {"LICENSE.md", "THIRD_PARTY.md", "README.md", "build-info.json"},
                        )
                        self.assertFalse((support / "firmware").exists())
                    else:
                        choice = next(item for item in installer.COMPONENTS if item[2] == package_id)
                        self.assertTrue((payload_root / choice[6] / choice[5] / "Contents/Info.plist").is_file())
                    destination = Path(command[-1])
                    destination.write_bytes(("package:" + command[2]).encode())
                    pkgbuild_calls.append((command, payload_root))
                    return ""
                raise AssertionError(command)

            def fake_subprocess_run(command, check):
                self.assertEqual(command[0], "productbuild")
                distribution = ET.parse(command[command.index("--distribution") + 1]).getroot()
                package_dir = Path(command[command.index("--package-path") + 1])
                self.assertEqual(len(distribution.findall("choice")), 4)
                for package in distribution.findall("pkg-ref"):
                    if package.text:
                        self.assertTrue((package_dir / package.text).is_file())
                destination = Path(command[-1])
                destination.write_bytes(b"product-archive")

            with patch.object(installer, "verify_installer_choices") as verify_choices, patch.object(
                installer, "run", side_effect=fake_run
            ), patch.object(
                installer.subprocess, "check_output", return_value="a" * 40
            ), patch.object(installer.subprocess, "run", side_effect=fake_subprocess_run):
                result = installer.build_macos_installer(products, output, "1.0.1")
            verify_choices.assert_called_once_with(result)

            self.assertEqual(result.name, "Silver-K-Blue-1.0.1-macOS-Universal2.pkg")
            self.assertEqual(len(pkgbuild_calls), 4)
            installed_roots = [root_path for _, root_path in pkgbuild_calls]
            self.assertEqual(len(installed_roots), 4)
            info_file = output / "Silver-K-Blue-macOS-Universal2-installer.json"
            record = json.loads(info_file.read_text(encoding="utf-8"))
            self.assertEqual(record["commit"], "a" * 40)
            self.assertEqual(record["version"], "1.0.1")
            self.assertEqual(record["platform"], "macOS-Universal2")
            self.assertEqual(record["sha256"], hashlib.sha256(result.read_bytes()).hexdigest())
            self.assertEqual(record["signature"], "unsigned")
            self.assertEqual(record["payloadSignature"], "ad-hoc")
            self.assertFalse(record["notarized"])
            self.assertFalse(record["firmwareIncluded"])


if __name__ == "__main__":
    unittest.main()
