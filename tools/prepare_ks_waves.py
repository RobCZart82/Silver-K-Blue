#!/usr/bin/env python3
"""Prepare private KS output captures; deliberately does not guess ROM encoding.

Requires numpy and matplotlib. Source WAVs are never changed. The output must
not exist. All offsets in the manifest are zero-based sample-frame offsets.
"""
import argparse
import csv
import hashlib
import html
import json
from pathlib import Path
import re
import shutil
import wave

import numpy as np

NAMES = [
    'Sine', 'Tri-Angle', 'Sawtooth', 'Square (Pulse)', 'White Noise',
    'Hi-Pass Noise', 'Band Pass Noise', 'Hi / Band Pass Noise', 'Organ',
    'Harpiscord', 'Electric Piano', 'Slap Bass', 'Rhodes Piano', 'Rhodes Tine',
    'Whirly EP', 'Clavinet', 'Analogue Bass', 'Kick 1', 'Kick 2', 'Kick 3',
    'Snare 1', 'Snare 2', 'Snare 3', 'Tom Tom (Front)', 'Rimshot', 'Stick',
    'Closed Hi-Hat', 'Open Hi-Hat', 'Tamborine', 'Conga', 'Cow Bell', 'Cymbal',
]
MANUAL = ('https://fael-downloads-prod.focusrite.com/customer/prod/'
          's3fs-public/novation/downloads/10720/english102.pdf')
KEYBOARD_MANUAL = ('https://fael-downloads-prod.focusrite.com/customer/dev/'
                   's3fs-public/novation/downloads/10719/english101.pdf')


def db(value):
    return float(20 * np.log10(max(float(value), 1e-15)))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_pcm(path):
    with wave.open(str(path), 'rb') as wav:
        if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) != (2, 3, 48000):
            raise ValueError(f'{path}: expected stereo PCM24 / 48000 Hz')
        raw = np.frombuffer(wav.readframes(wav.getnframes()), np.uint8).reshape(-1, 3)
        raw = raw.astype(np.int32)
        pcm = raw[:, 0] | (raw[:, 1] << 8) | (raw[:, 2] << 16)
        return ((pcm ^ 0x800000) - 0x800000).reshape(-1, 2) / 8388608.0


