import {useEffect,useRef} from 'react';
import {X} from 'lucide-react';
export function Dialog({title,onClose,children}) {
  const ref=useRef(null);useEffect(()=>{ref.current.showModal();},[]);
  return <dialog ref={ref} onCancel={e=>{e.preventDefault();onClose();}} onClick={e=>{if(e.target===ref.current)onClose();}}><div className="dialog-heading"><h2>{title}</h2><button className="icon-button" aria-label="Закрыть" onClick={onClose}><X size={20}/></button></div>{children}</dialog>;
}
