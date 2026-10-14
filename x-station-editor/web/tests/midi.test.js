import test from 'node:test';
import assert from 'node:assert/strict';
import { MidiConnection, ParameterDecoder } from '../src/midi.js';
import { parseSysEx } from '../src/protocol.js';
import parameters from '../src/parameters.json' with { type: 'json' };

const cutoff = parameters.find(p => p.id === 'cutoff');
const arpOn = parameters.find(p => p.id === 'arp_on');

function fakePort(id, type) {
  return {
    id, name: `X-Station ${type}`, type, state: 'connected', connection: 'closed',
    onmidimessage: null, opened: 0, closed: 0, sent: [],
    async open() { this.opened++; this.connection = 'open'; return this; },
    async close() { this.closed++; this.connection = 'closed'; return this; },
    send(bytes, time) {
      assert.equal(this.state, 'connected', 'cannot send to a disconnected output');
      assert.equal(this.connection, 'open', 'cannot send to a closed output');
      this.sent.push({ bytes: Array.from(bytes), time });
    },
  };
}
function environment(t, { sysexEnabled = true, secure = true } = {}) {
  const input = fakePort('input', 'input'), output = fakePort('output', 'output');
  const access = { sysexEnabled, inputs: new Map([[input.id, input]]),
    outputs: new Map([[output.id, output]]), onstatechange: null };
  const requests = [], states = [], messages = [], errors = [];
  for (const [key, value] of Object.entries({
    navigator: { async requestMIDIAccess(options) { requests.push(options); return access; } },
    isSecureContext: secure,
  })) {
    const descriptor = Object.getOwnPropertyDescriptor(globalThis, key);
    Object.defineProperty(globalThis, key, { configurable: true, value });
    t.after(() => descriptor ? Object.defineProperty(globalThis, key, descriptor) : delete globalThis[key]);
  }
  const midi = new MidiConnection({ onState: state => states.push(state),
    onMessage: message => messages.push(message), onError: error => errors.push(error) });
  t.after(() => midi.clearPending());
  return { midi, access, input, output, requests, states, messages, errors };
}
function storedPatch() {
  const bytes = new Uint8Array(270);
  bytes.set([0xf0, 0, 0x20, 0x29, 1, 0x42, 0x7f, 1, 0, 0x10, 0, 2, 99]);
  for (let i = 13; i < 269; i++) bytes[i] = i & 127;
  bytes[269] = 0xf7;
  return parseSysEx(bytes)[0];
}
const sentBytes = output => output.sent.map(message => message.bytes);

