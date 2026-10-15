import test from 'node:test';
import assert from 'node:assert/strict';
import { createMidiLearner, noteFromMessage, noteLabel, noteRangeChanges } from '../src/midiLearn.mjs';

class Input extends EventTarget {
  state = 'connected';
  opens = 0;
  closes = 0;
  async open() { this.opens++; return this; }
  async close() { this.closes++; return this; }
  send(data) { const event = new Event('midimessage'); event.data = data; this.dispatchEvent(event); }
}
class Access extends EventTarget { inputs = new Map(); }
const flush = async () => { for (let i = 0; i < 5; i++) await Promise.resolve(); };
const setup = () => {
  const input = new Input(), access = new Access(), states = [], notes = [];
  access.inputs.set('keyboard', input);
  const learner = createMidiLearner({ requestAccess: async () => access, onStatus: s => states.push(s) });
  return { input, access, states, notes, learner };
};

test('accepts Note On on all channels and ignores release, zero velocity, clock, CC and invalid notes', () => {
  for (let ch = 0; ch < 16; ch++) assert.equal(noteFromMessage([0x90 | ch, 60, 100]), 60);
  for (const data of [[0x80,60,80],[0x90,60,0],[0xb0,64,127],[0xf8],[0x90,128,1],[0x90,-1,1],[0x90,60,128],[0x90,60],[0x90,60,1,2]]) assert.equal(noteFromMessage(data), null);
  assert.equal(noteFromMessage([0x90,0,1]), 0);
  assert.equal(noteFromMessage([0x9f,127,127]), 127);
  assert.equal(noteLabel(60), 'C4 · 60');
});

test('captures one note only and releases listeners and ports', async () => {
  const { learner, input, notes, states } = setup();
  await learner.begin(n => notes.push(n)); await flush();
  input.send([0xb0,64,127]); input.send([0x90,60,0]);
  input.send([0x92,61,99]); input.send([0x92,62,99]);
  assert.deepEqual(notes,[61]); assert.equal(states.at(-1).phase,'learned'); assert.equal(input.closes,1);
});

test('cancelling before permission resolves cannot change a stale part', async () => {
  let grant; const access = new Access(), input = new Input(), notes = [];
  access.inputs.set('k',input);
  const learner = createMidiLearner({requestAccess:()=>new Promise(resolve=>{grant=resolve;}),onStatus:()=>{}});
  const pending = learner.begin(n=>notes.push(n)); learner.cancel(); grant(access); await pending;
  input.send([0x90,60,100]); assert.deepEqual(notes,[]); assert.equal(input.opens,0);
});

test('switching the learning target updates only the new target; manual cancel ignores later notes', async () => {
  const { learner, input } = setup(); const notes = [];
  await learner.begin(n=>notes.push(['low1',n])); await flush();
  await learner.begin(n=>notes.push(['high3',n])); await flush();
  input.send([0x90,72,100]); assert.deepEqual(notes,[['high3',72]]);
  await learner.begin(n=>notes.push(['low2',n])); learner.cancel(); input.send([0x90,30,100]);
  assert.deepEqual(notes,[['high3',72]]);
});

test('supports connecting a keyboard while waiting and disconnecting it without accepting stale events', async () => {
  const { learner, access, input, states, notes } = setup(); access.inputs.clear();
  await learner.begin(n=>notes.push(n)); assert.equal(states.at(-1).phase,'waiting');
  access.inputs.set('k',input); access.dispatchEvent(new Event('statechange')); await flush();
  input.state='disconnected'; access.dispatchEvent(new Event('statechange'));
  input.send([0x90,45,100]); assert.deepEqual(notes,[]);
  input.state='connected'; access.dispatchEvent(new Event('statechange')); await flush();
  input.send([0x90,45,100]); assert.deepEqual(notes,[45]);
});

test('permission denial is recoverable and does not leave a pending capture', async () => {
  const states = []; let attempts = 0; const access = new Access(), input = new Input(); access.inputs.set('k',input);
  const learner = createMidiLearner({requestAccess:async()=>{if(!attempts++)throw Object.assign(new Error(),{name:'NotAllowedError'});return access;},onStatus:s=>states.push(s)});
  await learner.begin(()=>assert.fail('denied request captured a note'));
  assert.equal(states.at(-1).phase,'error');
  const notes = []; await learner.begin(n=>notes.push(n)); await flush(); input.send([0x90,64,100]); assert.deepEqual(notes,[64]);
});

test('a failed input does not prevent another connected keyboard from learning', async () => {
  const { learner, access, input, states, notes } = setup(); const broken = new Input();
  broken.open = async()=>{throw new Error('offline');}; access.inputs.set('broken',broken);
  await learner.begin(n=>notes.push(n)); await flush();
  assert.equal(states.at(-1).phase,'waiting'); input.send([0x90,36,100]); assert.deepEqual(notes,[36]);
});

test('keeps exactly the chosen note and moves the opposite range endpoint only when necessary', () => {
  assert.deepEqual(noteRangeChanges('part_low',84,0,59),[['part_high',84],['part_low',84]]);
  assert.deepEqual(noteRangeChanges('part_high',24,60,127),[['part_low',24],['part_high',24]]);
  assert.deepEqual(noteRangeChanges('part_low',36,0,127),[['part_low',36]]);
});
