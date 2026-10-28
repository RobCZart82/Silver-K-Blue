#!/usr/bin/env python3
"""Pack captured cycles using the verified KS OS 2.1.02 loader table.

No firmware or samples are embedded in this script. Requires numpy and the
private prepared-capture manifest. Output assets must remain outside Git.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import wave

import numpy as np

OS_HASH = '7acd2d1a8a56f44522b12327437a27826b64fc49a18851816423546fd6ea085c'
BANK_SIZE = 129024


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_mono(path):
    with wave.open(str(path), 'rb') as wav:
        if (wav.getnchannels(),wav.getsampwidth(),wav.getframerate()) != (1,3,48000):
            raise ValueError(f'Expected mono PCM24 / 48 kHz: {path}')
        b=np.frombuffer(wav.readframes(wav.getnframes()),np.uint8).reshape(-1,3).astype(np.int32)
        return (((b[:,0]|b[:,1]<<8|b[:,2]<<16)^0x800000)-0x800000)/8388608.


def periodic_resample(x, length):
    # Periodic Fourier resampling: preserve DC and amplitude; remove frequencies
    # above the target Nyquist instead of folding them into the ROM cycle.
    source=len(x)
    spectrum=np.fft.rfft(x)
    result=np.zeros(length//2+1,dtype=complex)
    common=min(source,length)
    result[:common//2+1]=spectrum[:common//2+1]
    if common%2==0:
        if length<source:result[common//2]*=2
        elif source<length:result[common//2]*=.5
    return np.fft.irfft(result,n=length)*length/source


def slots(os):
    offset=0
    result=[]
    for index in range(8,32):
        length_flags,command=struct.unpack_from('>II',os,0x15140+(index-8)*8)
        count=length_flags>>16
        bits=8 if command&0x08000000 else 16
        size=count*bits//8
        result.append(dict(index=index,offset=offset,samples=count,bits=bits,
                           bytes=size,loader_command=f'0x{command:08x}',
                           encoding='unsigned_offset_binary_8' if bits==8 else 'signed_16_big_endian'))
        offset+=size
    if offset!=BANK_SIZE:raise ValueError('Unexpected KS loader table layout')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--os',type=Path,required=True)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--gain',type=float,default=1.)
    parser.add_argument('--polarity',type=int,choices=[-1,1],default=-1,
                        help='KS output pipeline inverts stored PCM; -1 restores capture polarity')
    parser.add_argument('--cycles',type=Path,help='JSON map of wave index to complete repetitions per ROM block')
    args=parser.parse_args()
    os=args.os.read_bytes()
    if len(os)!=87360 or digest(os)!=OS_HASH:raise ValueError('Use the verified decoded KS OS 2.1.02')
    if args.gain<=0 or not np.isfinite(args.gain):raise ValueError('Gain must be positive and finite')
    captures=json.loads((args.prepared/'manifest.json').read_text())
    rows={r['index']:r for r in captures['waves']}
    cycles=json.loads(args.cycles.read_text()) if args.cycles else {}
    bank=bytearray()
    metadata=[]
    for slot in slots(os):
        index=slot['index']
        row=rows[index]
        path=args.prepared/row['loop_candidate']['file']
        x=read_mono(path)
        repeats=int(cycles.get(str(index),1))
        if repeats<1 or slot['samples']%repeats:raise ValueError(f'Invalid repetitions for wave {index}')
        signal=np.tile(periodic_resample(x,slot['samples']//repeats),repeats)*args.gain*args.polarity
        signal-=signal.mean()
        peak=float(abs(signal).max())
        if peak>.98:raise ValueError(f'Wave {index} would exceed conservative .98 ROM peak: {peak}')
        if slot['bits']==16:
            pcm=np.rint(signal*32768).astype('>i2')
            encoded=pcm.tobytes()
        else:
            pcm=np.rint(signal*128).astype(np.int16)+128
            if pcm.min()<0 or pcm.max()>255:raise ValueError('8-bit range exceeded')
            encoded=pcm.astype(np.uint8).tobytes()
        if len(bank)!=slot['offset'] or len(encoded)!=slot['bytes']:raise ValueError('Slot packing mismatch')
        bank.extend(encoded)
        metadata.append(dict(**slot,name=row['name'],cycles_per_block=repeats,
                             capture_cycle_sha256=digest(path.read_bytes()),
                             capture_file=str(path.resolve()),source_file_sha256=row['source_sha256'],
                             peak_before_quantization=peak,rms_before_quantization=float(np.sqrt(np.mean(signal**2))),
                             mean_before_quantization=float(signal.mean()),
                             maximum_quantization_error=(.5/128 if slot['bits']==8 else .5/32768),
                             slot_sha256=digest(encoded)))
    data=bytes(bank)
    if len(data)!=BANK_SIZE or not any(data):raise ValueError('Invalid output bank')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    # Versioned assets are immutable: a different gain/map gets a new pathname.
    if args.output.exists():raise ValueError('Output exists; choose a new version filename')
    args.output.write_bytes(data)
    manifest=dict(schema_version=1,kind='KS captured-output experimental wave bank',
        firmware_sha256=OS_HASH,firmware_modified=False,loader_table_offset='0x15140',
        loader_routine='0x76DA..0x77DA',size_bytes=len(data),sha256=digest(data),
        bank_file=args.output.name,global_linear_gain=args.gain,stored_pcm_polarity=args.polarity,
        processing='source measured cycle; Fourier periodic resampling; configured whole-cycle repetitions; DC removal; PCM quantization',
        limitations=['Output captures include the physical signal path; this is not an original ROM dump.',
                     '8-bit Open Hi-Hat and Cymbal quantization is required by the original firmware.',
                     'Cycle phase and amplitude remain approximate and require hardware comparison.'],
        slots=metadata)
    args.output.with_suffix('.manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n')
    # Independently unpack every slot and compare with its declared hash/range.
    for slot in metadata:
        block=data[slot['offset']:slot['offset']+slot['bytes']]
        assert digest(block)==slot['slot_sha256']
        decoded=(np.frombuffer(block,np.uint8).astype(float)-128)/128 if slot['bits']==8 else np.frombuffer(block,'>i2').astype(float)/32768
        assert len(decoded)==slot['samples'] and abs(decoded).max()<1
    print(json.dumps(dict(bank=str(args.output.resolve()),sha256=digest(data),size=len(data),slots=24),indent=2))


if __name__=='__main__':main()
