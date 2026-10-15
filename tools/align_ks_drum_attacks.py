#!/usr/bin/env python3
"""Audit and phase-align KS rhythm captures17..31 without deleting samples.

The search windows below were visually reviewed for this exact capture set.
This is not a general-purpose transient detector. Requires numpy/matplotlib.
"""
import argparse
import copy
import csv
import hashlib
import json
from pathlib import Path
import wave
import zipfile

import numpy as np
from build_ks_wave_rom import read_mono
from prepare_ks_waves import read_pcm, write_pcm

V1_SHA='2c784a6bb144db5b13768cf63feaddea421b4a8a0246726da84cb8b046df6bfb'
# Milliseconds relative to the v1 C1 cycle. Restrict to the FIRST rise of the
# attack, not its loudest peak or a later noisy fluctuation. Kick3 has an early
# onset before the much stronger second burst. Tom's attack straddles phase0.
WINDOWS={17:(20.5,23),18:(53.8,56),19:(14.5,17),20:(36,40),21:(4.5,7),
         22:(195.5,198),23:(-1,2),24:(-.5,2),25:(3.5,6),
         27:(36.5,40),28:(54,58),29:(23.5,27),30:(4,7),31:(22.5,26)}


def sha(data):return hashlib.sha256(data).hexdigest()


