#!/usr/bin/env python3
"""Build an unsigned macOS Universal 2 component installer from CI products."""
import argparse
import hashlib
import json
import plistlib
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
APP_SUPPORT = Path("Library/Application Support/The Usual Suspects/Silver-K Blue")
COMPONENTS = (
    ("standalone", "Standalone application", "com.robczart.silverkblue.installer.standalone",
     "Silver-K-Blue-Standalone.pkg", "Standalone", "Silver-K Blue.app", "Applications"),
    ("vst3", "VST3 plug-in", "com.robczart.silverkblue.installer.vst3",
     "Silver-K-Blue-VST3.pkg", "VST3", "Silver-K Blue.vst3", "Library/Audio/Plug-Ins/VST3"),
    ("au", "Audio Unit plug-in", "com.robczart.silverkblue.installer.au",
     "Silver-K-Blue-AU.pkg", "AU", "Silver-K Blue.component", "Library/Audio/Plug-Ins/Components"),
)
SUPPORT_ID = "com.robczart.silverkblue.installer.support"


def digest(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def bundle_executable(bundle):
    info_path = Path(bundle) / "Contents/Info.plist"
    with info_path.open("rb") as stream:
        info = plistlib.load(stream)
    executable = info.get("CFBundleExecutable")
    if not isinstance(executable, str) or not executable or Path(executable).name != executable:
        raise ValueError(f"Invalid CFBundleExecutable in {info_path}")
    binary = Path(bundle) / "Contents/MacOS" / executable
    if not binary.is_file():
        raise FileNotFoundError(f"Missing bundle executable: {binary}")
    return binary


def run(command, *, capture=False):
    if capture:
        return subprocess.check_output(command, text=True).strip()
    subprocess.run(command, check=True)
    return ""


def validate_universal_bundle(bundle, runner=None):
    runner = runner or run
    binary = bundle_executable(bundle)
    arches = set(runner(["lipo", "-archs", str(binary)], capture=True).split())
    if not {"arm64", "x86_64"}.issubset(arches):
        raise ValueError(f"Universal 2 binary missing from {binary}: {sorted(arches)}")
    runner(["codesign", "--verify", "--deep", "--strict", str(bundle)])


def installer_distribution(version):
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError("Installer version must be numeric major.minor.patch")
    refs = "\n".join(
        f'  <pkg-ref id="{package_id}">{package_file}</pkg-ref>'
        for _, _, package_id, package_file, _, _, _ in COMPONENTS
    )
    choices = "\n".join(
        f'''  <choice id="{choice_id}" title="{title}" description="Install {title}." start_selected="true">
    <pkg-ref id="{package_id}"/>
  </choice>'''
        for choice_id, title, package_id, _, _, _, _ in COMPONENTS
    )
    outline = "\n".join(f'    <line choice="{choice_id}"/>' for choice_id, *_ in COMPONENTS)
    return f'''<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Silver-K Blue {version}</title>
  <options customize="always" require-scripts="false" rootVolumeOnly="true"/>
  <os-version min="12.0"/>
  <domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
  <choices-outline>
    <line choice="shared-support"/>
{outline}
  </choices-outline>
  <choice id="shared-support" title="Shared license and build information" start_selected="true" start_enabled="false" start_visible="false">
    <pkg-ref id="{SUPPORT_ID}"/>
  </choice>
{choices}
{refs}
  <pkg-ref id="{SUPPORT_ID}">Silver-K-Blue-Shared-Support.pkg</pkg-ref>
</installer-gui-script>
'''


def verify_installer_choices(installer, runner=None):
    """Ask macOS Installer to evaluate the package's actual component choices."""
    runner = runner or run
    output = runner(["installer", "-pkg", str(installer), "-target", "/",
                     "-showChoicesXML"], capture=True)
    choices = plistlib.loads(output.encode("utf-8"))
    if not isinstance(choices, list) or len(choices) != 1:
        raise ValueError("Installer returned an unexpected choice tree")
    items = choices[0].get("childItems", [])
    by_id = {item.get("choiceIdentifier"): item for item in items}
    expected = {choice_id for choice_id, *_ in COMPONENTS} | {"shared-support"}
    if set(by_id) != expected:
        raise ValueError(f"Installer choices differ from expected components: {sorted(by_id)}")
    support = by_id["shared-support"]
    if support.get("choiceIsEnabled") is not False or support.get("choiceIsVisible") is not False:
        raise ValueError("Shared support must remain hidden and mandatory")
    if not any(path.endswith("#Silver-K-Blue-Shared-Support.pkg")
               for path in support.get("pathsOfActivePackagesInChoice", [])):
        raise ValueError("Shared support choice is not bound to its component package")
    for choice_id, _, _, package_name, *_ in COMPONENTS:
        choice = by_id[choice_id]
        if choice.get("choiceIsVisible") is not True or choice.get("choiceIsEnabled") is not True:
            raise ValueError(f"Installer component {choice_id} must be selectable")
        if choice.get("choiceIsSelected") != 1:
            raise ValueError(f"Installer component {choice_id} must be selected by default")
        if not any(path.endswith("#" + package_name)
                   for path in choice.get("pathsOfActivePackagesInChoice", [])):
            raise ValueError(f"Installer component {choice_id} is not bound to {package_name}")


def stage_component(source, root, install_base):
    target = Path(root) / install_base / source.name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source, target, symlinks=True)


