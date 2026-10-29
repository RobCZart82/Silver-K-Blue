#!/usr/bin/env python3
"""Deterministic, bounded compilation chunks for embedded Blue skins."""
from pathlib import Path
import io
import sys
import zipfile
ROOT=Path(__file__).resolve().parents[1]
CHUNK=4*1024*1024

def generate(output):
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    expected=set()
    for label,folder in [('standard','Silver-K Blue Standard v1.0'),('retina','Silver-K Blue Retina 4K v1.0')]:
        source=ROOT/'blue/Skins'/folder
        data=io.BytesIO()
        with zipfile.ZipFile(data,'w',zipfile.ZIP_STORED) as archive:
            for path in sorted(source.rglob('*')):
                relative=path.relative_to(source)
                if path.is_file() and not any(part.startswith('.') for part in relative.parts):
                    info=zipfile.ZipInfo(relative.as_posix(),(2026,10,25,0,0,0))
                    archive.writestr(info,path.read_bytes())
        payload=data.getvalue()
        for number,offset in enumerate(range(0,len(payload),CHUNK)):
            name='blue'+label+str(number)+'.bin';expected.add(name)
            target=output/name;chunk=payload[offset:offset+CHUNK]
            if not target.exists() or target.read_bytes()!=chunk:target.write_bytes(chunk)
    for old in output.glob('blue*.bin'):
        if old.name not in expected:old.unlink()

if __name__=='__main__':generate(sys.argv[1])
