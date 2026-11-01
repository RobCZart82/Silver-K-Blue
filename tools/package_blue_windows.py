#!/usr/bin/env python3
"""Package Windows or Universal 2 builds with embedded Blue skins and provenance."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import zipfile
ROOT = Path(__file__).resolve().parents[1]

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

def check_x64(path):
    with path.open('rb') as stream:
        if stream.read(2) != b'MZ':
            raise ValueError(f'Not a Windows executable: {path}')
        stream.seek(0x3c)
        offset = struct.unpack('<I', stream.read(4))[0]
        stream.seek(offset)
        if stream.read(4) != b'PE\0\0' or struct.unpack('<H', stream.read(2))[0] != 0x8664:
            raise ValueError(f'Not Windows x64 PE: {path}')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--products', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--platform', choices=['Windows-x64','macOS-Universal2'], default='Windows-x64')
    args = parser.parse_args()
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    macos = args.platform == 'macOS-Universal2'
    products = [('Standalone','Silver-K Blue.app' if macos else 'Silver-K Blue.exe','Standalone'),
                ('VST3','Silver-K Blue.vst3','Plugins/VST3')]
    if macos:
        products.append(('AU','Silver-K Blue.component','Plugins/AU'))
    for category, filename, destination in products:
        product = args.products/category/filename
        if macos:
            import plistlib
            with (product/'Contents/Info.plist').open('rb') as stream:
                executable = plistlib.load(stream)['CFBundleExecutable']
            arches = subprocess.check_output(['lipo','-archs',str(product/'Contents/MacOS'/executable)],text=True).split()
            if not {'arm64','x86_64'}.issubset(arches):
                raise ValueError('Universal 2 architecture missing')
            subprocess.run(['codesign','--verify','--deep','--strict',str(product)],check=True)
        elif category == 'Standalone':
            check_x64(product)
        else:
            candidates=list(product.glob('Contents/x86_64-win/*.vst3'))
            if len(candidates)!=1:raise ValueError('Expected one Windows x64 VST3 executable')
            check_x64(candidates[0])
    name = 'Silver-K-Blue-Embedded-' + args.platform + '-' + revision[:12]
    args.output.mkdir(parents=True, exist_ok=True)
    archive = args.output / (name + '.zip')
    if archive.exists():
        raise ValueError('Refusing to overwrite an existing package')
    with tempfile.TemporaryDirectory(prefix='silver-blue-') as temporary:
        stage = Path(temporary) / name
        for category, filename, destination in products:
            target=stage/destination/filename
            target.parent.mkdir(parents=True,exist_ok=True)
            built=args.products/category/filename
            if built.is_dir():shutil.copytree(built,target)
            else:shutil.copy2(built,target)
        shutil.copytree(ROOT / 'blue/Skins', stage / 'Skins')
        for f in ['LICENSE.md', 'THIRD_PARTY.md']:
            shutil.copy2(ROOT / f, stage / f)
        shutil.copy2(ROOT / 'doc/BLUE_PACKAGE_README.md', stage / 'README.md')
        for f in ['INSTALL.md', 'USER_GUIDE.md', 'SKINS.md']:
            shutil.copy2(ROOT / 'doc' / f, stage / f)
        (stage / 'README-HU.md').write_text('''# Silver-K Blue — beépített skines tesztbuild

A Plugins/VST3/Silver-K Blue.vst3 teljes csomagját a host által használt VST3 mappába másold. macOS-on a Plugins/AU/Silver-K Blue.component az AU mappába kerülhet. A Standalone mappában található alkalmazás önállóan indítható. Az eredeti Silver-K mellé telepíthető. A régi, közös azonosítójú Blue buildet a régi projektekhez külön őrizd meg.

A Silver-K Blue Standard v1.0 és Silver-K Blue Retina 4K v1.0 már be van építve mindegyik termékbe; a Setup skinválasztóban külön telepítés nélkül elérhetők. Új telepítésnél a Retina az alap. A Skins mappában a szerkeszthető külső változatokat is mellékeljük, de a beépített változatokhoz nem szükséges őket telepíteni.

Saját firmware és szükség esetén Wave ROM/programadat kell; gyártói ROM nincs ebben a csomagban. A ROM nélküli automatizált tesztek nem helyettesítik a DAW-ban végzett működési és hangpróbát. A Windows bináris nincs gyártói tanúsítvánnyal aláírva; a macOS helyi ad hoc aláírást használ, nem notarizált.

Forrás és licencek: a build-info.json fájlban rögzített repository és commit; LICENSE.md, THIRD_PARTY.md. Angol telepítési útmutató: README.md, INSTALL.md, USER_GUIDE.md.
''', encoding='utf-8')
        info = {'repository': 'https://github.com/RobCZart82/Silver-K-Blue', 'commit': revision,
                'platform': args.platform, 'configuration': 'Release',
                'skinsEmbedded': True, 'firmwareIncluded': False,
                'source': 'https://github.com/RobCZart82/Silver-K-Blue/tree/' + revision}
        (stage / 'build-info.json').write_text(json.dumps(info, indent=2) + '\n', encoding='utf-8')
        files = sorted(p for p in stage.rglob('*') if p.is_file())
        sums = {p.relative_to(stage).as_posix(): digest(p) for p in files}
        (stage / 'SHA256SUMS.json').write_text(json.dumps(sums, indent=2) + '\n', encoding='utf-8')
        with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as bundle:
            for path in sorted(stage.rglob('*')):
                if path.is_file():
                    bundle.write(path, path.relative_to(stage.parent))
        with zipfile.ZipFile(archive) as bundle:
            if bundle.testzip() is not None:
                raise ValueError('ZIP verification failed')
    archive.with_suffix('.zip.sha256').write_text(digest(archive) + '  ' + archive.name + '\n', encoding='utf-8')
    print('Created and verified:', archive)

if __name__ == '__main__':
    main()
