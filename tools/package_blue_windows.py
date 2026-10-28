#!/usr/bin/env python3
"""Package private Windows x64 builds, with external Blue skins and provenance."""
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
    args = parser.parse_args()
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    exe = args.products / 'Standalone/Silver-K.exe'
    vst = args.products / 'VST3/Silver-K.vst3'
    check_x64(exe)
    if not vst.is_dir():
        raise ValueError('Missing VST3 bundle')
    candidates = list(vst.glob('Contents/x86_64-win/*.vst3'))
    if len(candidates) != 1:
        raise ValueError('Expected one Windows x64 VST3 executable')
    check_x64(candidates[0])
    name = 'Silver-K-Blue-Windows-x64-' + revision[:12]
    args.output.mkdir(parents=True, exist_ok=True)
    archive = args.output / (name + '.zip')
    if archive.exists():
        raise ValueError('Refusing to overwrite an existing package')
    with tempfile.TemporaryDirectory(prefix='silver-blue-') as temporary:
        stage = Path(temporary) / name
        (stage / 'Plugins/VST3').mkdir(parents=True)
        (stage / 'Standalone').mkdir()
        shutil.copytree(vst, stage / 'Plugins/VST3/Silver-K.vst3')
        shutil.copy2(exe, stage / 'Standalone/Silver-K.exe')
        shutil.copytree(ROOT / 'blue/Skins', stage / 'Skins')
        for f in ['LICENSE.md', 'THIRD_PARTY.md']:
            shutil.copy2(ROOT / f, stage / f)
        (stage / 'README-HU.md').write_text('''# Silver-K Blue — Windows x64 tesztbuild

A Plugins/VST3/Silver-K.vst3 teljes mappáját a host által használt VST3 mappába másold. A Standalone/Silver-K.exe önállóan indítható. A korábbi azonos Silver-K példányt előbb mentsd beolvasási útvonalon kívülre.

A Skins két almappáját a Setup → Skins folder helyére másold, majd válaszd a Silver-K Blue Standard v1.0 vagy Silver-K Blue Retina 4K v1.0 nevet. A skinek jelenleg külső csomagok; még nincsenek beépítve a binárisba.

Saját firmware és szükség esetén Wave ROM/programadat kell; gyártói ROM nincs ebben a csomagban. A ROM nélküli automatizált tesztek nem helyettesítik a Windows DAW-ban végzett működési és hangpróbát. Az alkalmazás nincs gyártói tanúsítvánnyal aláírva.

Forrás és licencek: a build-info.json fájlban rögzített privát repository és commit; LICENSE.md, THIRD_PARTY.md.
''', encoding='utf-8')
        info = {'repository': 'https://github.com/RobCZart82/Silver-K-Blue', 'commit': revision,
                'platform': 'Windows-x64', 'configuration': 'Release',
                'skinsEmbedded': False, 'firmwareIncluded': False,
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
