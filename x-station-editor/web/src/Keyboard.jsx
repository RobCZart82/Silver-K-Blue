import {useEffect,useState} from 'react';
import {CircleAlert} from 'lucide-react';

const letters='zsxdcvgbhnjm,l.;/';
export function Keyboard({midi,connected,onError}) {
  const [active,setActive]=useState(new Set());
  function on(note){try{midi.noteOn(note);setActive(prev=>new Set(prev).add(note));}catch(e){onError(e.message);}}
  function off(note){try{midi.noteOff(note);}catch(e){onError(e.message);}setActive(prev=>{const next=new Set(prev);next.delete(note);return next;});}
  useEffect(()=>{
    const up=e=>{const i=letters.indexOf(e.key);if(i>=0&&e.key.length===1)off(48+i);};
    const down=e=>{if(!connected||e.repeat||e.ctrlKey||e.metaKey||e.altKey||/INPUT|TEXTAREA|SELECT/.test(e.target.tagName))return;const i=letters.indexOf(e.key);if(i>=0&&e.key.length===1){e.preventDefault();on(48+i);}};
    const blur=()=>{midi.panic();setActive(new Set());};
    window.addEventListener('keydown',down);window.addEventListener('keyup',up);window.addEventListener('blur',blur);
    const hide=()=>{if(document.hidden)blur();};document.addEventListener('visibilitychange',hide);
    return()=>{window.removeEventListener('keydown',down);window.removeEventListener('keyup',up);window.removeEventListener('blur',blur);document.removeEventListener('visibilitychange',hide);midi.panic();};
  },[midi,connected]);
  const notes=Array.from({length:36},(_,i)=>48+i),white=notes.filter(n=>![1,3,6,8,10].includes(n%12));
  return <section className="test-keyboard"><h3>Test notes</h3><div className="keyboard" aria-label="MIDI-клавиатура для тестирования"><div className="white-keys">{white.map(n=><button className={`white-key ${active.has(n)?'active':''}`} key={n} disabled={!connected} aria-label={`MIDI note ${n}`} onPointerDown={e=>{e.preventDefault();e.currentTarget.setPointerCapture(e.pointerId);on(n);}} onPointerUp={()=>off(n)} onPointerCancel={()=>off(n)} onLostPointerCapture={()=>off(n)}>{n%12===0?<span>C{Math.floor(n/12)-1}</span>:null}</button>)}</div>{notes.filter(n=>[1,3,6,8,10].includes(n%12)).map(n=>{const index=white.filter(w=>w<n).length;return <button className={`black-key ${active.has(n)?'active':''}`} key={n} style={{left:`${index/white.length*100}%`}} disabled={!connected} aria-label={`MIDI note ${n}`} onPointerDown={e=>{e.preventDefault();e.currentTarget.setPointerCapture(e.pointerId);on(n);}} onPointerUp={()=>off(n)} onPointerCancel={()=>off(n)} onLostPointerCapture={()=>off(n)}/>;})}</div><button className="button" onClick={()=>{midi.panic();setActive(new Set());}} disabled={!connected}><CircleAlert size={17}/>Panic</button></section>;
}
