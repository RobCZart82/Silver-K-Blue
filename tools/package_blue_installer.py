#!/usr/bin/env python3
"""Build the Windows x64 Silver-K Blue installer from validated CI products."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
APP_ID = "{E214A797-830C-5F86-9384-442F17D88F14}"


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def iss_string(value):
    return '"' + str(value).replace('"', '""') + '"'


def check_x64(path):
    with Path(path).open("rb") as stream:
        if stream.read(2) != b"MZ":
            raise ValueError(f"Not a Windows executable: {path}")
        stream.seek(0x3C)
        offset = struct.unpack("<I", stream.read(4))[0]
        stream.seek(offset)
        if stream.read(4) != b"PE\0\0" or struct.unpack("<H", stream.read(2))[0] != 0x8664:
            raise ValueError(f"Not a Windows x64 executable: {path}")


def windows_script(products, output, version):
    """Return an Inno Setup definition with independently selectable payloads."""
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError("Installer version must be numeric major.minor.patch")
    product_root = Path(products).resolve()
    output_dir = Path(output).resolve()
    standalone = product_root / "Standalone" / "Silver-K Blue.exe"
    vst3 = product_root / "VST3" / "Silver-K Blue.vst3"
    if not standalone.is_file():
        raise FileNotFoundError(f"Missing Standalone executable: {standalone}")
    if not vst3.is_dir():
        raise FileNotFoundError(f"Missing VST3 bundle: {vst3}")
    vst3_binaries = list(vst3.glob("Contents/x86_64-win/*.vst3"))
    if len(vst3_binaries) != 1:
        raise ValueError("Expected exactly one Windows x64 VST3 binary")
    check_x64(standalone)
    check_x64(vst3_binaries[0])

    version_info = version + ".0"
    text = f'''#define ProductVersion {iss_string(version)}
#define ProductRoot {iss_string(product_root)}
#define InstallerOutput {iss_string(output_dir)}

[Setup]
AppId=__APP_ID__
AppName=Silver-K Blue
AppVersion={{#ProductVersion}}
AppVerName=Silver-K Blue {{#ProductVersion}}
AppPublisher=RobCZart82
AppPublisherURL=https://github.com/RobCZart82/Silver-K-Blue
DefaultDirName={{autopf}}\\Silver-K Blue
DefaultGroupName=Silver-K Blue
OutputDir={{#InstallerOutput}}
OutputBaseFilename=Silver-K-Blue-{{#ProductVersion}}-Windows-x64
VersionInfoVersion={version_info}
VersionInfoProductVersion={{#ProductVersion}}
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
WizardStyle=modern
Compression=lzma2/ultra64
SolidCompression=yes
Uninstallable=yes
DisableProgramGroupPage=yes
CloseApplications=yes
RestartApplications=no
ChangesAssociations=no

[Types]
Name: "full"; Description: "VST3 and Standalone"
Name: "custom"; Description: "Choose components"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in"; Types: full custom
Name: "standalone"; Description: "Standalone application"; Types: full custom

[InstallDelete]
Type: filesandordirs; Name: "{{commoncf64}}\\VST3\\Silver-K Blue.vst3"; Check: IsPreviousInstall
Type: filesandordirs; Name: "{{autopf}}\\Silver-K Blue"; Check: IsPreviousInstall

[Files]
Source: "{{#ProductRoot}}\\VST3\\Silver-K Blue.vst3\\*"; DestDir: "{{commoncf64}}\\VST3\\Silver-K Blue.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{{#ProductRoot}}\\Standalone\\Silver-K Blue.exe"; DestDir: "{{autopf}}\\Silver-K Blue"; Components: standalone; Flags: ignoreversion
Source: {iss_string(ROOT / 'LICENSE.md')}; DestDir: "{{commonappdata}}\\The Usual Suspects\\Silver-K Blue"; Flags: ignoreversion
Source: {iss_string(ROOT / 'THIRD_PARTY.md')}; DestDir: "{{commonappdata}}\\The Usual Suspects\\Silver-K Blue"; Flags: ignoreversion
Source: "{{#InstallerOutput}}\\build-info.json"; DestDir: "{{commonappdata}}\\The Usual Suspects\\Silver-K Blue"; Flags: ignoreversion

[Icons]
Name: "{{group}}\\Silver-K Blue"; Filename: "{{autopf}}\\Silver-K Blue\\Silver-K Blue.exe"; Components: standalone

[Run]
Filename: "{{autopf}}\\Silver-K Blue\\Silver-K Blue.exe"; Description: "Launch Silver-K Blue"; Components: standalone; Flags: postinstall nowait skipifsilent

[Code]
function IsPreviousInstall: Boolean;
begin
  Result := RegKeyExists(HKLM64, 'Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{{E214A797-830C-5F86-9384-442F17D88F14}}_is1');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = wpSelectComponents then
    Result := IsComponentSelected('vst3') or IsComponentSelected('standalone');
end;
'''
    return text.replace("__APP_ID__", "{" + APP_ID)


def find_iscc():
    for candidate in (
        shutil.which("ISCC.exe"),
        Path(r"C:\Program Files (x86)\Inno Setup 6\ISCC.exe"),
        Path(r"C:\Program Files\Inno Setup 6\ISCC.exe"),
    ):
        if candidate and Path(candidate).is_file():
            return str(candidate)
    raise FileNotFoundError("Inno Setup 6 ISCC.exe is required to build the Windows installer")


def build_windows_installer(products, output, version, compiler=None):
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    script = windows_script(products, output, version)
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    build_info = {
        "repository": "https://github.com/RobCZart82/Silver-K-Blue",
        "commit": revision,
        "version": version,
        "platform": "Windows-x64",
        "products": ["VST3", "Standalone"],
        "skinsEmbedded": True,
        "firmwareIncluded": False,
    }
    (output / "build-info.json").write_text(json.dumps(build_info, indent=2) + "\n", encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="silver-blue-innosetup-") as temp:
        iss = Path(temp) / "Silver-K-Blue.iss"
        iss.write_text(script, encoding="utf-8-sig")
        subprocess.run([compiler or find_iscc(), "/Qp", str(iss)], check=True)
    installer = output / f"Silver-K-Blue-{version}-Windows-x64.exe"
    if not installer.is_file() or installer.stat().st_size == 0:
        raise RuntimeError("Inno Setup did not create the expected installer")
    record = {
        "repository": "https://github.com/RobCZart82/Silver-K-Blue",
        "commit": revision,
        "version": version,
        "platform": "Windows-x64",
        "installer": installer.name,
        "sha256": digest(installer),
        "signature": "unsigned",
        "notarized": False,
        "firmwareIncluded": False,
    }
    (output / "Silver-K-Blue-Windows-x64-installer.json").write_text(
        json.dumps(record, indent=2) + "\n", encoding="utf-8"
    )
    return installer


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--products", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--version", required=True)
    args = parser.parse_args()
    print("Created:", build_windows_installer(args.products, args.output, args.version))


if __name__ == "__main__":
    main()
