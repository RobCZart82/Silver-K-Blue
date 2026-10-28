import {useEffect,useMemo,useRef,useState} from 'react';
import {Upload,Download,Search,Undo2,Send,Settings,Info,Copy,Trash2,AlertTriangle,Usb,PlugZap} from 'lucide-react';
import parameters from './parameters.json';
import {getName,parseSysEx,serializeSysEx,setName,readParameter,writeParameter,updateControl,updateTemplateSettings,SysExAssembler} from './protocol.js';
import {MidiConnection,ParameterDecoder} from './midi.js';
import {SynthPanel,synthGroups,groupNames} from './SynthPanel.jsx';
import {TemplatePanel} from './TemplatePanel.jsx';
import {Keyboard} from './Keyboard.jsx';
import {Dialog} from './Dialog.jsx';
import {unzipSync} from 'fflate';
import demoSounds from './data/demo-sounds.syx?base64';
import demoTemplate from './data/demo-template.syx?base64';

const initialMidi={connected:false,inputs:[],outputs:[],inputId:'',outputId:''};
const defaults=Object.fromEntries(parameters.map(p=>[p.id,p.default]));
const same=(a,b)=>a.length===b.length&&a.every((x,i)=>x===b[i]);
function entry(dump,source){return {...dump,id:crypto.randomUUID(),source,saved:dump.bytes.slice(),dirty:false};}
function download(bytes,name){const url=URL.createObjectURL(new Blob([bytes],{type:'application/octet-stream'}));const a=document.createElement('a');a.href=url;a.download=name.replace(/[<>:"/\\|?*\x00-\x1f]/g,'_');a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
function xstationPort(ports){return ports.find(p=>p.name?.replace(/[ -]/g,'').toLowerCase().includes('xstation'))?.id??'';}

export default function App(){
  const [entries,setEntries]=useState([]),[selected,setSelected]=useState(''),[mode,setMode]=useState('synth'),[group,setGroup]=useState('Oscillators'),[query,setQuery]=useState('');
  const [midiState,setMidiState]=useState(initialMidi),[ports,setPorts]=useState({input:'',output:''}),[channel,setChannel]=useState(1),[live,setLive]=useState(false),[receive,setReceive]=useState(false),[sync,setSync]=useState(false);
  const [message,setMessage]=useState('Файлы остаются на вашем компьютере.'),[error,setError]=useState(false),[modal,setModal]=useState(null),[busy,setBusy]=useState(false),[historyCount,setHistoryCount]=useState(0),[liveValues,setLiveValues]=useState(defaults);
  const fileInput=useRef(),history=useRef([]),stateRef=useRef(),assembler=useRef(new SysExAssembler()),decoder=useRef(new ParameterDecoder(parameters));
  const current=entries.find(e=>e.id===selected)??null;
  stateRef.current={current,receive,sync,channel};
  const [midi]=useState(()=>new MidiConnection({onState:setMidiState,onError:m=>notify(m,true),onMessage:data=>incoming(data)}));
  function notify(text,failed=false){setMessage(text);setError(failed);}
  async function action(fn){try{await fn();}catch(e){notify(e.name==='NotAllowedError'?'Доступ к MIDI/SysEx запрещён. Разрешите его для редактора в настройках Chrome или Edge.':e.message,true);}}
  function incoming(data){
    const s=stateRef.current;
    try{
      if(s.receive){const dumps=assembler.current.push(data);if(dumps.length){const rows=dumps.map(d=>entry(d,'MIDI dump'));setEntries(prev=>[...prev,...rows]);setSelected(rows[0].id);setMode(rows[0].kind);notify(`Принято: ${rows.length} · ${rows[0].name}`);}}
      if(s.sync&&s.current?.kind==='synth')for(const [p,value] of decoder.current.decode(data,s.channel)){
        setLiveValues(prev=>({...prev,[p.id]:value}));
        if(p.offset!=null)setEntries(prev=>prev.map(d=>{if(d.id!==s.current.id)return d;const next={...d,bytes:d.bytes.slice()};writeParameter(next,p,value);return {...next,dirty:!same(next.bytes,d.saved)};}));
      }
    }catch(e){assembler.current.reset();notify(e.message,true);}
  }
  useEffect(()=>{
    try{
      const [sounds,templates]=[demoSounds,demoTemplate].map(data=>parseSysEx(Uint8Array.from(atob(data),c=>c.charCodeAt(0))).map(d=>entry(d,'Novation demo')));
      setEntries([...sounds,...templates]);setSelected(sounds[0].id);
    }catch(e){notify(e.message,true);}
  },[]);
  useEffect(()=>{midi.setChannel(channel);},[midi,channel]);
  useEffect(()=>{const unload=e=>{midi.panic();if(entries.some(d=>d.dirty)){e.preventDefault();e.returnValue='';}};window.addEventListener('beforeunload',unload);return()=>window.removeEventListener('beforeunload',unload);},[midi,entries]);
  useEffect(()=>()=>{void Promise.resolve(midi.close()).catch(()=>{});},[midi]);
  useEffect(()=>{if(!midiState.connected)setLive(false);},[midiState.connected]);
  const visible=useMemo(()=>entries.filter(e=>e.kind===mode&&e.name.toLowerCase().includes(query.toLowerCase())),[entries,mode,query]);
  const values=useMemo(()=>Object.fromEntries(parameters.map(p=>[p.id,current?.kind==='synth'&&p.offset!=null?readParameter(current,p):liveValues[p.id]??p.default])),[current,liveValues]);
  function choose(id){midi.clearPending();setLive(false);setSelected(id);}
  function changeMode(next){setMode(next);setQuery('');const first=entries.find(e=>e.kind===next);choose(first?.id??'');}
  function mutate(fn){if(!current)throw new Error('Сначала откройте звук или MIDI-шаблон.');const next={...current,bytes:current.bytes.slice()};fn(next);if(same(next.bytes,current.bytes))return;history.current.push({id:current.id,bytes:current.bytes.slice()});if(history.current.length>200)history.current.shift();setHistoryCount(history.current.length);setEntries(prev=>prev.map(d=>d.id===current.id?{...next,name:getName(next),dirty:!same(next.bytes,current.saved)}:d));}
  function parameterChange(p,v){action(()=>{if(p.offset!=null&&current?.kind==='synth')mutate(d=>writeParameter(d,p,v));else setLiveValues(prev=>({...prev,[p.id]:v}));if(live&&midi.connected)midi.queueParameter(p,v);});}
  function undo(){const item=history.current.pop();if(!item)return;setHistoryCount(history.current.length);setEntries(prev=>prev.map(d=>{if(d.id!==item.id)return d;const next={...d,bytes:item.bytes};return {...next,name:getName(next),dirty:!same(next.bytes,d.saved)};}));setSelected(item.id);const d=entries.find(d=>d.id===item.id);if(d)setMode(d.kind);midi.clearPending();notify('Изменение файла отменено. Отправьте патч для обновления звучания.');}
  async function importFiles(files){
    setBusy(true);
    try{
      let rows=[],skipped=0;
      for(const file of files){
        if(file.size>16*1024*1024)throw new Error('Файл больше 16 МБ.');
        const bytes=new Uint8Array(await file.arrayBuffer());
        if(file.name.toLowerCase().endsWith('.zip')){
          let total=0;
          const items=unzipSync(bytes,{filter:e=>{if(!e.name.toLowerCase().endsWith('.syx'))return false;total+=e.originalSize;if(e.originalSize>16*1024*1024||total>64*1024*1024)throw new Error('Архив SysEx слишком большой.');return true;}});
          for(const [path,data] of Object.entries(items)){try{rows.push(...parseSysEx(data).map(d=>entry(d,path)));}catch{skipped++;}}
        }else rows.push(...parseSysEx(bytes).map(d=>entry(d,file.name)));
      }
      if(!rows.length)throw new Error('Совместимых звуков или шаблонов не найдено. Дампы прошивки и других моделей не поддерживаются.');
      setEntries(prev=>[...prev,...rows]);choose(rows[0].id);setMode(rows[0].kind);setQuery('');notify(`Импортировано ${rows.length} · пропущено несовместимых файлов: ${skipped}`);
    }catch(e){notify(e.message,true);}finally{setBusy(false);if(fileInput.current)fileInput.current.value='';}
  }
  function save(){if(!current)return;download(serializeSysEx([current]),`${current.name||'XStation'}.syx`);setEntries(prev=>prev.map(d=>d.id===current.id?{...d,saved:d.bytes.slice(),dirty:false}:d));notify('SYX сохранён в папку загрузок.');}
  function saveLibrary(){const rows=entries.filter(d=>d.kind===mode);if(rows.length){download(serializeSysEx(rows),`XStation-${mode}-library.syx`);notify(`Библиотека экспортирована: ${rows.length} элементов.`);}}
  function duplicate(){if(!current)return;const next=entry(current,'Копия');next.dirty=true;setEntries(prev=>[...prev,next]);choose(next.id);}
  function remove(){if(!current)return;if(current.dirty&&!window.confirm('Удалить изменённый элемент из библиотеки?'))return;const next=entries.filter(e=>e.id!==current.id);setEntries(next);choose(next.find(e=>e.kind===mode)?.id??'');history.current=[];setHistoryCount(0);}
  async function requestMidi(){setModal('midi');setBusy(true);await action(async()=>{const p=await midi.request();setPorts({input:xstationPort(p.inputs),output:xstationPort(p.outputs)});notify(p.inputs.length?'Выберите MIDI-порты X-Station.':'MIDI-входов пока нет. Подключите X-Station и проверьте драйвер.');});setBusy(false);}
  return <div className="app-shell">
    <header className="app-header"><div className="brand"><strong>X-Station</strong><span>Editor</span></div><nav aria-label="Тип редактора"><button className={mode==='synth'?'active':''} onClick={()=>changeMode('synth')}>Synth</button><button className={mode==='template'?'active':''} onClick={()=>changeMode('template')}>MIDI templates</button></nav><div className="header-right"><button className="connection-button" onClick={requestMidi}><span className={`status-dot ${midiState.connected?'connected':''}`}/>{midiState.connected?'USB подключён':'Подключить USB'}</button><button className="icon-button" aria-label="Настройки и подключение" onClick={()=>setModal('help')}><Settings size={20}/></button></div></header>
    <aside className="library"><div className="library-top"><h2>Библиотека</h2><button className="import-button" onClick={()=>fileInput.current.click()} disabled={busy}><Upload size={17}/>{busy?'Загрузка…':'Импорт .syx / .zip'}</button><input ref={fileInput} type="file" accept=".syx,.zip" multiple onChange={e=>importFiles([...e.target.files])} hidden aria-label="Импорт файлов X-Station"/><label className="search"><Search size={17}/><input placeholder="Поиск патчей…" value={query} onChange={e=>setQuery(e.target.value)} aria-label="Поиск в библиотеке"/></label></div><div className="patch-list" role="listbox" aria-label="Библиотека патчей">{visible.map(d=><button key={d.id} role="option" aria-selected={d.id===selected} className={d.id===selected?'selected':''} onClick={()=>choose(d.id)} title={d.source}><span>{d.name||'Без имени'}</span>{d.dirty?<span className="dirty-dot" aria-label="Изменён"/>:null}</button>)}{!visible.length?<p className="no-results">{query?'Ничего не найдено':'Импортируйте SYX или ZIP'}</p>:null}</div><div className="library-footer"><div className="library-tools"><button aria-label="Экспорт библиотеки" title="Экспорт библиотеки" onClick={saveLibrary}><Download size={17}/></button><button aria-label="Копия выбранного" title="Копия выбранного" onClick={duplicate} disabled={!current}><Copy size={17}/></button><button aria-label="Удалить выбранный" title="Удалить из списка" onClick={remove} disabled={!current}><Trash2 size={17}/></button><span>{entries.filter(e=>e.kind===mode).length} {mode==='synth'?'звуков':'шаблонов'}</span></div><div className="offline"><Info size={16}/>{midiState.connected?'USB MIDI':'Офлайн-редактирование'}</div></div></aside>
    <main className="workspace"><div className="workspace-heading"><div><input className="patch-name" aria-label="Имя патча · ASCII до 16 символов" key={current?.id+current?.name} defaultValue={current?.name??'Без файла'} maxLength={16} disabled={!current} onBlur={e=>{if(current&&e.target.value!==current.name)action(()=>mutate(d=>setName(d,e.target.value)));}} onKeyDown={e=>{if(e.key==='Enter')e.currentTarget.blur();}}/><p>{mode==='synth'?'Synth patch':'MIDI control template'}{current?.dirty?' · изменён':''}</p></div><div className="workspace-actions"><button className="button" onClick={undo} disabled={!historyCount}><Undo2 size={17}/>Отменить</button><button className="button" onClick={save} disabled={!current}><Download size={17}/>Сохранить .syx</button><button className="button primary" onClick={()=>action(()=>{if(current){midi.upload(current);notify('Отправлено во временный буфер. Для сохранения используйте Write на X-Station.');}})} disabled={!current||!midiState.connected}><Send size={17}/>На X-Station</button></div></div>
    {mode==='synth'?<><nav className="section-tabs" aria-label="Разделы синтезатора">{synthGroups.map(g=><button key={g} className={group===g?'active':''} onClick={()=>setGroup(g)}>{groupNames[g]}</button>)}</nav><SynthPanel group={group} values={values} onChange={parameterChange}/><div className="midi-options"><label>Канал<select aria-label="MIDI-канал синтезатора" value={channel} onChange={e=>setChannel(Number(e.target.value))}>{Array.from({length:16},(_,i)=><option value={i+1} key={i}>{i+1}</option>)}</select></label><label><input type="checkbox" checked={live} onChange={e=>{setLive(e.target.checked);if(!e.target.checked)midi.clearPending();}} disabled={!midiState.connected}/>Live MIDI</label><label><input type="checkbox" checked={sync} onChange={e=>setSync(e.target.checked)} disabled={!midiState.connected}/>Синхронизировать ручки</label><label><input type="checkbox" checked={receive} onChange={e=>{setReceive(e.target.checked);assembler.current.reset();}} disabled={!midiState.connected}/>Принимать SysEx</label></div><Keyboard midi={midi} connected={midiState.connected} onError={m=>notify(m,true)}/><p className="experimental"><AlertTriangle size={16}/>Офлайн-карта звуков экспериментальная · требуется проверка на X-Station</p></>:current?.kind==='template'?<><TemplatePanel dump={current} onControl={(index,changes)=>action(()=>mutate(d=>updateControl(d,index,changes)))} onSettings={changes=>action(()=>mutate(d=>updateTemplateSettings(d,changes)))}/><div className="midi-options"><label><input type="checkbox" checked={receive} onChange={e=>{setReceive(e.target.checked);assembler.current.reset();}} disabled={!midiState.connected}/>Принимать SysEx</label><span>Приём: Global → Dump Current Template → Write</span></div></>:<div className="empty-state"><SlidersIcon/><h2>Откройте MIDI-шаблон</h2><p>Импортируйте файл .syx или архив с оригинальными шаблонами.</p><button className="button" onClick={()=>fileInput.current.click()}>Импорт файлов</button></div>}
    <footer className={`app-status ${error?'error':''}`} role="status"><span className="status-dot"/>{message}</footer></main>
    {modal==='midi'?<Dialog title="Подключение по USB" onClose={()=>setModal(null)}><p>Разрешите браузеру доступ к MIDI и SysEx. На X-Station выберите <strong>Synth MIDI Src: USB</strong>.</p><label className="form-field"><span>MIDI IN</span><select aria-label="MIDI IN" value={ports.input} onChange={e=>setPorts(prev=>({...prev,input:e.target.value}))}><option value="">Выберите вход X-Station</option>{midiState.inputs.map(p=><option key={p.id} value={p.id}>{p.name}</option>)}</select></label><label className="form-field"><span>MIDI OUT</span><select aria-label="MIDI OUT" value={ports.output} onChange={e=>setPorts(prev=>({...prev,output:e.target.value}))}><option value="">Выберите выход X-Station</option>{midiState.outputs.map(p=><option key={p.id} value={p.id}>{p.name}</option>)}</select></label><p className="dialog-hint">Chrome / Edge · Windows 10/11 x64. Если порты не появились, проверьте USB-драйвер устройства.</p><div className="dialog-actions"><button className="button" onClick={requestMidi} disabled={busy}><Usb size={16}/>Обновить</button>{midiState.connected?<button className="button" onClick={()=>action(async()=>{await midi.disconnect();setModal(null);notify('MIDI отключён.');})}>Отключить</button>:<button className="button primary" disabled={!ports.input||!ports.output||busy} onClick={()=>action(async()=>{setBusy(true);try{await midi.connect(ports.input,ports.output);setModal(null);notify('USB MIDI подключён. Включите Synth Mode и проверьте MIDI-канал устройства.');}finally{setBusy(false);}})}><PlugZap size={17}/>Подключить</button>}</div></Dialog>:null}
    {modal==='help'?<Dialog title="X-Station в браузере" onClose={()=>setModal(null)}><ol className="setup-list"><li>Подключите X-Station по USB и включите питание.</li><li>Откройте редактор в Chrome или Edge. Нажмите «Подключить USB» и разрешите MIDI/SysEx.</li><li>На устройстве включите <strong>Play/Synth</strong>, нажмите <strong>Global</strong> дважды и выберите <strong>Synth MIDI Src: USB</strong>. MIDI-канал должен совпадать с редактором.</li><li>Выберите звук и нажмите «На X-Station». После отправки включите Live MIDI для изменения звучания.</li><li>Для приёма включите «Принимать SysEx» и выполните <strong>Dump Current Program / Template</strong> на устройстве.</li><li>Сохранение в память выполняется кнопкой <strong>Write</strong> на X-Station.</li></ol><p>Звук создаёт сам X-Station. Веб-редактор управляет им по MIDI и сохраняет файлы; он не заменяет USB/ASIO-драйвер.</p><p>Synth Template меняет поведение MIDI-назначений Mod/Env3. Физический режим синтезатора включается на устройстве.</p><p className="dialog-hint">Назначения шаблонов восстановлены из оригинального редактора. Офлайн-смещения звуков предполагают общую структуру с KS. Аппаратной проверки ещё не было.</p><a href="https://downloads.novationmusic.com/novation/synthesisers-legacy/x-station-61" target="_blank" rel="noreferrer">Документация и драйвер Novation</a></Dialog>:null}
  </div>;
}
function SlidersIcon(){return <Settings size={40} strokeWidth={1}/>;}
