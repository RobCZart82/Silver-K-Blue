import controlKinds from './control-kinds.json' with { type: 'json' };

const SYNTH_HEADER = [0xf0, 0, 0x20, 0x29, 1, 0x42, 0x7f];
const TEMPLATE_HEADER = [0xf0, 0, 0x20, 0x29, 2, 0, 0x7f];
const RECORD_START = 405, RECORD_SIZE = 52, CONTROL_COUNT = 150;

function integer(value, min, max, label) {
  if (!Number.isInteger(value) || value < min || value > max)
    throw new RangeError(`${label} must be an integer from ${min} to ${max}`);
  return value;
}
function dataBytes(input) {
  if (!(input instanceof Uint8Array) && !Array.isArray(input))
    throw new TypeError('Expected a Uint8Array or byte array');
  for (const value of input) integer(value, 0, 255, 'Byte');
  return Uint8Array.from(input);
}
function matches(bytes, prefix) { return prefix.every((value, i) => bytes[i] === value); }
function classify(bytes) {
  if (bytes[0] !== 0xf0 || bytes.at(-1) !== 0xf7)
    throw new Error('Incomplete SysEx frame');
  for (let i = 1; i < bytes.length - 1; i++)
    integer(bytes[i], 0, 127, 'SysEx data byte');
  if (bytes.length === 270 && matches(bytes, SYNTH_HEADER) &&
      (bytes[7] === 0 || bytes[7] === 1) && bytes[8] === 0 &&
      bytes[9] === 0x10 && bytes[10] === 0) return 'synth';
  if (bytes.length === 8208 && matches(bytes, TEMPLATE_HEADER) &&
      bytes[7] === 0 && bytes[8] === 0 &&
      bytes[8205] === 0x12 && bytes[8206] === 0x34) return 'template';
  throw new Error('Unsupported SysEx packet. Firmware and other device formats are not accepted.');
}
function requireDump(dump, kind) {
  if (!dump || !(dump.bytes instanceof Uint8Array)) throw new TypeError('Expected a parsed dump');
  const actual = classify(dump.bytes);
  if (dump.kind !== actual || (kind && actual !== kind)) throw new Error(`Expected ${kind || actual} dump`);
  return dump.bytes;
}
function readText(bytes, offset, length) {
  return String.fromCharCode(...bytes.subarray(offset, offset + length)).replace(/\0.*$/, '').trimEnd();
}
function textBytes(value, length, label) {
  if (typeof value !== 'string' || value.length > length || /[^\x20-\x7e]/.test(value))
    throw new RangeError(`${label} must contain at most ${length} printable ASCII characters`);
  const bytes = new Uint8Array(length).fill(32);
  for (let i = 0; i < value.length; i++) bytes[i] = value.charCodeAt(i);
  return bytes;
}

export function parseSysEx(input) {
  const bytes = dataBytes(input), dumps = [];
  let start = 0;
  while (start < bytes.length) {
    if (bytes[start] !== 0xf0) throw new Error(`Expected SysEx start at byte ${start}`);
    const end = bytes.indexOf(0xf7, start + 1);
    if (end < 0) throw new Error('Truncated SysEx frame');
    const frame = bytes.slice(start, end + 1), kind = classify(frame);
    const dump = { kind, bytes: frame, name: '' };
    dump.name = getName(dump); dumps.push(dump); start = end + 1;
  }
  if (!dumps.length) throw new Error('Empty SysEx file');
  return dumps;
}
export function getName(dump) {
  const bytes = requireDump(dump);
  return readText(bytes, dump.kind === 'synth' ? 164 : 13, 16);
}
export function setName(dump, value) {
  const bytes = requireDump(dump), encoded = textBytes(value, 16, 'Name');
  bytes.set(encoded, dump.kind === 'synth' ? 164 : 13);
  dump.name = getName(dump); return dump;
}
export function serializeSysEx(dumps) {
  if (!Array.isArray(dumps) || !dumps.length) throw new Error('No dumps to serialize');
  for (const dump of dumps) requireDump(dump);
  const output = new Uint8Array(dumps.reduce((size, dump) => size + dump.bytes.length, 0));
  let position = 0;
  for (const dump of dumps) { output.set(dump.bytes, position); position += dump.bytes.length; }
  return output;
}
export function prepareUpload(dump) {
  const bytes = requireDump(dump).slice();
  if (dump.kind === 'synth') { bytes[7] = 0; bytes[11] = 0; bytes[12] = 0; }
  return bytes;
}