test('legacy packed CC updates decode independent fields and ignore other channels', () => {
  const packed = [
    { id: 'unison', kind: 'CC', number: 67, min: 0, max: 7, wireMask: 7, wireShift: 0 },
    { id: 'voice', kind: 'CC', number: 67, min: 0, max: 3, wireMask: 24, wireShift: 3 },
    { id: 'slope', kind: 'CC', number: 67, min: 0, max: 1, wireMask: 32, wireShift: 5 },
  ];
  const decoder = new ParameterDecoder(packed);
  assert.deepEqual(decoder.decode([0xb5, 67, 55], 6).map(([p, v]) => [p.id, v]),
    [['unison', 7], ['voice', 2], ['slope', 1]]);
  assert.deepEqual(decoder.decode([0xb0, 67, 55], 6), []);
  assert.deepEqual(decoder.decode([0x95, 67, 55], 6), []);
});
test('NRPN alternatives select only matching base and reject unselected/foreign-bank data', () => {
  const decoder = new ParameterDecoder(parameters);
  const send = (number, value, channel = 1) => decoder.decode([0xb0 + channel - 1, number, value], channel);
  assert.deepEqual(send(6, 29), []);
  send(99, 0); send(98, 104);
  assert.deepEqual(send(6, 29).map(([p, v]) => [p.id, v]), [['arp_on', 1]]);
  assert.deepEqual(send(6, 28).map(([p, v]) => [p.id, v]), [['arp_on', 0]]);
  assert.deepEqual(send(6, 127), []);
  assert.deepEqual(send(6, 29, 2), [], 'selector state is independent for every MIDI channel');
  send(99, 1); assert.deepEqual(send(6, 29), []);
  send(99, 0);
  assert.equal(send(6, 29)[0][0], arpOn);
});
test('RPN selectors cancel NRPN interpretation until a new NRPN LSB arrives', () => {
  for (const rpn of [100, 101]) {
    const decoder = new ParameterDecoder(parameters);
    decoder.decode([0xb0, 98, 104], 1);
    assert.equal(decoder.decode([0xb0, 6, 29], 1).length, 1);
    decoder.decode([0xb0, rpn, 0], 1);
    assert.deepEqual(decoder.decode([0xb0, 6, 29], 1), []);
    decoder.decode([0xb0, 98, 104], 1);
    assert.equal(decoder.decode([0xb0, 6, 29], 1).length, 1);
  }
});
test('Web MIDI request explicitly asks for SysEx and rejects insecure or denied contexts', async t => {
  const env = environment(t);
  await env.midi.request();
  assert.deepEqual(env.requests, [{ sysex: true }]);
  assert.equal(env.states.at(-1).connected, false);
  env.access.sysexEnabled = false;
  await assert.rejects(env.midi.request(), /SysEx/);
  Object.defineProperty(globalThis, 'isSecureContext', { configurable: true, value: false });
  await assert.rejects(env.midi.request(), /HTTPS|localhost/);
});
test('connect opens both ports; explicit disconnect detaches callbacks, panics and closes ports', async t => {
  const env = environment(t);
  await env.midi.connect('input', 'output');
  assert.equal(env.midi.connected, true);
  assert.equal(env.input.opened, 1); assert.equal(env.output.opened, 1);
  const source = new Uint8Array([0xb0, 105, 64]);
  env.input.onmidimessage({ data: source }); source[2] = 0;
  assert.deepEqual(Array.from(env.messages[0]), [0xb0, 105, 64]);
  env.midi.noteOn(60);
  await env.midi.disconnect();
  assert.equal(env.input.onmidimessage, null);
  assert.equal(env.input.closed, 1); assert.equal(env.output.closed, 1);
  assert.equal(env.midi.connected, false);
  assert.equal(env.midi.activeNotes.size, 0);
  const panic = sentBytes(env.output).slice(1);
  assert.equal(panic.length, 48);
  for (let c = 0; c < 16; c++)
    assert.deepEqual(panic.slice(c * 3, c * 3 + 3), [[0xb0 + c, 64, 0], [0xb0 + c, 123, 0], [0xb0 + c, 120, 0]]);
});
test('failed port opening cleans up both ports and never reports a usable connection', async t => {
  const env = environment(t);
  env.output.open = async () => { throw new Error('Port busy'); };
  await assert.rejects(env.midi.connect('input', 'output'), /Port busy/);
  assert.equal(env.input.closed, 1); assert.equal(env.output.closed, 1);
  assert.equal(env.input.onmidimessage, null);
  assert.ok(!env.midi.connected);
});
test('slider throttle coalesces changes and cancellation prevents delayed output', async t => {
  t.mock.timers.enable({ apis: ['setTimeout'] });
  const env = environment(t); await env.midi.connect('input', 'output');
  env.midi.queueParameter(cutoff, 1); env.midi.queueParameter(cutoff, 73);
  t.mock.timers.tick(49); assert.deepEqual(sentBytes(env.output), []);
  t.mock.timers.tick(1); assert.deepEqual(sentBytes(env.output), [[0xb0, 105, 73]]);
  env.midi.queueParameter(arpOn, 1); env.midi.clearPending(); t.mock.timers.tick(100);
  assert.equal(env.output.sent.length, 1);
  env.midi.queueParameter(cutoff, 20); await env.midi.disconnect();
  const count = env.output.sent.length; t.mock.timers.tick(100);
  assert.equal(env.output.sent.length, count, 'disconnect cancels pending parameter sends');
});
test('audition sends edit-buffer packet, cancels pending edits and leaves stored dump intact', async t => {
  t.mock.timers.enable({ apis: ['setTimeout'] });
  const env = environment(t); await env.midi.connect('input', 'output');
  const dump = storedPatch(), before = dump.bytes.slice();
  env.midi.queueParameter(cutoff, 20); env.midi.upload(dump);
  const upload = env.output.sent[0].bytes;
  assert.deepEqual([upload[7], upload[11], upload[12]], [0, 0, 0]);
  assert.deepEqual(upload.slice(13), Array.from(before.subarray(13)));
  assert.deepEqual(dump.bytes, before);
  t.mock.timers.tick(100); assert.equal(env.output.sent.length, 1);
  env.midi.noteOn(60); env.midi.noteOff(60);
  assert.deepEqual(sentBytes(env.output).slice(1), [[0x90, 60, 90], [0x80, 60, 0]]);
});
test('note-off uses the original note channel; changing channel clears notes and pending edits', async t => {
  t.mock.timers.enable({ apis: ['setTimeout'] });
  const env = environment(t); await env.midi.connect('input', 'output');
  env.midi.setChannel(4); env.output.sent.length = 0;
  env.midi.noteOn(60); env.midi.noteOn(60);
  env.midi.channel = 9; // Retained note channel must remain valid even if current channel changes.
  env.midi.noteOff(60); env.midi.noteOff(60);
  assert.deepEqual(sentBytes(env.output), [[0x93, 60, 90], [0x83, 60, 0]]);
  env.midi.noteOn(64); env.midi.queueParameter(cutoff, 9); env.midi.setChannel(10);
  assert.equal(env.midi.activeNotes.size, 0);
  const count = env.output.sent.length; t.mock.timers.tick(100); assert.equal(env.output.sent.length, count);
  assert.equal(sentBytes(env.output).filter(m => m[1] === 123).length, 16);
});
test('disconnected ports cancel throttle and update availability', async t => {
  t.mock.timers.enable({ apis: ['setTimeout'] });
  const env = environment(t); await env.midi.connect('input', 'output');
  env.midi.queueParameter(cutoff, 9);
  env.input.state = 'disconnected'; env.output.state = 'disconnected'; env.access.onstatechange();
  t.mock.timers.tick(100);
  assert.deepEqual(sentBytes(env.output), []);
  assert.equal(env.states.at(-1).connected, false);
  assert.deepEqual(env.midi.ports(), { inputs: [], outputs: [] });
  assert.equal(env.errors.length, 1);
});
test('closing editor releases ports and state listeners after sending panic', async t => {
  const env = environment(t); await env.midi.connect('input', 'output');
  env.midi.noteOn(60); await env.midi.close();
  assert.equal(env.input.onmidimessage, null); assert.equal(env.access.onstatechange, null);
  assert.equal(env.input.connection, 'closed', 'unmount must release its MIDI input');
  assert.equal(env.output.connection, 'closed', 'unmount must release its MIDI output');
  assert.ok(!env.midi.connected);
  assert.equal(env.midi.activeNotes.size, 0);
});
test('losing only input stops audition notes on still-open output', async t => {
  const env = environment(t); await env.midi.connect('input', 'output');
  env.midi.noteOn(60); env.input.state = 'disconnected'; env.access.onstatechange();
  const stops = sentBytes(env.output).slice(1);
  assert.ok(stops.some(m => (m[0] === 0x80 && m[1] === 60) ||
    (m[0] === 0xb0 && [120, 123].includes(m[1]))),
  'a connected output must receive note-off or panic before active-note tracking is discarded');
  await env.midi.disconnect();
  assert.equal(env.input.connection, 'closed'); assert.equal(env.output.connection, 'closed');
});
