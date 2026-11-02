import test from 'node:test';
import assert from 'node:assert/strict';
import {createKeyboardInput} from '../src/keyboard-input.js';
const event=(key,extra={})=>({key,code:'KeyZ',repeat:false,ctrlKey:false,metaKey:false,altKey:false,target:{tagName:'BODY'},preventDefault(){},...extra});
test('keyup releases the started note after Shift or keyboard layout changes',()=>{
  for(const released of ['Z','y']) {
    const messages=[],input=createKeyboardInput(n=>messages.push(['on',n]),n=>messages.push(['off',n]));
    input.down(event('z'),true);input.up(event(released));input.up(event(released));
    assert.deepEqual(messages,[['on',48],['off',48]]);
  }
});
test('ignores repeats, typing and disconnected input; reset forgets panicked keys',()=>{
  const messages=[],input=createKeyboardInput(n=>messages.push(['on',n]),n=>messages.push(['off',n]));
  input.down(event('z'),false);input.down(event('z',{target:{tagName:'INPUT'}}),true);
  input.down(event('z',{ctrlKey:true}),true);input.down(event('z',{repeat:true}),true);
  assert.deepEqual(messages,[]);
  input.down(event('z'),true);input.down(event('z'),true);input.reset();input.up(event('Z'));
  assert.deepEqual(messages,[['on',48]]);
});
