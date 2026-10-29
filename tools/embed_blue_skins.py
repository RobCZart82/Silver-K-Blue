#!/usr/bin/env python3
"""Create deterministic embedded skin archives; no runtime extraction required."""
from pathlib import Path
import sys,zipfile
root=Path(__file__).resolve().parents[1]
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
for label,folder in [('standard','Silver-K Blue Standard v1.0'),('retina','Silver-K Blue Retina 4K v1.0')]:
    with zipfile.ZipFile(out/('blue-'+label+'.zip'),'w',zipfile.ZIP_STORED) as archive:
        for path in sorted((root/'blue/Skins'/folder).rglob('*')):
            if path.is_file():
                info=zipfile.ZipInfo(path.relative_to(root/'blue/Skins'/folder).as_posix(),(2026,10,25,0,0,0))
                archive.writestr(info,path.read_bytes())
