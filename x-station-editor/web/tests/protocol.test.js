import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import parameters from '../src/parameters.json' with { type: 'json' };
import { parseSysEx, serializeSysEx, getName, setName, prepareUpload, readParameter,
  writeParameter, midiMessages, readControls, updateControl, readTemplateSettings,
  updateTemplateSettings, SysExAssembler } from '../src/protocol.js';

function synth() {
  const bytes = new Uint8Array(270);
  bytes.set([0xf0, 0, 0x20, 0x29, 1, 0x42, 0x7f, 1, 0, 0x10, 0, 2, 37]);
  for (let i = 13; i < 269; i++) bytes[i] = (i * 11) & 127;
  bytes[269] = 0xf7;
  return parseSysEx(bytes)[0];
}
function template() {
  const bytes = new Uint8Array(8208);
  bytes.set([0xf0, 0, 0x20, 0x29, 2, 0, 0x7f, 0, 0, 8, 10, 0, 28]);
  for (let i = 13; i < 8205; i++) bytes[i] = (i * 3) & 127;
  bytes.set([0x12, 0x34, 0xf7], 8205);
  for (let i = 0; i < 150; i++) {
    const offset = 405 + i * 52;
    bytes[offset + 16] = 1;
    bytes[offset + 27] = 1; bytes[offset + 28] = 0;
  }
  return parseSysEx(bytes)[0];
}
function changed(before, after) {
  return Array.from(before.keys()).filter(i => before[i] !== after[i]);
}
test('concatenated dumps roundtrip without normalizing reserved bytes', () => {
  const a = synth(), b = template(), bytes = serializeSysEx([a, b]);
  const decoded = parseSysEx(bytes);
  assert.deepEqual(decoded.map(d => d.kind), ['synth', 'template']);
  assert.deepEqual(serializeSysEx(decoded), bytes);
  bytes[164] = 65;
  assert.notEqual(decoded[0].bytes[164], 65, 'parsed records own their bytes');
});
test('reject firmware, foreign, invalid data, garbage and truncated packets', () => {
  const firmware = synth().bytes.slice(); firmware[4] = 2; firmware[5] = 1; firmware[7] = 0x7e;
  assert.throws(() => parseSysEx(firmware), /Unsupported/);
  const data = synth().bytes.slice(); data[30] = 0x80;
  assert.throws(() => parseSysEx(data), /SysEx data/);
  assert.throws(() => parseSysEx(synth().bytes.slice(0, -1)), /Truncated/);
  assert.throws(() => parseSysEx([0, ...synth().bytes]), /start/);
  assert.throws(() => parseSysEx([]), /Empty/);
  assert.throws(() => parseSysEx([256]), /Byte/);
});
test('name editing changes only its 16-byte field; validation is atomic', () => {
  for (const dump of [synth(), template()]) {
    const before = dump.bytes.slice(), offset = dump.kind === 'synth' ? 164 : 13;
    setName(dump, 'NEW PATCH');
    assert.equal(getName(dump), 'NEW PATCH'); assert.equal(dump.name, 'NEW PATCH');
    assert.ok(changed(before, dump.bytes).every(i => i >= offset && i < offset + 16));
    assert.deepEqual(dump.bytes.subarray(offset + 9, offset + 16), new Uint8Array(7).fill(32));
    const valid = dump.bytes.slice();
    assert.throws(() => setName(dump, 'АБВ'), /ASCII/);
    assert.throws(() => setName(dump, 'x'.repeat(17)), /ASCII/);
    assert.deepEqual(dump.bytes, valid);
  }
});
test('upload targets synth edit buffer without mutating source or payload tails', () => {
  const dump = synth(), before = dump.bytes.slice(), upload = prepareUpload(dump);
  assert.equal(upload[7], 0); assert.equal(upload[11], 0); assert.equal(upload[12], 0);
  assert.deepEqual(upload.subarray(13), before.subarray(13));
  assert.deepEqual(dump.bytes, before);
  assert.deepEqual(prepareUpload(template()), template().bytes);
});
test('packed parameter editing preserves neighbors, tail and unrelated bytes', () => {
  const dump = synth(), p = parameters.find(p => p.id === 'voice_mode');
  dump.bytes[13] = 0x65; const before = dump.bytes.slice();
  writeParameter(dump, p, 2);
  assert.equal(readParameter(dump, p), 2);
  assert.equal(dump.bytes[13] & ~p.mask, before[13] & ~p.mask);
  assert.ok(changed(before, dump.bytes).every(i => i === 13));
  assert.deepEqual(dump.bytes.subarray(13 + 209), before.subarray(13 + 209));
  const valid = dump.bytes.slice();
  for (const value of [-1, 4, 1.5, NaN]) assert.throws(() => writeParameter(dump, p, value), /integer/);
  assert.throws(() => writeParameter(dump, parameters.find(p => p.id === 'modulation'), 4), /not stored/);
  assert.deepEqual(dump.bytes, valid);
});
test('documented MIDI messages use channel1..16 and atomic NRPN base encoding', () => {
  const arp = parameters.find(p => p.id === 'arp_on'), cutoff = parameters.find(p => p.id === 'cutoff');
  assert.deepEqual(midiMessages(arp, 1, 16), [[0xbf, 99, 0], [0xbf, 98, 104], [0xbf, 6, 29]]);
  assert.deepEqual(midiMessages(cutoff, 73, 1), [[0xb0, 105, 73]]);
  assert.throws(() => midiMessages(cutoff, 73, 0), /channel/);
  assert.throws(() => midiMessages(cutoff, 128, 1), /integer/);
});
test('template records expose150 controls and preserve channel high nibble and flags', () => {
  const dump = template(), rows = readControls(dump);
  assert.equal(rows.length, 150); assert.equal(rows[0].physicalKind, 'pot'); assert.equal(rows[1].physicalKind, 'button');
  const offset = 405 + 52, before = dump.bytes.slice();
  updateControl(dump, 1, { name: 'Pedal', type: 2, bank: 0, number: 104, low: 29, high: 28,
    channel: 16, default: 29, buttonMode: 3, step: 64 });
  const row = readControls(dump)[1];
  assert.equal(row.low, 29); assert.equal(row.high, 28, 'reverse ranges allowed');
  assert.equal(row.channel, 16); assert.equal(row.buttonMode, 3); assert.equal(row.step, 64);
  assert.equal(dump.bytes[offset + 24] & 0x70, before[offset + 24] & 0x70);
  assert.equal(dump.bytes[offset + 19] & ~12, before[offset + 19] & ~12);
  assert.equal(dump.bytes[offset + 20] & ~32, before[offset + 20] & ~32);
  assert.ok(changed(before, dump.bytes).every(i => i >= offset && i < offset + 52));
  const valid = dump.bytes.slice();
  assert.throws(() => updateControl(dump, 1, { low: 64, channel: 17 }), /channel/);
  assert.throws(() => updateControl(dump, 0, { buttonMode: 1 }), /buttons/);
  assert.throws(() => updateControl(dump, 1, { step: 0 }), /Step/);
  assert.throws(() => updateControl(dump, 150, {}), /index/);
  assert.deepEqual(dump.bytes, valid);
});
test('button modes clear only their action masks; MMC device ID uses whole byte', () => {
  const dump = template(), offset = 457;
  for (const mode of [0, 1, 2, 3]) {
    updateControl(dump, 1, { buttonMode: mode });
    assert.equal(readControls(dump)[1].buttonMode, mode);
  }
  updateControl(dump, 1, { type: 5, channel: 127, default: 6 });
  assert.equal(dump.bytes[offset + 24], 127); assert.equal(readControls(dump)[1].channel, 127);
  updateControl(dump, 1, { type: 6, high: 50 });
  assert.equal(dump.bytes[offset + 18], 0);
  dump.bytes[offset + 16] = 7;
  updateControl(dump, 1, { name: 'Legacy' });
  assert.equal(dump.bytes[offset + 16], 7, 'untouched legacy type is preserved');
});
test('SysEx DV metadata, literal7F, unused payload and metadata byte26 are preserved', () => {
  const dump = template(), offset = 405, before = dump.bytes.slice();
  updateControl(dump, 0, { sysex: '42 30 7F DV 00' });
  assert.equal(readControls(dump)[0].sysex, '42 30 7F DV 00');
  assert.equal(dump.bytes[offset + 27], 6); assert.equal(dump.bytes[offset + 28], 4);
  assert.equal(dump.bytes[offset + 26], before[offset + 26]);
  assert.deepEqual(dump.bytes.subarray(offset + 34, offset + 47), before.subarray(offset + 34, offset + 47));
  const valid = dump.bytes.slice();
  for (const text of ['F0 00', 'DV DV', 'A', '', Array(19).fill('00').join(' ')])
    assert.throws(() => updateControl(dump, 0, { sysex: text }));
  assert.deepEqual(dump.bytes, valid);
});
test('template settings invert synth flag without changing neighboring flag bits', () => {
  const dump = template(), before = dump.bytes.slice();
  updateTemplateSettings(dump, { manufacturer: 'Novation', synthTemplate: true, overrideChannel: 16 });
  const settings = readTemplateSettings(dump);
  assert.equal(settings.manufacturer, 'Novation'); assert.equal(settings.synthTemplate, true);
  assert.equal(settings.overrideChannel, 16); assert.equal(dump.bytes[49] & ~2, before[49] & ~2);
  assert.ok(changed(before, dump.bytes).every(i => (i >= 30 && i <= 42) || i === 49 || i === 50));
  updateTemplateSettings(dump, { synthTemplate: false }); assert.equal(readTemplateSettings(dump).synthTemplate, false);
  const valid = dump.bytes.slice();
  assert.throws(() => updateTemplateSettings(dump, { manufacturer: 'OK', overrideChannel: 17 }));
  assert.deepEqual(dump.bytes, valid);
});
test('incoming assembler handles fragments, realtime MIDI and repeated frames', () => {
  const assembler = new SysExAssembler(), dump = synth();
  assert.deepEqual(assembler.push([0x90, 60, 100]), []);
  assert.deepEqual(assembler.push(dump.bytes.subarray(0, 80)), []);
  assert.deepEqual(assembler.push([0xf8, 0xfe]), []);
  const output = assembler.push(dump.bytes.subarray(80));
  assert.deepEqual(output[0].bytes, dump.bytes);
  assert.equal(assembler.push(serializeSysEx([dump, dump])).length, 2);
  assert.throws(() => assembler.push([0xf0, 0, 0x90]), /status/);
  assert.deepEqual(assembler.push([0xf7]), []);
});