def periodic_mean(x,width):
    return sum(np.roll(x,k) for k in range(-width//2,width-width//2))/width


def onset(x,window):
    # Second difference suppresses the slow kick/tom body, exposing the front
    # of its noisy click. Centered250us RMS has at most125us lookahead.
    dx=x-2*np.roll(x,1)+np.roll(x,2)
    hf=np.sqrt(periodic_mean(dx*dx,12))
    a,b=[round(t*48) for t in window]
    baseline=float(np.median(hf[np.arange(a-72,a-12)%len(x)]))
    peak=float(hf[np.arange(a,b)%len(x)].max())
    threshold=baseline+.05*(peak-baseline)
    possible=np.arange(a,b)
    valid=(hf[possible%len(x)]>threshold)&np.array([
        np.mean(hf[(k+np.arange(12))%len(x)]>threshold)>=.75 for k in possible])
    hits=np.flatnonzero(valid)
    if not len(hits) or peak<8*baseline:raise ValueError('Ambiguous attack; review window')
    edge=int(possible[hits[0]]%len(x))
    # Preserve6extra samples before the detected rise. Including RMS lookahead,
    # this leaves about0.125..0.25ms safety; never cut directly at a peak.
    return edge,(edge-6)%len(x),peak/max(baseline,1e-12)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--v1-package',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists():raise ValueError('Choose a new output directory')
    with zipfile.ZipFile(args.v1_package) as z:
        original=z.read('KS-Wave-ROM-trial-v1/ks-wave-captured-v1.bin')
        base=json.loads(z.read('KS-Wave-ROM-trial-v1/ks-wave-captured-v1.manifest.json'))
        zero=z.read('KS-Wave-ROM-trial-v1/ks-wave-zero.bin')
    if sha(original)!=V1_SHA or len(original)!=129024:raise ValueError('Wrong v1 bank')
    if len(zero)!=129024 or any(zero):raise ValueError('Wrong rollback zero bank')
    prepared=json.loads((args.prepared/'manifest.json').read_text())
    sources={r['index']:r for r in prepared['waves']}
    slots={r['index']:r for r in base['slots']}
    args.output.mkdir(parents=True)
    (args.output/'rollback').mkdir()
    (args.output/'rollback/ks-wave-captured-v1.bin').write_bytes(original)
    (args.output/'rollback/ks-wave-zero.bin').write_bytes(zero)
    bank=bytearray(original);rows=[];plots=[]
    for index in range(17,32):
        row=sources[index];slot=slots[index]
        path=args.prepared/row['loop_candidate']['file']
        if sha(path.read_bytes())!=slot['capture_cycle_sha256']:raise ValueError('Capture changed')
        x=read_mono(path);size=len(x)
        if index==26:
            edge=cut=0;contrast=None;status='no_unique_transient_continuous_noise_phase_preserved'
        else:
            edge,cut,contrast=onset(x,WINDOWS[index]);status='visually_reviewed_attack_rise'
        native_shift=round(cut/size*slot['samples'])%slot['samples']
        offset=slot['offset'];length=slot['bytes'];word=slot['bits']//8
        before=original[offset:offset+length]
        matrix=np.frombuffer(before,np.uint8).reshape(-1,word)
        after=np.roll(matrix,-native_shift,axis=0).tobytes()
        assert np.array_equal(np.roll(np.frombuffer(after,np.uint8).reshape(-1,word),native_shift,axis=0),matrix)
        bank[offset:offset+length]=after
        new=np.roll(x,-cut)
        write_pcm(args.output/'cycles_before'/path.name,x)
        write_pcm(args.output/'cycles_attack_aligned'/path.name,new)
        assert np.array_equal(np.roll(new,cut),x)
        # Prove timing/level/spectrum preservation in quantized native ROM.
        unpack=lambda data:(np.frombuffer(data,np.uint8).astype(float)-128 if word==1 else np.frombuffer(data,'>i2').astype(float))
        old_pcm=unpack(before);new_pcm=unpack(after)
        assert np.array_equal(np.sort(old_pcm),np.sort(new_pcm))
        assert np.allclose(abs(np.fft.rfft(old_pcm)),abs(np.fft.rfft(new_pcm)),rtol=1e-10,atol=1e-7)
        source_period=row['loop_candidate']['period_frames']
        point=row['loop_candidate']['source_start_frame']+edge*source_period/size
        limit=row['notes'][0]['source_end_frame']
        # Last150ms of each slot can contain the gate release and silence;
        # do not mistake those for missing/inconsistent waveform transients.
        repeats=[point+k*source_period for k in range(-100,100) if 2400<point+k*source_period<limit-7200]
        source=Path(row['source_path'])
        if sha(source.read_bytes())!=row['source_sha256']:raise ValueError('Source recording changed')
        raw=read_pcm(source)[:,0]
        raw_dx=raw-2*np.roll(raw,1)+np.roll(raw,2)
        ratios=[]
        if index!=26:
            for t in repeats:
                t=round(t)
                pre=np.sqrt(np.mean(raw_dx[t-120:t-48]**2))
                post=np.sqrt(np.mean(raw_dx[t+12:t+60]**2))
                ratios.append(float(post/max(pre,1e-12)))
            if not ratios or min(ratios)<3:
                raise ValueError(f'Wave{index}: onset not corroborated in repeated source attacks')
        record=dict(index=index,name=row['name'],status=status,
            cycle_frames_48k=size,attack_old_cycle_frame=edge if index!=26 else None,
            attack_old_cycle_ms=edge/48 if index!=26 else None,
            cut_old_cycle_frame=cut,cut_old_cycle_ms=cut/48,
            native_shift_frames=native_shift,native_slot_samples=slot['samples'],
            new_attack_frame_48k=(edge-cut)%size if index!=26 else None,
            source_attack_frames_C1=repeats if index!=26 else [],
            source_repeat_front_energy_ratios=ratios,
            source_sha256=row['source_sha256'],cycle_sha256_before=sha(path.read_bytes()),
            cycle_sha256_after=sha((args.output/'cycles_attack_aligned'/path.name).read_bytes()),
            native_slot_sha256_before=sha(before),native_slot_sha256_after=sha(after),
            detector_front_contrast=contrast)
        rows.append(record);plots.append((record,x,new))
    for index in range(8,17):
        slot=slots[index];a=slot['offset'];b=a+slot['bytes'];assert bank[a:b]==original[a:b]
    assert len(bank)==129024
    bankpath=args.output/'ks-wave-captured-v2-attacks.bin';bankpath.write_bytes(bank)
    result=dict(kind='KS rhythm transient alignment v2',date='2026-10-15',
        source_v1_sha256=V1_SHA,bank_sha256=sha(bank),bank_size_bytes=len(bank),
        method='Reviewed first-rise windows;250us second-difference RMS;5%rise;125usadditional safety. Integer circular rotations only.',
        corrected_attacks=14,unresolved_continuous_noise=[26],
        tonal_slots_8_to_16_bit_identical=True,native_sample_values_preserved=True,
        slot_lengths_rms_peaks_and_spectral_magnitudes_preserved=True,
        no_fades_no_new_gain_no_resampling=True,
        limitations=['Closed Hi-Hat has no unique attack in the held-wave capture; its phase is unchanged.',
                     'Attack alignment in ROM does not override a patch set to random oscillator start phase.',
                     'Existing v1 timbre reconstruction and8bit quantization limits remain.'],waves=rows)
    (args.output/'attacks.json').write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n')
    fields=['index','name','status','attack_old_cycle_ms','cut_old_cycle_ms','native_shift_frames','native_slot_samples','new_attack_frame_48k']
    with (args.output/'attacks.csv').open('w',newline='') as file:
        writer=csv.DictWriter(file,fieldnames=fields,extrasaction='ignore');writer.writeheader();writer.writerows(rows)
    make_plot(args.output,plots)
    print(json.dumps(dict(output=str(args.output.resolve()),bank_sha256=sha(bank),corrected=14,ambiguous=[26]),indent=2))


def make_plot(root,items):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    fig,axes=plt.subplots(15,2,figsize=(16,26))
    for (row,x,y),pair in zip(items,axes):
        for ax,z,title in zip(pair,[x,y],['v1','v2 attack aligned']):
            t=np.arange(len(z))/48;ax.plot(t,z,lw=.5,alpha=.7)
            env=np.sqrt(periodic_mean(z*z,24));ax.plot(t,env,lw=.8,color='#d17c00')
            ax.set_title(f"{row['index']} {row['name']} — {title}",fontsize=10)
            ax.set_xlim(0,len(z)/48);ax.grid(alpha=.15);ax.set_xlabel('ms')
        if row['attack_old_cycle_ms'] is not None:
            pair[0].axvline(row['attack_old_cycle_ms'],color='#be2634',ls='--',lw=1)
            pair[1].axvline(row['new_attack_frame_48k']/48,color='#248857',ls='--',lw=1)
        else:pair[1].text(.5,.8,'No unique transient; unchanged',transform=pair[1].transAxes,ha='center')
    fig.tight_layout();fig.savefig(root/'attacks-before-after.png',dpi=130);plt.close(fig)


if __name__=='__main__':main()