function parameterValue(parameter, value) {
  return integer(value, parameter.min, parameter.max, parameter.name || 'Parameter');
}
function parameterLocation(parameter) {
  if (parameter.offset === null || parameter.offset === undefined) throw new Error('This live control is not stored in a patch');
  integer(parameter.offset, 0, 255, 'Parameter offset');
  integer(parameter.mask, 1, 127, 'Parameter mask');
  integer(parameter.shift, 0, 6, 'Parameter shift');
  return 13 + parameter.offset;
}
export function readParameter(dump, parameter) {
  const bytes = requireDump(dump, 'synth');
  return (bytes[parameterLocation(parameter)] & parameter.mask) >>> parameter.shift;
}
export function writeParameter(dump, parameter, value) {
  const bytes = requireDump(dump, 'synth'), location = parameterLocation(parameter);
  parameterValue(parameter, value);
  const encoded = value << parameter.shift;
  if ((encoded & parameter.mask) !== encoded) throw new RangeError('Value does not fit parameter mask');
  bytes[location] = (bytes[location] & ~parameter.mask) | encoded;
  return dump;
}
export function midiMessages(parameter, value, channel = 1) {
  integer(channel, 1, 16, 'MIDI channel'); parameterValue(parameter, value);
  integer(parameter.number, 0, 127, 'Controller number');
  const status = 0xb0 + channel - 1, wireValue = value + (parameter.base || 0);
  integer(wireValue, 0, 127, 'MIDI value');
  if (parameter.kind === 'CC') return [[status, parameter.number, wireValue]];
  if (parameter.kind === 'NRPN') return [[status, 99, 0], [status, 98, parameter.number], [status, 6, wireValue]];
  throw new Error('Unsupported parameter MIDI mapping');
}

