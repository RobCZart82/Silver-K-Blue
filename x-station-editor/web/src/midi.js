import { midiMessages, prepareUpload } from './protocol.js';

export class MidiConnection {
  constructor({onState, onMessage, onError}) {
    this.onState=onState; this.onMessage=onMessage; this.onError=onError;
    this.access=null; this.input=null; this.output=null; this.pending=new Map();
    this.activeNotes=new Map(); this.channel=1; this.timer=null;this.connecting=false;
  }
  async request() {
    if (!globalThis.isSecureContext) throw new Error('Браузер не разрешает MIDI для этого адреса. Откройте автономный HTML в Chrome или Edge; для веб-размещения используйте HTTPS.');
    if (!navigator.requestMIDIAccess) throw new Error('Этот браузер не поддерживает Web MIDI. Откройте редактор в Chrome или Edge.');
    this.access=await navigator.requestMIDIAccess({sysex:true});
    if (!this.access.sysexEnabled) throw new Error('Разрешите MIDI и SysEx в настройках браузера.');
    this.access.onstatechange=()=>{
      if (this.output?.state==='disconnected' || this.input?.state==='disconnected') {
        try {this.panic();} catch(e) {this.clearPending();this.activeNotes.clear();}
        this.onError('X-Station отключён. Проверьте USB и подключите порты снова.');
      }
      this.publish();
    };
    this.publish(); return this.ports();
  }
  ports() {return {inputs:[...(this.access?.inputs.values()??[])].filter(p=>p.state==='connected'), outputs:[...(this.access?.outputs.values()??[])].filter(p=>p.state==='connected')};}
  get connected() {return this.input?.state==='connected' && this.output?.state==='connected' && this.input.connection==='open' && this.output.connection==='open';}
  publish() {this.onState({...this.ports(),connected:!!this.connected,inputId:this.input?.id??'',outputId:this.output?.id??''});}
  async connect(inputId,outputId) {
    if(this.connecting)throw new Error('Подключение уже выполняется.');
    this.connecting=true;
    try {
    if (!this.access) await this.request();
    const input=this.access.inputs.get(inputId), output=this.access.outputs.get(outputId);
    if (!input || !output || input.state!=='connected' || output.state!=='connected') throw new Error('Выберите доступные входной и выходной MIDI-порты X-Station.');
    await this.disconnect();
    try {
      await input.open(); await output.open(); this.input=input; this.output=output;
      input.onmidimessage=e=>this.onMessage(new Uint8Array(e.data)); this.publish();
    } catch(e) {await input.close(); await output.close(); throw e;}
    } finally {this.connecting=false;}
  }
  async disconnect() {
    this.clearPending();
    try {this.panic();} catch(e) {this.onError(e.message);}
    const input=this.input,output=this.output;
    if(input)input.onmidimessage=null;
    this.input=null;this.output=null;
    const results=await Promise.allSettled([input?.close(),output?.close()]);
    this.publish();
    const failed=results.find(r=>r.status==='rejected');if(failed)throw failed.reason;
  }
  setChannel(channel) {if (this.channel!==channel) {if (this.connected) this.panic(); this.clearPending();} this.channel=channel;}
  send(bytes,time) {if (!this.connected) throw new Error('Сначала подключите X-Station по USB.'); this.output.send(bytes,time);}
  upload(dump) {this.clearPending(); this.send(prepareUpload(dump));}
  queueParameter(parameter,value) {
    if (!this.connected) return;
    this.pending.set(parameter.id,{parameter,value});
    if (this.timer!==null) return;
    this.timer=setTimeout(()=>{
      this.timer=null;
      try {for(const {parameter,value} of this.pending.values()) for(const m of midiMessages(parameter,value,this.channel)) this.send(m);}
      catch(e) {this.onError(e.message);}
      finally {this.pending.clear();}
    },50);
  }
  clearPending() {if(this.timer!==null) clearTimeout(this.timer);this.timer=null;this.pending.clear();}
  noteOn(note) {
    if(this.activeNotes.has(note))return;
    const channel=this.channel; this.send([0x90+channel-1,note,90]); this.activeNotes.set(note,channel);
  }
  noteOff(note) {
    const channel=this.activeNotes.get(note); if(channel===undefined)return;
    this.activeNotes.delete(note); if(this.connected)this.send([0x80+channel-1,note,0]);
  }
  panic() {
    this.clearPending(); if(this.output?.state==='connected') {
      for(let c=0;c<16;c++) for(const n of [64,123,120]) this.output.send([0xb0+c,n,0]);
    }
    this.activeNotes.clear();
  }
  async close() {if(this.access)this.access.onstatechange=null;this.onState=()=>{};await this.disconnect();}
}

export class ParameterDecoder {
  constructor(parameters) {this.parameters=parameters;this.lsb=Array(16).fill(-1);this.msb=Array(16).fill(0);}
  decode(data,channel) {
    if(data.length!==3 || data[0]!==0xb0+channel-1)return [];
    const c=channel-1,n=data[1],v=data[2];
    if(n===98){this.lsb[c]=v;return [];}
    if(n===99){this.msb[c]=v;return [];}
    if(n===100||n===101){this.lsb[c]=-1;return [];}
    const result=[];
    for(const p of this.parameters) {
      let value;
      if(p.kind==='CC'&&p.number===n)value=(v&(p.wireMask??127))>>(p.wireShift??0);
      else if(p.kind==='NRPN'&&n===6&&this.msb[c]===0&&p.number===this.lsb[c])value=v-(p.base??0);
      else continue;
      if(value>=p.min&&value<=p.max)result.push([p,value]);
    }
    return result;
  }
}