def make_payload(products, work, version, revision):
    products = Path(products).resolve()
    packages = Path(work) / "packages"
    packages.mkdir(parents=True)
    for choice_id, _, package_id, package_name, product_category, bundle_name, install_directory in COMPONENTS:
        bundle = products / product_category / bundle_name
        if not bundle.is_dir():
            raise FileNotFoundError(f"Missing {choice_id} bundle: {bundle}")
        validate_universal_bundle(bundle)
        staged = Path(work) / (choice_id + "-payload")
        stage_component(bundle, staged, install_directory)
        package_path = packages / package_name
        run(["pkgbuild", "--root", str(staged), "--identifier", package_id,
             "--version", version, "--install-location", "/", str(package_path)])

    shared = Path(work) / "shared-payload" / APP_SUPPORT
    shared.mkdir(parents=True)
    for source, name in ((ROOT / "LICENSE.md", "LICENSE.md"),
                         (ROOT / "THIRD_PARTY.md", "THIRD_PARTY.md"),
                         (ROOT / "doc/BLUE_PACKAGE_README.md", "README.md")):
        shutil.copy2(source, shared / name)
    info = {
        "repository": "https://github.com/RobCZart82/Silver-K-Blue",
        "commit": revision,
        "version": version,
        "platform": "macOS-Universal2",
        "products": ["AU", "VST3", "Standalone"],
        "skinsEmbedded": True,
        "firmwareIncluded": False,
        "payloadSignature": "ad-hoc",
        "installerSignature": "unsigned",
        "notarized": False,
    }
    (shared / "build-info.json").write_text(json.dumps(info, indent=2) + "\n", encoding="utf-8")
    support_package = packages / "Silver-K-Blue-Shared-Support.pkg"
    run(["pkgbuild", "--root", str(Path(work) / "shared-payload"), "--identifier",
         SUPPORT_ID, "--version", version, "--install-location", "/", str(support_package)])
    return packages


def build_macos_installer(products, output, version):
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError("Installer version must be numeric major.minor.patch")
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    installer = output / f"Silver-K-Blue-{version}-macOS-Universal2.pkg"
    metadata = output / "Silver-K-Blue-macOS-Universal2-installer.json"
    if installer.exists() or metadata.exists():
        raise FileExistsError("Refusing to overwrite an existing macOS installer or metadata file")
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    with tempfile.TemporaryDirectory(prefix="silver-blue-pkg-") as temp:
        work = Path(temp)
        packages = make_payload(products, work, version, revision)
        distribution = work / "Distribution.xml"
        distribution.write_text(installer_distribution(version), encoding="utf-8")
        subprocess.run(["productbuild", "--distribution", str(distribution),
                        "--package-path", str(packages), str(installer)], check=True)
    if not installer.is_file() or installer.stat().st_size == 0:
        raise RuntimeError("productbuild did not create the expected macOS installer")
    verify_installer_choices(installer)
    metadata.write_text(
        json.dumps({
            "repository": "https://github.com/RobCZart82/Silver-K-Blue",
            "commit": revision,
            "version": version,
            "platform": "macOS-Universal2",
            "installer": installer.name,
            "sha256": digest(installer),
            "signature": "unsigned",
            "payloadSignature": "ad-hoc",
            "notarized": False,
            "firmwareIncluded": False,
        }, indent=2) + "\n", encoding="utf-8")
    return installer


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--products", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--version", required=True)
    args = parser.parse_args()
    print("Created:", build_macos_installer(args.products, args.output, args.version))


if __name__ == "__main__":
    main()