def write_pcm(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    pcm = np.rint(data * 8388608).astype(np.int64)
    if pcm.max() > 8388607 or pcm.min() < -8388608:
        raise ValueError(f'Clipping during export: {path}')
    unsigned = (pcm.ravel() & 0xffffff).astype(np.uint32)
    raw = np.column_stack([unsigned & 255, (unsigned >> 8) & 255, unsigned >> 16]).astype(np.uint8)
    with wave.open(str(path), 'wb') as wav:
        wav.setnchannels(1 if data.ndim == 1 else data.shape[1])
        wav.setsampwidth(3)
        wav.setframerate(48000)
        wav.writeframes(raw.tobytes())


def normalized_name(name):
    return re.sub(r'[^a-z0-9]', '', name.lower())


def slug(name):
    return re.sub(r'[^a-z0-9]+', '_', name.lower()).strip('_')


def correlation(x, lag):
    lag = int(lag)
    return float(np.corrcoef(x[:-lag], x[lag:])[0, 1])


def estimate_period(y, expected):
    # Match multiple cycles when available to improve fractional-lag precision.
    multiple = min(8, max(1, int(len(y) / (expected * 2))))
    shift = expected * multiple
    positions = np.arange(0, len(y) - int(shift * 1.01) - 2, dtype=float)
    base = y[positions.astype(int)]
    def error(lag):
        other = np.interp(positions + lag, np.arange(len(y)), y)
        return float(np.mean((base - other)**2))
    # Brute coarse search prevents a golden-section search selecting a small
    # local maximum in high-frequency percussion, then narrow the minimum.
    values = np.arange(shift - 6, shift + 6.01, .25)
    best = float(values[np.argmin([error(v) for v in values])])
    lo, hi = best - .3, best + .3
    for _ in range(28):
        a, b = lo + (hi-lo)/3, hi - (hi-lo)/3
        if error(a) < error(b):
            hi = b
        else:
            lo = a
    return (lo + hi) / 2 / multiple


def loop_candidate(y, expected):
    period = estimate_period(y, expected)
    count = int(round(period))
    # Choose a low-energy seam with similar value and slope at both ends.
    possible = np.arange(2, len(y) - int(np.ceil(period)) - 2, dtype=float)
    end = np.interp(possible + period, np.arange(len(y)), y)
    end_slope = (np.interp(possible + period + 1, np.arange(len(y)), y) -
                 np.interp(possible + period - 1, np.arange(len(y)), y)) / 2
    start = y[possible.astype(int)]
    start_slope = (y[possible.astype(int)+1] - y[possible.astype(int)-1]) / 2
    score = (start-end)**2 + (start_slope-end_slope)**2 + .002*(start**2+end**2)
    offset = int(possible[np.argmin(score)])
    # Resample only the fractional cycle boundary, not to a guessed ROM size.
    loop = np.interp(offset + np.arange(count)*period/count, np.arange(len(y)), y)
    dc = float(loop.mean())
    loop -= dc
    corr = correlation(y, round(period))
    return loop, dict(period_frames=period, frames=count, period_seconds=period/48000,
                      matched_cycle_correlation=corr, local_start_frame=offset,
                      removed_dc=dc, boundary_step=float(loop[0]-loop[-1]),
                      peak_dbfs=db(abs(loop).max()), rms_dbfs=db(np.sqrt(np.mean(loop**2))))


def note_boundaries(index, length):
    # Grid observed in captures: 55 BPM, one-beat slots; Snare 1 has a two-beat
    # first slot, Cymbal has three- and two-beat slots before its last note.
    beats = [2, 3] if index == 20 else [3, 5] if index == 31 else [1, 2]
    return [0] + [round(b*60/55*48000) for b in beats] + [length]


def octave_diagnostic(audio, bounds, first_period):
    periods = []
    for number, (start, end) in enumerate(zip(bounds[:-1], bounds[1:])):
        y = audio[start+2400:end-7200, 0]
        expected = first_period / 2**number
        length = len(y)
        ac = np.fft.irfft(abs(np.fft.rfft(y, 2*length))**2)[:length]
        energy = np.r_[0, np.cumsum(y*y)]
        lags = np.arange(int(expected*.90), min(int(expected*1.10), length-2400))
        corr = ac[lags] / np.sqrt(energy[length-lags]*(energy[length]-energy[lags]))
        best = int(lags[np.argmax(corr)])
        periods.append(dict(period_frames=float(best), correlation=float(np.max(corr))))
    return dict(method='normalized autocorrelation around nominal octave periods, integer-frame precision',
                periods=periods, first_to_second_ratio=periods[0]['period_frames']/periods[1]['period_frames'],
                second_to_third_ratio=periods[1]['period_frames']/periods[2]['period_frames'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    files = {}
    for path in args.source.glob('*.wav'):
        match = re.fullmatch(r'(\d+)\s*-\s*(.+)\.wav', path.name, re.I)
        if not match:
            raise ValueError(f'Unrecognized filename: {path.name}')
        index = int(match[1])
        if index in files or not 0 <= index < 32:
            raise ValueError(f'Duplicate/out-of-range index: {index}')
        if normalized_name(match[2]) != normalized_name(NAMES[index]):
            raise ValueError(f'Manual name mismatch: {path.name}')
        files[index] = path
    if set(files) != set(range(32)):
        raise ValueError(f'Missing waves: {sorted(set(range(32))-set(files))}')
    if args.output.exists():
        raise ValueError('Output exists; choose a new directory to avoid overwriting assets')
    # Validate all input formats before creating any outputs.
    for path in files.values():
        read_pcm(path)
    args.output.mkdir(parents=True)
    rows, envelopes, loops = [], [], []
    for index, path in sorted(files.items()):
        audio = read_pcm(path)
        left = audio[:, 0]
        base = f'{index:02d}_{slug(NAMES[index])}'
        group = 'generated_reference' if index < 4 else 'noise_reference' if index < 8 else 'tonal_sample' if index < 17 else 'rhythm_sample'
        original = args.output / 'originals_sorted' / group / (base + '.wav')
        original.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, original)
        bounds = note_boundaries(index, len(audio))
        normalized_gain = 10**(-3/20) / abs(left).max()
        aligned = audio[12000:36000]
        lags = list(range(-8, 9))
        correlations = []
        for lag in lags:
            l = aligned[max(0,lag):min(len(aligned),len(aligned)+lag), 0]
            r = aligned[max(0,-lag):min(len(aligned),len(aligned)-lag), 1]
            correlations.append(float(np.corrcoef(l, r)[0, 1]))
        row = dict(index=index, name=NAMES[index], source_name=path.name,
                   source_path=str(path.resolve()), source_sha256=sha(path),
                   sorted_copy=str(original.relative_to(args.output)), group=group,
                   sample_rate=48000, channels=2, bits=24, frames=len(audio),
                   duration_seconds=len(audio)/48000, source_peak_dbfs=db(abs(audio).max()),
                   left_peak_dbfs=db(abs(left).max()), left_rms_dbfs=db(np.sqrt(np.mean(left**2))),
                   dc_mean=float(left.mean()), clipping_samples=int(np.count_nonzero(abs(audio)>=1-1/8388608)),
                   lr_correlation=float(np.corrcoef(audio.T)[0, 1]),
                   lr_best_lag_frames=lags[int(np.argmax(correlations))],
                   lr_aligned_correlation=max(correlations),
                   right_minus_left_rms_db=db(np.sqrt(np.mean(audio[:,1]**2)))-db(np.sqrt(np.mean(left**2))),
                   optional_peak_gain_db=db(normalized_gain), notes=[])
        for note, (a, b) in enumerate(zip(bounds[:-1], bounds[1:])):
            stereo = audio[a:b]
            # Retain a 5 ms pad around material above -70 dBFS. Trim silence,
            # without flattening internal decays or tapering transients.
            active = np.flatnonzero(abs(stereo[:,0]) > 10**(-70/20))
            ta = max(0, int(active[0])-240) if len(active) else 0
            tb = min(len(stereo), int(active[-1])+241) if len(active) else len(stereo)
            mono = stereo[ta:tb,0]
            note_name = f'{base}_C{note+1}.wav'
            stereo_path = Path('notes_stereo')/group/note_name
            mono_path = Path('notes_mono_preserved')/group/note_name
            norm_path = Path('notes_mono_peak_minus3')/group/note_name
            write_pcm(args.output/stereo_path, stereo)
            write_pcm(args.output/mono_path, mono)
            write_pcm(args.output/norm_path, mono*normalized_gain)
            row['notes'].append(dict(nominal_note=f'C{note+1}',
                source_start_frame=a, source_end_frame=b,
                trimmed_start_frame=a+ta, trimmed_end_frame=a+tb,
                frames=len(mono), peak_dbfs=db(abs(mono).max()),
                stereo_file=str(stereo_path), mono_file=str(mono_path),
                optional_peak_file=str(norm_path)))
        if not 4 <= index < 8:
            # Lowest note offers most bandwidth. Exclude attack/release and
            # use longer first slots where the source explicitly provides them.
            start = 2400
            end = min(bounds[1]-7200, round((2.0 if index == 20 else 3.05 if index == 31 else .94)*48000))
            y = left[start:end]
            expected = 48000/(440*2**((24-69)/12))
            if index in [17,18,19,23,24,29,30]:
                expected *= 4
            elif index in [21,27,28]:
                expected = 11745
            elif index in [20,22]:
                expected = 23490
            elif index == 31:
                expected = 46980
            loop, info = loop_candidate(y, expected)
            info['source_start_frame'] = start + info.pop('local_start_frame')
            info['source_note'] = 'C1 (user label)'
            info['status'] = 'candidate_requires_audition_and_firmware_mapping'
            info['processing'] = 'one measured period; fractional linear interpolation; DC subtraction; no crossfade or averaging'
            info['file'] = str(Path('loop_candidates_48k')/group/(base+'.wav'))
            write_pcm(args.output/info['file'], loop)
            gain = 10**(-3/20)/abs(loop).max()
            info['optional_peak_gain_db'] = db(gain)
            info['optional_peak_file'] = str(Path('loop_candidates_peak_minus3')/group/(base+'.wav'))
            write_pcm(args.output/info['optional_peak_file'], loop*gain)
            row['loop_candidate'] = info
            loops.append((index, loop))
            if index in [20, 31]:
                row['octave_diagnostic'] = octave_diagnostic(audio, bounds, info['period_frames'])
        rows.append(row)
        size = 480
        envelope = np.sqrt(np.mean(left[:len(left)//size*size].reshape(-1,size)**2, axis=1))
        envelopes.append((index, envelope, bounds))

    manifest = dict(schema_version=1, recorded_settings='User: C1 C2 C3; all zero; no envelopes/modulation',
        note_labels='Nominal labels from user, not independently verified MIDI events; C1=32.703 Hz convention assumed for tonal analysis',
        manuals=[dict(url=MANUAL, page_printed=23, pdf_page=23), dict(url=KEYBOARD_MANUAL, page_printed=23, pdf_page=23)],
        verified_manual_names_and_indices=True, source_count=32, missing_indices=[],
        note_grid='55 BPM inferred from file lengths and silence gaps; cut at 1-beat slots, index20 first 2 beats, index31 first 3 then 2 beats; all input frames preserved in stereo splits',
        primary_mono_policy='left channel only; no summing, gain, EQ, dynamics, denoising or pitch correction',
        optional_peak_policy='one gain per complete wave across C1/C2/C3; peak -3 dBFS; separate loop peak variant; these variants change relative loudness between waves',
        rom=dict(bank_size_bytes=129024, mapped_base='0x860000', mapped_last='0x87f7ff',
                 source='source/novation/ks/ksLib/ksBoard.h',
                 binary_created=False, status='Prepared assets; firmware slot layout, encoding, lengths, scale and base pitch must be verified before packing'),
        waves=rows)
    (args.output/'manifest.json').write_text(json.dumps(manifest, indent=2, ensure_ascii=False)+'\n')
    fields = ['index','name','group','duration_seconds','source_peak_dbfs','left_rms_dbfs',
              'clipping_samples','lr_best_lag_frames','lr_aligned_correlation','optional_peak_gain_db','source_sha256']
    with (args.output/'catalog.csv').open('w',newline='') as file:
        writer=csv.DictWriter(file,fieldnames=fields,extrasaction='ignore')
        writer.writeheader();writer.writerows(rows)
    make_plots(args.output, rows, envelopes, loops)
    make_report(args.output, rows)
    verification = verify(args.output, rows)
    (args.output/'verification.json').write_text(json.dumps(verification,indent=2)+'\n')
    shutil.copy2(Path(__file__), args.output/'prepare_ks_waves.py')
    checksum_files = sorted(p for p in args.output.rglob('*') if p.is_file())
    (args.output/'SHA256SUMS.txt').write_text(''.join(f'{sha(p)}  {p.relative_to(args.output)}\n' for p in checksum_files))
    print(json.dumps(dict(output=str(args.output.resolve()), waves=32,
                         verification=verification),indent=2))


def make_plots(output, rows, envelopes, loops):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    plt.rcParams.update({'font.family':'DejaVu Sans','font.size':9})
    fig, ax = plt.subplots(figsize=(15,6))
    indexes=np.arange(32)
    ax.bar(indexes-.2,[r['source_peak_dbfs'] for r in rows],.4,label='Stereo peak')
    ax.bar(indexes+.2,[r['left_rms_dbfs'] for r in rows],.4,label='Left RMS, whole capture')
    ax.set_xticks(indexes);ax.set_xticklabels([f"{r['index']:02d} {r['name']}" for r in rows],rotation=65,ha='right')
    ax.set_ylabel('dBFS');ax.set_ylim(-35,0);ax.legend();ax.grid(axis='y',alpha=.2)
    fig.tight_layout();fig.savefig(output/'levels.png',dpi=140);plt.close(fig)
    fig, axes=plt.subplots(8,4,figsize=(16,16))
    for ax,(index,envelope,bounds) in zip(axes.ravel(),envelopes):
        ax.plot(np.arange(len(envelope))*.01,envelope,lw=.7)
        for boundary in bounds[1:-1]:ax.axvline(boundary/48000,color='orange',lw=.8)
        ax.set_title(f'{index:02d} {NAMES[index]}',fontsize=10)
        ax.set_xlim(0,bounds[-1]/48000);ax.set_ylim(bottom=0)
        ax.set_xlabel('seconds');ax.grid(alpha=.15)
    fig.tight_layout();fig.savefig(output/'envelopes.png',dpi=125);plt.close(fig)
    fig, axes=plt.subplots(7,4,figsize=(16,13))
    for ax,(index,loop) in zip(axes.ravel(),loops):
        ax.plot(np.arange(len(loop))/len(loop),loop,lw=.6)
        ax.set_title(f'{index:02d} {NAMES[index]} / {len(loop)} frames',fontsize=9)
        ax.set_xlim(0,1);ax.set_xlabel('one measured period');ax.grid(alpha=.15)
    fig.tight_layout();fig.savefig(output/'loops.png',dpi=125);plt.close(fig)


def verify(output, rows):
    for row in rows:
        assert sha(output/row['sorted_copy']) == row['source_sha256']
        source=read_pcm(Path(row['source_path']))
        pieces=[read_pcm(output/n['stereo_file']) for n in row['notes']]
        assert np.array_equal(np.concatenate(pieces),source), row['name']
        for note in row['notes']:
            with wave.open(str(output/note['mono_file'])) as wav:
                raw=wav.readframes(wav.getnframes())
                assert wav.getnchannels()==1 and wav.getsampwidth()==3 and wav.getframerate()==48000
            b=np.frombuffer(raw,np.uint8).reshape(-1,3).astype(np.int32)
            mono=((b[:,0]|b[:,1]<<8|b[:,2]<<16)^0x800000)-0x800000
            reference=np.rint(source[note['trimmed_start_frame']:note['trimmed_end_frame'],0]*8388608).astype(np.int32)
            assert np.array_equal(mono,reference), row['name']
    wav_files=list(output.rglob('*.wav'))
    for path in wav_files:
        with wave.open(str(path)) as wav:
            assert wav.getnframes()>0 and wav.getframerate()==48000
            b=np.frombuffer(wav.readframes(wav.getnframes()),np.uint8).reshape(-1,3).astype(np.int32)
            pcm=((b[:,0]|b[:,1]<<8|b[:,2]<<16)^0x800000)-0x800000
            assert np.max(pcm)<8388607 and np.min(pcm)>-8388608
    return dict(passed=True, original_copies_bit_identical=32,
                stereo_note_splits_reconstruct_sources_bit_exactly=32,
                mono_left_channel_preserved_bit_exactly=96,
                wav_files_validated=len(wav_files), output_clipped_samples=0,
                waveform_indices_complete=True,
                rom_binary_compatibility_tested=False, listening_test_performed=False)


def make_report(output, rows):
    def e(value):return html.escape(str(value))
    table=''.join('<tr>'+''.join('<td>'+e(v)+'</td>' for v in [
        f"{r['index']:02d}", r['name'],r['group'],f"{r['duration_seconds']:.3f}",
        f"{r['source_peak_dbfs']:.2f}",f"{r['left_rms_dbfs']:.2f}",
        f"{r['optional_peak_gain_db']:+.2f}",
        f"{r['loop_candidate']['period_frames']:.3f}" if 'loop_candidate' in r else '—',
        f"{r['loop_candidate']['matched_cycle_correlation']:.4f}" if 'loop_candidate' in r else '—'])+'</tr>' for r in rows)
    octave_rows=''.join('<tr>'+''.join('<td>'+e(v)+'</td>' for v in [r['name'],
        *[int(p['period_frames']) for p in r['octave_diagnostic']['periods']],
        f"{r['octave_diagnostic']['first_to_second_ratio']:.5f}",
        f"{r['octave_diagnostic']['second_to_third_ratio']:.5f}"] )+'</tr>'
        for r in rows if 'octave_diagnostic' in r)
    report=f'''<!doctype html><html lang="ru"><meta charset="utf-8"><title>KS waves — подготовка ROM</title>
<style>body{{font:16px/1.55 system-ui,sans-serif;max-width:1280px;margin:40px auto;padding:0 24px;color:#182333;background:#f8fafc}}h1,h2{{line-height:1.2}}table{{border-collapse:collapse;width:100%;font-size:13px;background:white}}th,td{{padding:8px;border:1px solid #dce2e9;text-align:left}}th{{background:#e6edf5}}img{{width:100%;height:auto}}code{{background:#e6edf5;padding:2px 5px}}.note{{background:#fff0d6;padding:20px;border-left:4px solid #dc941d}}a{{color:#145ea8}}</style>
<h1>KS: подготовленный набор волн</h1><p>14 октября 2026 · источник: пользовательские записи с KS · 32/32 волн · PCM24, stereo, 48 кГц.</p>
<p>Набор разобран по номерам, разделён на C1/C2/C3 и подготовлен как исходный материал для подменного банка.
Оригиналы сохранены без изменений. Пакет содержит 32 отсортированных оригинала, 96 stereo-фрагментов,
96 моно-фрагментов с исходным уровнем, 96 моно-фрагментов с пиками −3 dBFS и 28 кандидатов петель в двух вариантах уровня.</p>
<p class="note"><b>Статус: подготовленные исходники, ещё не совместимый BIN-ROM.</b>
В коде Silver-K подтверждены размер банка 129024 байта и диапазон H8 0x860000–0x87F7FF.
Этого недостаточно для упаковки: нужны адреса и длины слотов, кодирование/знаковость/порядок байтов,
масштаб амплитуды, исходная частота и схема воспроизведения из прошивки. WAV является записью выхода KS,
а не извлечёнными байтами Wave ROM. Произвольное объединение WAV не даст работоспособный банк.</p>
<h2>Сверка с мануалом</h2><p>Таблица Oscillator Waveforms, печатная стр. 23 (страница PDF 23),
<a href="{MANUAL}">KS Rack User Guide</a> и <a href="{KEYBOARD_MANUAL}">KS 4/5 User Guide</a>:
номера 0–31 и все названия совпали. Пропущенных или повторных номеров нет.</p>
<p>0–3 — традиционные формы; 4–7 — шумовые варианты; 8–16 — дополнительные тональные формы;
17–31 — Rhythm Sounds. Список пользовательских файлов — список выбора осциллятора, а не доказанная адресная карта ROM.
Мануал описывает форму и параметры синтеза, но не внутренний бинарный формат Wave ROM.
В новых именах унифицированы регистр, дефисы и разделители; сохранены даже написания Harpiscord и Tamborine из мануала.</p>
<h2>Что сделано со звуком</h2><ul>
<li><b>Основной вариант:</b> левый канал, исходный gain, PCM24 / 48 кГц. Без эквалайзера, компрессии, шумоподавления и изменения высоты.</li>
<li>Три nominal-ноты на файл. Сетка по длительностям и паузам: примерно 55 BPM, слот 1.090909 с.
У Snare 1 первый слот удвоен; у Cymbal первый утроен, второй удвоен. Stereo-разрезы сохраняют каждый кадр оригинала.</li>
<li>В моно-фрагментах удалена только крайняя тишина ниже −70 dBFS с запасом 5 мс. Внутренние хвосты сохранены; транзиенты не сглажены.</li>
<li>Каналы почти совпадают после сдвига: для большинства лучший lag 1 кадр, для Sine, Tri-Angle и Electric Piano — 2.
Есть также разница уровней каналов. На шумах прямое суммирование без выравнивания даёт частотные потери;
поэтому сохранён один канал и доступны stereo-исходники.</li>
<li><b>Отдельный вариант peak −3 dBFS:</b> один постоянный множитель на волну для всех трёх нот.
Он удобен для сравнения, но меняет соотношение громкости разных волн. Для ROM по умолчанию использовать исходные уровни.</li>
<li><b>Кандидаты петель:</b> один измеренный повтор из C1, выбор стыка по значениям и наклону,
линейная интерполяция дробной границы и вычитание DC. Без усреднения повторов, crossfade и обрезки спектра.
Размеры петель относятся к 48 кГц записи и не являются доказанными длинами слотов ROM.</li>
</ul>
<h2>Уровни и качество</h2><p>Пики оригиналов: от {min(r['source_peak_dbfs'] for r in rows):.2f} до {max(r['source_peak_dbfs'] for r in rows):.2f} dBFS.
Цифровой клиппинг не обнаружен. Разница RMS всего файла достигает
{max(r['left_rms_dbfs'] for r in rows)-min(r['left_rms_dbfs'] for r in rows):.2f} dB: это разные формы и заполнение сигнала,
а не основание автоматически делать их одинаковыми по RMS. Такая нормализация чрезмерно усилила бы короткие ударные.
Большие различия L/R без сдвига объясняются преимущественно временным смещением, а не независимым stereo-содержимым.</p>
<img src="levels.png" alt="Peak и RMS 32 записей"><p>RMS здесь рассчитан по всему левому каналу, включая паузы; это диагностический показатель, не LUFS и не уровень чистой волны в ROM.</p>
<h2>Периодичность и ограничения</h2><p>Тональные формы показывают стабильные повторы на C1 около 1467.5 кадров (около 32.7 Гц).
Ударные в этих записях многократно повторяются внутри удержанной ноты: нельзя считать весь WAV одним ударом.
Из них выделен повтор целиком с его внутренним затуханием. Формы 0–3 служат контрольными;
4–7 сохранены как шумовые записи без искусственно назначенных петель. Для тональных и ударных слотов в первую очередь интересны 8–31.
Зависимость шума 4–7 от банка также нужно проверить: текущий zero-bank adapter Silver-K заглушает все выборы выше 3,
поэтому по одному лишь названию нельзя считать шумовые формы независимыми от ROM.</p>
<p>Корреляция повторов зависит от интерполяции и фильтрации выхода. Для Closed Hi-Hat, Tamborine и Cymbal она ниже,
поэтому их петли обязательно требуют прослушивания и сравнения с KS; период Closed Hi-Hat особенно предварительный.
Порог/корреляция не доказывают побитовую идентичность с внутренним ROM.</p>
<p>Пользователь подтвердил C1/C2/C3 и нулевые настройки без огибающих. Для отдельных файлов Snare 1 и Cymbal
предварительные периоды во второй/третьей части показывают отношения, отличающиеся от ровных октав
(около 1.888 вместо 2 между первой и второй частями). Возможны другая фактическая нота или зависимость
воспроизведения от клавиши; ноты не переименованы и высота не исправлена. Перед упаковкой проверить MIDI-ноты
по исходной сессии и процедуру pitch scaling в прошивке. Velocity и точный gain тракта записи в данных не задокументированы.</p>
<table><tr><th>Форма</th><th>C1: кадры</th><th>C2: кадры</th><th>C3: кадры</th><th>C1/C2</th><th>C2/C3</th></tr>{octave_rows}</table>
<p>В контрольном Square заметен спад плоских участков за полупериод: запись уже содержит низкочастотную
окраску выходного/записывающего тракта. Поэтому даже отсутствие эффектов и огибающих не делает запись
точной копией внутренних отсчётов. Обратная коррекция фильтра автоматически не применялась;
её нужно калибровать по контрольным формам и прошивке, чтобы не исказить sampled-волны.</p>
<h2>Каталог</h2><p>Gain — только для дополнительного варианта peak −3 dBFS; Corr — Pearson при ближайшем целом периоде.</p>
<table><thead><tr><th>ID</th><th>Название по мануалу</th><th>Группа</th><th>Длина, с</th><th>Peak, dBFS</th><th>RMS L, dBFS</th><th>Gain, dB</th><th>Период C1, кадров</th><th>Corr</th></tr></thead><tbody>{table}</tbody></table>
<h2>Огибающие записей</h2><p>RMS в окнах 10 мс; оранжевые линии — границы фрагментов C1/C2/C3.</p><img src="envelopes.png" alt="Огибающие всех записей">
<h2>Кандидаты циклов</h2><img src="loops.png" alt="28 измеренных циклов">
<h2>Файлы пакета и проверка</h2><ul>
<li><code>originals_sorted/</code> — оригинальные WAV с номерами 00–31, по группам; SHA-256 совпадает.</li>
<li><code>notes_stereo/</code> — полный stereo-разрез на 3 номинальные ноты без потерь.</li>
<li><code>notes_mono_preserved/</code> — основной подготовленный материал, 96 файлов.</li>
<li><code>notes_mono_peak_minus3/</code> — дополнительный набор для сравнения уровней.</li>
<li><code>loop_candidates_48k/</code> и <code>loop_candidates_peak_minus3/</code> — 28 петель: 4 эталона + 24 sampled/rhythm.</li>
<li><a href="manifest.json">manifest.json</a> — исходные SHA, границы каждого фрагмента, параметры и обработка петель.</li>
<li><a href="catalog.csv">catalog.csv</a> — сортируемая таблица;
<a href="verification.json">verification.json</a> — результаты проверки;
<a href="SHA256SUMS.txt">SHA256SUMS.txt</a> — контрольные суммы всего пакета.</li>
<li><code>prepare_ks_waves.py</code> — воспроизводимый генератор; зависимости numpy и matplotlib.</li></ul>
<p>Проверки: 32 оригинальные копии побитово совпали; сборка stereo-фрагментов восстановила каждый оригинал;
96 моно-фрагментов побитово совпали с соответствующим левым каналом; все экспортированные WAV проверены
на формат, непустые данные и отсутствие цифрового клиппинга. Прослушивание и запуск банка в эмуляторе не выполнены.</p>
<p>Следующий этап сборки ROM: получить из поддержанной KS OS карту чтения дополнительных и шумовых форм,
подтвердить формат каждого блока, восстановить требуемые циклы/отсчёты и масштаб относительно
контрольной sine, упаковать банк ровно 129024 байта, затем проверить все 32 выбора и несколько нот в Silver-K против KS.</p>
</html>'''
    (output/'report.html').write_text(report,encoding='utf-8')


if __name__ == '__main__':
    main()
