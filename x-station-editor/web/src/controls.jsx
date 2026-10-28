import {useEffect,useState} from 'react';

export function NumericField({value,onChange,min=0,max=127,label,className=''}) {
  const [draft,setDraft]=useState(String(value));
  useEffect(()=>setDraft(String(value)),[value]);
  function commit(){const n=Number(draft);if(draft!==''&&Number.isInteger(n)&&n>=min&&n<=max)onChange(n);else setDraft(String(value));}
  return <input className={`numeric ${className}`} type="number" min={min} max={max} value={draft} aria-label={label} onChange={e=>setDraft(e.target.value)} onBlur={commit} onKeyDown={e=>{if(e.key==='Enter'){commit();e.currentTarget.blur();}}}/>;
}
export function ParameterKnob({parameter,value,onChange,compact=false}) {
  const p=parameter,ratio=Math.max(0,Math.min(1,(value-p.min)/(p.max-p.min||1))),angle=-135+270*ratio;
  const label=`${p.section} ${p.name}`;
  return <label className={`knob-control ${compact?'compact':''}`}>
    <span className="control-label">{p.name}</span>
    <span className="dial">
      <svg viewBox="0 0 80 80" aria-hidden="true"><circle cx="40" cy="40" r="32" fill="none" stroke="#c5cbd3" strokeWidth="3.5" strokeDasharray="150.8 201" transform="rotate(135 40 40)"/><circle cx="40" cy="40" r="32" fill="none" stroke="#176bff" strokeWidth="3.5" strokeDasharray={`${150.8*ratio} 201`} transform="rotate(135 40 40)"/><circle cx="40" cy="40" r="26" fill="#f7f9fc" stroke="#e2e7ed"/><line x1="40" y1="16" x2="40" y2="30" stroke="#176bff" strokeWidth="2" strokeLinecap="round" transform={`rotate(${angle} 40 40)`}/></svg>
      <input type="range" min={p.min} max={p.max} value={value} onChange={e=>onChange(Number(e.target.value))} aria-label={label} title={`${p.kind} ${p.number} • MIDI ${value}`}/>
    </span>
    <NumericField value={value+(p.displayOffset??0)} min={p.min+(p.displayOffset??0)} max={p.max+(p.displayOffset??0)} onChange={v=>onChange(v-(p.displayOffset??0))} label={`${label} значение`}/>
  </label>;
}
export function ParameterSlider({parameter,value,onChange,shortName}) {
  const p=parameter;return <label className="slider-control"><span>{shortName??p.name}</span><input type="range" min={p.min} max={p.max} value={value} onChange={e=>onChange(Number(e.target.value))} aria-label={`${p.section} ${p.name}`}/><NumericField value={value+(p.displayOffset??0)} min={p.min+(p.displayOffset??0)} max={p.max+(p.displayOffset??0)} onChange={v=>onChange(v-(p.displayOffset??0))} label={`${p.section} ${p.name} значение`}/></label>;
}
export function ParameterSelect({parameter,value,onChange}) {
  const p=parameter;return <label className="select-control"><span>{p.name}</span><select aria-label={`${p.section} ${p.name}`} value={value} onChange={e=>onChange(Number(e.target.value))}>{Array.from({length:p.max-p.min+1},(_,i)=>{const v=p.min+i;return <option key={v} value={v}>{p.labels?.[i]??String(v+(p.displayOffset??0))}</option>;})}</select></label>;
}
export function Waveform({type}) {
  let path;
  if(type===2)path='M6 53 L56 12 L56 53 L106 12 L106 53 L156 12 L156 53 L196 20';
  else if(type===1)path='M6 40 L36 14 L86 55 L136 14 L186 55';
  else if(type===3)path='M6 54 L6 16 L47 16 L47 54 L88 54 L88 16 L129 16 L129 54 L170 54 L170 16 L194 16';
  else if(type===0)path=Array.from({length:100},(_,i)=>`${i?'L':'M'}${6+i*1.9},${35-Math.sin(i/10)*20}`).join(' ');
  return <svg className="waveform" viewBox="0 0 200 70" aria-label="Форма волны">{path?<path d={path} fill="none" stroke="#176bff" strokeWidth="1.5"/>:<text x="100" y="39" textAnchor="middle" fill="#708095" fontSize="12">Wave {type}</text>}</svg>;
}
export function EnvelopeGraph({a,d,s,r}) {
  const attack=16+a/127*48,decay=attack+30+d/127*36,sustain=64-s/127*50,release=150+r/127*35;
  const path=`M4 66 L${attack} 12 L${decay} ${sustain} L${release} ${sustain} L196 66`;
  return <svg className="envelope" viewBox="0 0 200 78" role="img" aria-label="Относительная форма огибающей ADSR"><defs><pattern id="grid" width="20" height="15" patternUnits="userSpaceOnUse"><path d="M20 0 H0 V15" fill="none" stroke="#e5ebf3" strokeWidth=".7"/></pattern></defs><rect width="200" height="78" fill="url(#grid)"/><path d={`${path} Z`} fill="#176bff" opacity=".12"/><path d={path} stroke="#176bff" strokeWidth="1.5" fill="none"/></svg>;
}
