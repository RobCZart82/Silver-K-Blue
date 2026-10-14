#!/usr/bin/env python3
"""Package only built products, authored KS substitutes and release documentation."""
import argparse
import hashlib
import json
import plistlib
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path

VERSION = '0.2.4-alpha.1'
ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--products', type=Path, required=True)
    parser.add_argument('--dependencies', type=Path, required=True)
    parser.add_argument('--platform', choices=['Windows-x64', 'macOS-arm64'], required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    name = f'Silver-K-{VERSION}-{args.platform}'
    args.output.mkdir(parents=True, exist_ok=True)
    archive = args.output / (name + '.zip')
    if archive.exists():
        raise RuntimeError(f'Refusing to replace existing release archive: {archive}')
    with tempfile.TemporaryDirectory(prefix='silver-package-') as temp:
        stage = Path(temp) / name
        stage.mkdir()
        def copy(src, relative):
            dest = stage / relative
            dest.parent.mkdir(parents=True, exist_ok=True)
            if src.is_dir():
                shutil.copytree(src, dest, symlinks=True)
            else:
                shutil.copy2(src, dest)
            return dest
        if args.platform == 'macOS-arm64':
            products = [('Standalone', 'Silver-K.app', 'Standalone'),
                        ('VST3', 'Silver-K.vst3', 'Plugins/VST3'),
                        ('AU', 'Silver-K.component', 'Plugins/AU')]
        else:
            products = [('Standalone', 'Silver-K.exe', 'Standalone'),
                        ('VST3', 'Silver-K.vst3', 'Plugins/VST3')]
        for source, filename, target in products:
            built = args.products / source / filename
            if args.platform == 'macOS-arm64':
                info = plistlib.loads((built / 'Contents/Info.plist').read_bytes())
                if info.get('CFBundleShortVersionString') != '0.2.4':
                    raise RuntimeError(f'Incorrect bundle version: {built}')
                executable = built / 'Contents/MacOS' / info['CFBundleExecutable']
                arches = subprocess.check_output(['lipo', '-archs', str(executable)], text=True).strip()
                if arches != 'arm64':
                    raise RuntimeError(f'Unexpected architecture: {arches}')
                subprocess.run(['codesign', '--verify', '--deep', '--strict', str(built)], check=True)
            copy(built, target + '/' + filename)
        flash = ROOT / 'assets/ks-init-flash.bin'
        expected = json.loads((ROOT / 'assets/ks-init-flash.manifest.json').read_text())
        if digest(flash) != expected['sha256'] or flash.stat().st_size != expected['size']:
            raise RuntimeError('Synthetic KS Flash does not match its manifest')
        copy(flash, 'Silver-K/ROMs/ks-init-flash.bin')
        copy(ROOT / 'assets/ks-init-flash.manifest.json', 'Silver-K/ROMs/ks-init-flash.manifest.json')
        (stage / 'Silver-K/Patches').mkdir(parents=True)
        waves = stage / 'Silver-K/ROMs/ks-wave-zero.bin'
        waves.write_bytes(bytes(129024))
        wave_manifest = dict(origin='Silver-K synthetic zero bank', size=129024,
                             sha256=digest(waves), placeholder=True, contains_firmware=False,
                             contains_sampled_waves=False, mapped_base='0x860000',
                             generator='tools/package_release.py')
        (stage / 'Silver-K/ROMs/ks-wave-zero.manifest.json').write_text(json.dumps(wave_manifest, indent=2)+'\n')
        for src, dest in [('doc/INSTALL.md', 'INSTALL.md'),
                          ('doc/releases/0.2.4-alpha.1.md', 'RELEASE_NOTES.md'),
                          ('LICENSE.md', 'LICENSE.md'), ('THIRD_PARTY.md', 'THIRD_PARTY.md')]:
            copy(ROOT / src, dest)
        for dep in ['juce', 'dsp', 'asmjit']:
            copy(args.dependencies / f'silver_{dep}-src/LICENSE.md', f'Licenses/{dep}/LICENSE.md')
        for filename in ['LICENSE-BSD.txt', 'LICENSE-LGPL.txt']:
            copy(ROOT / 'source/3rdparty/libresample' / filename, 'Licenses/libresample/' + filename)
        # Include the actual local source, including current uncommitted C++ changes.
        # Original user assets and unrelated editor work are outside these roots.
        source_files = subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard'],cwd=ROOT,text=True).splitlines()
        source_archive = stage / 'Silver-K-source.zip'
        roots = ('source/','assets/','tools/','doc/','.github/')
        top = {'CMakeLists.txt','README.md','LICENSE.md','THIRD_PARTY.md','.gitignore'}
        with zipfile.ZipFile(source_archive,'w',zipfile.ZIP_DEFLATED) as source_zip:
            for relative in sorted(set(source_files)):
                path = ROOT / relative
                if (relative.startswith(roots) or relative in top) and path.is_file():
                    if path.suffix.lower() in {'.bin','.syx','.mid','.midi','.os','.fxb','.wav'} and relative!='assets/ks-init-flash.bin':
                        raise RuntimeError(f'Private/binary asset in source snapshot: {relative}')
                    source_zip.write(path,relative)
        dirty = bool(subprocess.check_output(['git','status','--porcelain'],cwd=ROOT,text=True).strip())
        sources = f'''Silver-K corresponding source is included in Silver-K-source.zip.
Snapshot SHA-256: {digest(source_archive)}
Base Git commit: {commit}; local changes included: {dirty}.
Upstream baseline (the bundled snapshot also includes local changes):
https://github.com/temaniak/Silver-K/tree/{commit}
https://github.com/temaniak/Silver-K/archive/{commit}.tar.gz

Build instructions and local compatibility patches are in that source tree.
Required dependency sources (built locally, not prebuilt binaries):
JUCE: https://github.com/dsp56300/JUCE/archive/2a8699d723fe9279e1148275485ddb0e18957f56.tar.gz
DSP56300: https://github.com/dsp56300/dsp56300/archive/44353be2f0156cac970c6ab4f467ea1998b40605.tar.gz
AsmJit: https://github.com/dsp56300/asmjit/archive/3577608cab0bc509f856ebf6e41b2f9d9f71acc4.tar.gz

Bundled framework, H8S, libresample and ptypes source/notices are in Silver-K's source tree.
No original firmware or factory sounds are included in this binary archive.
'''
        (stage / 'SOURCES.txt').write_text(sources, encoding='utf-8')
        manifest = dict(version=VERSION, plugin_version='0.2.4', platform=args.platform,
                        source_commit=commit, source_dirty=dirty, source_snapshot_sha256=digest(source_archive), firmware_included=False, factory_presets_included=False,
                        synthetic_ks_data=expected, synthetic_wave_rom=wave_manifest,
                        macos_signing='ad-hoc; not notarized' if args.platform.startswith('macOS') else None)
        (stage / 'release-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
        files = sorted(p for p in stage.rglob('*') if p.is_file() and not p.is_symlink())
        checksums = ''.join(f'{digest(p)}  {p.relative_to(stage).as_posix()}\n' for p in files)
        (stage / 'SHA256SUMS.txt').write_text(checksums, encoding='utf-8')
        if args.platform.startswith('macOS'):
            subprocess.run(['ditto', '-c', '-k', '--keepParent', str(stage), str(archive)], check=True)
        else:
            with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as package:
                for p in sorted(stage.rglob('*')):
                    package.write(p, p.relative_to(stage.parent).as_posix())
        with zipfile.ZipFile(archive) as package:
            if package.testzip() is not None:
                raise RuntimeError('Archive CRC check failed')
            if package.read(name+'/Silver-K/ROMs/ks-init-flash.bin') != flash.read_bytes():
                raise RuntimeError('Packaged init Flash differs from source')
            if package.read(name+'/Silver-K/ROMs/ks-wave-zero.bin') != bytes(129024):
                raise RuntimeError('Packaged wave substitute is not a zero bank')
        checksum = digest(archive)
        archive.with_suffix('.zip.sha256').write_text(f'{checksum}  {archive.name}\n', encoding='utf-8')
        print(f'Packaged {archive} ({archive.stat().st_size} bytes), SHA-256 {checksum}')


if __name__ == '__main__':
    main()
