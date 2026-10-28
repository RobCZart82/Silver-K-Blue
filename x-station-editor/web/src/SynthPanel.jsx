import parameters from './parameters.json';
import {ParameterKnob,ParameterSelect,ParameterSlider,Waveform,EnvelopeGraph} from './controls.jsx';

const byId=new Map(parameters.map(p=>[p.id,p]));
export const synthGroups=['Oscillators','Mixer','Filter','Envelopes','LFOs','Modulation','Arpeggiator','Voice'];
export const groupNames={Oscillators:'Осцилляторы',Mixer:'Микшер',Filter:'Фильтр',Envelopes:'Огибающие',LFOs:'LFO',Modulation:'Модуляция',Arpeggiator:'Арпеджиатор',Voice:'Голоса'};
export function SynthPanel({group,values,onChange}) {
  const control=(id,kind='knob',shortName)=>{
    const p=byId.get(id);if(!p)return null;
    const props={parameter:p,value:values[id]??p.default,onChange:v=>onChange(p,v)};
    return kind==='select'?<ParameterSelect key={id} {...props}/>:kind==='slider'?<ParameterSlider key={id} {...props} shortName={shortName}/>:<ParameterKnob key={id} {...props}/>;
  };
  return <>
    {group==='Oscillators'?<div className="oscillators">{[1,2,3].map(i=><section className="oscillator" key={i}><h2>Oscillator {i}</h2><div className="wave-row">{control(`osc${i}_wave`,'select')}<Waveform type={values[`osc${i}_wave`]??2}/></div><div className="knob-row">{control(`osc${i}_semitone`)}{control(`osc${i}_fine`)}{control(`osc${i}_pulse_width`)}</div><h3>Pitch modulation</h3>{control(`osc${i}_lfo1_pitch`,'slider','LFO 1')}{control(`osc${i}_env_pitch`,'slider','Envelope')}<details className="more-controls"><summary>Дополнительные настройки</summary>{control(`osc${i}_octave`,'select')}{control(`osc${i}_bend_depth`,'slider')}{control(`osc${i}_lfo2_pwm`,'slider')}{control(`osc${i}_env_pwm`,'slider')}</details></section>)}</div>:<div className="parameter-sections">{Object.entries(Object.groupBy(parameters.filter(p=>p.group===group),p=>p.section)).map(([section,ps])=><section key={section}><h2>{section}</h2><div className="parameter-row">{ps.map(p=>{const props={parameter:p,value:values[p.id]??p.default,onChange:v=>onChange(p,v)};return p.labels||p.max-p.min<=7?<ParameterSelect key={p.id} {...props}/>:<ParameterKnob key={p.id} {...props}/>;})}</div></section>)}</div>}
    <section className="quick-controls"><h2>Quick controls</h2><div className="quick-grid"><section className="quick-filter"><h3>Filter</h3><div className="knob-row">{control('cutoff')}{control('resonance')}</div></section><section className="quick-envelope"><h3>Amplitude envelope</h3><div className="adsr-body"><EnvelopeGraph a={values.attack??2} d={values.decay??64} s={values.sustain??127} r={values.release??32}/><div>{control('attack','slider','A')}{control('decay','slider','D')}{control('sustain','slider','S')}{control('release','slider','R')}</div></div></section><section className="quick-tempo"><h3>Tempo</h3>{control('arp_rate')}</section><section className="quick-arp"><h3>Arpeggiator</h3><label className="toggle-row"><input type="checkbox" checked={values.arp_on===1} onChange={e=>onChange(byId.get('arp_on'),e.target.checked?1:0)}/><span>{values.arp_on===1?'On':'Off'}</span></label>{control('arp_pattern','select')}{control('arp_octaves','select')}</section></div></section>
  </>;
}
