#!/usr/bin/env python3
"""Check every embedded resource byte and deterministic bounded chunk generation."""
from pathlib import Path
import io
import tempfile
import zipfile
from embed_blue_skins import generate,ROOT,CHUNK
with tempfile.TemporaryDirectory() as temporary:
    output=Path(temporary);generate(output)
    times={f.name:f.stat().st_mtime_ns for f in output.iterdir()}
    generate(output)
    assert times=={f.name:f.stat().st_mtime_ns for f in output.iterdir()}
    for label,folder in [('standard','Silver-K Blue Standard v1.0'),('retina','Silver-K Blue Retina 4K v1.0')]:
        chunks=[];number=0
        while (output/('blue'+label+str(number)+'.bin')).exists():
            chunk=(output/('blue'+label+str(number)+'.bin')).read_bytes()
            assert 0<len(chunk)<=CHUNK;chunks.append(chunk);number+=1
        with zipfile.ZipFile(io.BytesIO(b''.join(chunks))) as archive:
            files={p.relative_to(ROOT/'blue/Skins'/folder).as_posix():p for p in (ROOT/'blue/Skins'/folder).rglob('*') if p.is_file() and not any(part.startswith('.') for part in p.relative_to(ROOT/'blue/Skins'/folder).parts)}
            assert set(files)==set(archive.namelist())
            for name,path in files.items():assert archive.read(name)==path.read_bytes(),name
        print('PASS:',label,number,'bounded chunks; all source resource bytes exact; hidden files excluded')
print('PASS: generation is deterministic and unchanged resources retain timestamps')