function fixtures(directory) {
  const result = [];
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const location = path.join(directory, entry.name);
    if (entry.isDirectory()) result.push(...fixtures(location));
    else if (/\.syx$/i.test(entry.name)) result.push(location);
  }
  return result;
}
test('supplied official fixtures roundtrip and support narrow name edits', { skip: !process.env.XSTATION_FIXTURES }, () => {
  let synthCount = 0, templateCount = 0, bankCount = 0;
  for (const filename of fixtures(process.env.XSTATION_FIXTURES)) {
    const bytes = new Uint8Array(fs.readFileSync(filename));
    if (!([270, 27000, 8208, 16416].includes(bytes.length))) continue;
    const dumps = parseSysEx(bytes);
    assert.deepEqual(serializeSysEx(dumps), bytes, filename);
    if (bytes.length === 27000) bankCount++;
    for (const dump of dumps) {
      dump.kind === 'synth' ? synthCount++ : templateCount++;
      const before = dump.bytes.slice(), offset = dump.kind === 'synth' ? 164 : 13;
      setName(dump, 'FIXTURE TEST');
      assert.ok(changed(before, dump.bytes).every(i => i >= offset && i < offset + 16), filename);
      if (dump.kind === 'template') assert.equal(readControls(dump).length, 150);
      else for (const parameter of parameters.filter(p => p.offset !== null)) {
        const value = readParameter(dump, parameter);
        assert.ok(value >= parameter.min && value <= parameter.max, `${filename}: ${parameter.id}`);
      }
    }
  }
  assert.ok(synthCount >= 100); assert.ok(templateCount >= 40); assert.ok(bankCount >= 1);
});
