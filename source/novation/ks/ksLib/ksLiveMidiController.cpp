#include "ksLiveMidiController.h"
#include <stdexcept>

namespace ks {
namespace {
constexpr unsigned Entry=0x2de8,Status=0x15c1b,Data=0x15c1f;
void flag(H8SEmulator& h,unsigned mask,bool set){h.ccr=uint8_t((h.ccr&~mask)|(set?mask:0));}
void value(H8SEmulator& h,int32_t v){h.ccr=uint8_t((h.ccr&~14)|(v<0?8:0)|(!v?4:0));}
bool interruptReady(const H8SEmulator& h){
    if(!(h.pending_irqs||h.pending_irqs_high))return false;
    if(!h.interruptMode2)return (h.pending_irqs!=0)&&(!(h.ccr&128)||!(h.ccr&16));
    for(unsigned n=0;n<128;++n)if(((n<64?h.pending_irqs:h.pending_irqs_high)>>(n&63))&1)
        if(h.interruptPriority[n]>(h.exr&7))return true;
    return false;
}
}
bool LiveMidiController::supported(uint8_t status){const auto kind=status&0xf0;return kind==0x80||kind==0x90||kind==0xb0;}
LiveMidiController::LiveMidiController(const H8SEmulator& h){
    // Reject unverified parser/table revisions. No firmware bytes are embedded.
    uint64_t hash=14695981039346656037ull;
    for(auto range:{std::pair<unsigned,unsigned>{0x2de8,0x2efc},{0xf250,0xf2da}})
        for(unsigned n=range.first;n<range.second;++n){hash^=h.memory[n];hash*=1099511628211ull;}
    if(hash!=0x883c476e3df41f22ull)throw std::runtime_error("Native KS MIDI decoder requires verified OS 2.1.02");
    LiveNoteController::verify(h);
}
bool LiveMidiController::step(ControllerCpu& h){
    const unsigned pc=h.getPC();
    if(LiveNoteController::candidate(pc))return !interruptReady(h)&&noteActions.step(h);
    if(pc<0x2de8 || pc>=0x2efc || (!decoding && pc!=Entry))return false;
    // Interrupt arbitration stays in H8. Native instructions may be interrupted
    // between ANY two boundaries, including inside a partial running-status message.
    if(interruptReady(h))return false;
    if(pc==Entry){
        const auto byte=uint8_t(h.regs[6].rl);
        // Inspect backing RAM without bus cycles; ordinary instruction accesses
        // below still pass through the board mapping and its exact clock model.
        decoding=byte&128?supported(byte):
            (supported(h.memory[0xa00000+Status])&&supported(h.memory[0xa00000+0x15c1c]));
        if(decoding)++bytes;
    }
    if(!decoding)return false;
    unsigned length=0;
    switch(pc){
    case 0x2de8:case 0x2df0:case 0x2df2:case 0x2dfa:case 0x2e1c:case 0x2e1e:case 0x2e26:
    case 0x2e2e:case 0x2e44:case 0x2e58:case 0x2e62:case 0x2e64:case 0x2e6c:case 0x2e6e:
    case 0x2e78:case 0x2e86:case 0x2e90:case 0x2e92:case 0x2ed4:case 0x2ed6:case 0x2ed8:
    case 0x2eda:case 0x2edc:case 0x2ede:case 0x2ee0:case 0x2ee2:case 0x2eea:length=2;break;
    case 0x2e02:case 0x2ecc:length=4;break;
    case 0x2dea:case 0x2df4:case 0x2dfc:case 0x2e0e:case 0x2e20:case 0x2e28:case 0x2e30:case 0x2e36:
    case 0x2e52:case 0x2e66:case 0x2e7a:case 0x2e80:case 0x2e9c:case 0x2ea2:case 0x2eba:
    case 0x2ec0:case 0x2ec6:case 0x2ee4:length=6;break;
    case 0x2e06:case 0x2e14:case 0x2e3c:case 0x2e5a:case 0x2e70:case 0x2e88:
    case 0x2e94:case 0x2ea8:length=8;break;
    case 0x2eb0:length=10;break;
    default:return false; // Original handlers, interrupts and unsupported MIDI.
    }
    // Preserve instruction fetch bus cycles and bus pairing, without decoding
    // or executing firmware instructions. No JIT or CPU-core changes.
    for(unsigned n=0;n<length;++n)h.read8(pc+n);
    unsigned next=pc+length;
    auto mov8=[&](int8_t& dst,int v){dst=int8_t(v);value(h,dst);};
    auto store8=[&](unsigned addr,int8_t v){value(h,v);h.write8(v,addr);};
    auto mov32=[&](int& dst,uint32_t v){dst=int32_t(v);value(h,dst);};
    auto store32=[&](unsigned addr,int v){h.write32(v,addr);value(h,v);};
    auto ret=[&]{--h.indent;h.clockI(2);next=h.pcoff(h.popPC());};
    auto call=[&](unsigned target){h.pushPC(h.makepc(next));++h.indent;if(length==4)h.clockI(2);next=target;};
    auto& r=h.regs;
    switch(pc){
    case 0x2de8:mov8(r[0].rl,0);break;
    case 0x2dea:store8(0x15c15,r[0].rl);break;
    case 0x2df0:mov8(r[0].rl,r[6].rl);break;
    case 0x2df2:if(!(h.ccr&8))next=0x2e52;break;
    case 0x2df4:store8(0x15c1c,r[6].rl);break;
    case 0x2dfa:mov8(r[0].rh,0);break;
    case 0x2dfc:store8(0x1603a,r[0].rh);break;
    case 0x2e02:call(0x2ed4);break;
    case 0x2e06:mov8(r[1].rl,h.read8(r[0].er+4));break;
    case 0x2e0e:store8(0x15c1e,r[1].rl);break;
    case 0x2e14:mov8(r[1].rh,h.read8(r[0].er+5));break;
    case 0x2e1c:r[1].rh&=2;value(h,r[1].rh);break;
    case 0x2e1e:if(!(h.ccr&4))throw std::runtime_error("Unsupported native KS MIDI descriptor");break;
    case 0x2e20:store8(0x15c1d,r[1].rl);break;
    case 0x2e26:if(h.ccr&4)throw std::runtime_error("Unsupported native KS MIDI length");break;
    case 0x2e28:store8(Status,r[6].rl);break;
    case 0x2e2e:r[6].rl&=15;value(h,r[6].rl);break;
    case 0x2e30:store8(0x15c1a,r[6].rl);break;
    case 0x2e36:mov32(r[0].er,Data);break;
    case 0x2e3c:store32(0x15c16,r[0].er);break;
    case 0x2e44:ret();decoding=false;break;
    case 0x2e52:mov8(r[0].rl,h.read8(0x15c1d));break;
    case 0x2e58:if(h.ccr&4)next=0x2e78;break;
    case 0x2e5a:mov32(r[1].er,h.read32(0x15c16));break;
    case 0x2e62:h.write8(r[6].rl,r[1].er);value(h,r[6].rl);break;
    case 0x2e64:{const int old=r[0].rl,v=old-1;value(h,v);flag(h,2,(old&~v)&128);r[0].rl=int8_t(v);break;}
    case 0x2e66:store8(0x15c1d,r[0].rl);break;
    case 0x2e6c:if(h.ccr&4)next=0x2e7a;break;
    case 0x2e6e:{const uint32_t old=uint32_t(r[1].er),v=old+1;value(h,int32_t(v));flag(h,2,(~old&v)&0x80000000u);r[1].er=int32_t(v);break;}
    case 0x2e70:store32(0x15c16,r[1].er);break;
    case 0x2e78:ret();decoding=false;break;
    case 0x2e7a:mov8(r[0].rl,h.read8(Status));break;
    case 0x2e80:store8(0x15c22,r[0].rl);break;
    case 0x2e86:call(0x2ed4);break;
    case 0x2e88:mov8(r[1].rl,h.read8(r[0].er+5));break;
    case 0x2e90:r[1].rl&=1;value(h,r[1].rl);break;
    case 0x2e92:if(h.ccr&4)next=0x2eb0;break;
    case 0x2e94:mov8(r[1].rl,h.read8(r[0].er+4));break;
    case 0x2e9c:store8(0x15c1d,r[1].rl);break;
    case 0x2ea2:mov32(r[1].er,Data);break;
    case 0x2ea8:store32(0x15c16,r[1].er);break;
    case 0x2eb0:mov32(r[0].er,h.read32(r[0].er));break;
    case 0x2eba:mov32(r[4].er,Data);break;
    case 0x2ec0:mov8(r[1].rl,h.read8(0x15c1a));break;
    case 0x2ec6:store8(0x15c21,r[1].rl);break;
    case 0x2ecc:{
        ++messages;const uint8_t status=h.memory[0xa00000+Status];
        if((status&0xf0)==0xb0)++controls;
        else if((status&0xf0)==0x80 || !h.memory[0xa00000+Data+1])++noteOff;
        else ++noteOn;
        call(0x328e);decoding=false;break; // Original channel/voice/CC handlers.
    }
    case 0x2ed4:h.sub8(r[0].rl,0xf0);break;
    case 0x2ed6:if(!(h.ccr&1))throw std::runtime_error("System MIDI reached native KS decoder");break;
    case 0x2ed8:r[0].rl&=0x70;value(h,r[0].rl);break;
    case 0x2eda:case 0x2edc:{auto v=uint8_t(r[0].rl);flag(h,1,v&2);r[0].rl=int8_t(v>>2);value(h,r[0].rl);break;}
    case 0x2ede:mov8(r[0].rh,6);break;
    case 0x2ee0:r[0].r=int16_t(uint8_t(r[0].rl)*uint8_t(r[0].rh));h.clockI(12);break;
    case 0x2ee2:r[0].er&=0xffff;value(h,r[0].er);break;
    case 0x2ee4:r[0].er=h.add32(r[0].er,0xf250);break;
    case 0x2eea:ret();break;
    }
    h.pc=h.makepc(next);++nativeSteps;return true;
}
}