function controlPosition(dump, index) {
  requireDump(dump, 'template'); integer(index, 0, CONTROL_COUNT - 1, 'Control index');
  return RECORD_START + RECORD_SIZE * index;
}
function sysexText(bytes, offset) {
  const length = Math.max(0, Math.min(18, bytes[offset + 27] - 1)), dv = bytes[offset + 28];
  return Array.from(bytes.subarray(offset + 29, offset + 29 + length), (value, i) =>
    dv === i + 1 ? 'DV' : value.toString(16).padStart(2, '0').toUpperCase()).join(' ');
}
function encodeSysex(text) {
  if (typeof text !== 'string') throw new TypeError('SysEx must be hexadecimal text');
  const tokens = text.trim().split(/\s+/);
  if (!text.trim() || tokens.length > 18) throw new RangeError('SysEx must contain from 1 to 18 data tokens');
  let dv = 0;
  const bytes = tokens.map((token, i) => {
    if (token.toUpperCase() === 'DV') {
      if (dv) throw new Error('SysEx may contain only one DV token');
      dv = i + 1; return 127;
    }
    if (!/^[0-9a-f]{2}$/i.test(token)) throw new Error('SysEx tokens must be two-digit hex or DV');
    return integer(parseInt(token, 16), 0, 127, 'SysEx token');
  });
  return { bytes, dv };
}
export function readControls(dump) {
  const bytes = requireDump(dump, 'template');
  return Array.from({ length: CONTROL_COUNT }, (_, index) => {
    const offset = RECORD_START + RECORD_SIZE * index, isButton = controlKinds[index] === 0;
    const buttonMode = bytes[offset + 20] & 32 ? 3 : bytes[offset + 19] & 8 ? 2 : bytes[offset + 19] & 4 ? 1 : 0;
    return { index, physicalKind: isButton ? 'button' : 'pot', isButton,
      name: readText(bytes, offset, 16), type: bytes[offset + 16],
      low: bytes[offset + 17], high: bytes[offset + 18],
      bank: bytes[offset + 22], number: bytes[offset + 23],
      channel: bytes[offset + 16] === 5 ? bytes[offset + 24] : (bytes[offset + 24] & 15) + 1,
      default: bytes[offset + 25], sysex: sysexText(bytes, offset),
      buttonMode, step: bytes[offset + 47] + 1 };
  });
}
export function updateControl(dump, index, changes) {
  const offset = controlPosition(dump, index), bytes = dump.bytes;
  if (!changes || typeof changes !== 'object') throw new TypeError('Expected control changes');
  const known = new Set(['name', 'type', 'low', 'high', 'number', 'bank', 'channel', 'default', 'sysex', 'buttonMode', 'step']);
  for (const key of Object.keys(changes)) if (!known.has(key)) throw new Error(`Unknown control field: ${key}`);
  const next = bytes.slice(offset, offset + RECORD_SIZE), isButton = controlKinds[index] === 0;
  if ('name' in changes) next.set(textBytes(changes.name, 16, 'Control name'), 0);
  if ('type' in changes) { integer(changes.type, 0, 12, 'Control type'); next[16] = changes.type === 7 ? 6 : changes.type; }
  for (const [key, position] of [['low', 17], ['high', 18], ['bank', 22], ['number', 23], ['default', 25]])
    if (key in changes) next[position] = integer(changes[key], 0, 127, key);
  if ('channel' in changes) {
    if (next[16] === 5) next[24] = integer(changes.channel, 0, 127, 'MMC device ID');
    else next[24] = (next[24] & 0x70) | (integer(changes.channel, 1, 16, 'Control channel') - 1);
  }
  if ('sysex' in changes) {
    const encoded = encodeSysex(changes.sysex);
    next[27] = encoded.bytes.length + 1; next[28] = encoded.dv; next.set(encoded.bytes, 29);
  }
  if ('buttonMode' in changes) {
    if (!isButton) throw new Error('Button mode applies only to physical buttons');
    integer(changes.buttonMode, 0, 3, 'Button mode'); next[19] &= ~12; next[20] &= ~32;
    if (changes.buttonMode === 1) next[19] |= 4;
    if (changes.buttonMode === 2) next[19] |= 8;
    if (changes.buttonMode === 3) next[20] |= 32;
  }
  if ('step' in changes) {
    if (!isButton) throw new Error('Step applies only to physical buttons');
    next[47] = integer(changes.step, 1, 64, 'Step') - 1;
  }
  if ('type' in changes && next[16] === 6 && isButton) next[18] = 0;
  bytes.set(next, offset); return dump;
}
export function readTemplateSettings(dump) {
  const bytes = requireDump(dump, 'template');
  return { name: getName(dump), manufacturer: readText(bytes, 30, 13),
    synthTemplate: (bytes[49] & 2) === 0, overrideChannel: bytes[50] };
}
export function updateTemplateSettings(dump, changes) {
  const bytes = requireDump(dump, 'template'), next = bytes.slice();
  const known = new Set(['name', 'manufacturer', 'synthTemplate', 'overrideChannel']);
  for (const key of Object.keys(changes)) if (!known.has(key)) throw new Error(`Unknown template setting: ${key}`);
  if ('name' in changes) next.set(textBytes(changes.name, 16, 'Name'), 13);
  if ('manufacturer' in changes) next.set(textBytes(changes.manufacturer, 13, 'Manufacturer'), 30);
  if ('synthTemplate' in changes) {
    if (typeof changes.synthTemplate !== 'boolean') throw new TypeError('Synth template must be a boolean');
    next[49] = changes.synthTemplate ? next[49] & ~2 : next[49] | 2;
  }
  if ('overrideChannel' in changes) next[50] = integer(changes.overrideChannel, 0, 16, 'Override channel');
  bytes.set(next); dump.name = getName(dump); return dump;
}

// Web MIDI may deliver one frame over several events, with realtime bytes interleaved.
export class SysExAssembler {
  constructor() { this.pending = []; }
  reset() { this.pending = []; }
  push(input) {
    const output = [];
    for (const byte of dataBytes(input)) {
      if (byte >= 0xf8) continue;
      if (byte === 0xf0) { this.pending = [byte]; continue; }
      if (!this.pending.length) continue;
      if (byte >= 0x80 && byte !== 0xf7) { this.reset(); throw new Error('Unexpected MIDI status within SysEx'); }
      this.pending.push(byte);
      if (this.pending.length > 8208) { this.reset(); throw new Error('SysEx frame exceeds supported size'); }
      if (byte === 0xf7) { const frame = this.pending; this.reset(); output.push(...parseSysEx(frame)); }
    }
    return output;
  }
}
