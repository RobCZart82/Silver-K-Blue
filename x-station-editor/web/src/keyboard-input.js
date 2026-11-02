// Remember the note started by a physical key, even if Shift/layout changes e.key.
const letters='zsxdcvgbhnjm,l.;/';
export function createKeyboardInput(on,off) {
  const held=new Map();
  return {
    down(e,connected) {
      if(!connected||e.repeat||e.ctrlKey||e.metaKey||e.altKey||/INPUT|TEXTAREA|SELECT/.test(e.target.tagName))return;
      const key=e.code||e.key.toLowerCase(),i=letters.indexOf(e.key.toLowerCase());
      if(e.key.length!==1||i<0||held.has(key))return;
      e.preventDefault();const note=48+i;held.set(key,note);on(note);
    },
    up(e) {
      const key=e.code||e.key.toLowerCase(),note=held.get(key);
      if(note===undefined)return;
      held.delete(key);off(note);
    },
    reset() {held.clear();}
  };
}
