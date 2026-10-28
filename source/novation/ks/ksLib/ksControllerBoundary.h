#pragma once
#include "ksLiveMidiController.h"
#include <type_traits>

namespace ks::controllerDetail {
inline void flag(ControllerCpu& h,unsigned mask,bool set){h.ccr=uint8_t((h.ccr&~mask)|(set?mask:0));}
inline void value(ControllerCpu& h,int32_t v){h.ccr=uint8_t((h.ccr&~14)|(v<0?8:0)|(!v?4:0));}
// Migration ABI: preserve bus clocks and interruptible boundaries while native
// algorithms and original H8 handlers share controller state. Fetches account
// for bus timing only; this helper never decodes or executes opcodes.
struct Boundary {
    ControllerCpu& h;unsigned next,length;
    Boundary(ControllerCpu& cpu,unsigned pc,unsigned n):h(cpu),next(pc+n),length(n){
        if(h.nativeFetchClocksOnly && n && !(pc&1) && !(n&1)
            && pc<0x400000 && n<=0x400000-pc){
            // Verified aligned low code: each fetched 16-bit word costs three
            // clocks, independently of the preceding read. Fetches have no
            // callbacks in this mode; preserve their exact final bus history.
            h.cycles+=3*(n/2);h.lastread=int(pc+n-1);h.pc=h.makepc(pc+1);
            return;
        }
        if(h.nativeFetchClocksOnly)h.clockMem(pc&0xffffff,h.lastread);else h.read8(pc);
        // The original core exposes entry+1 to mapped devices while its local
        // decoder consumes the remainder. Port records and IO error context
        // must see that same in-flight PC, not just the final boundary PC.
        h.pc=h.makepc(pc+1);
        if(h.nativeFetchClocksOnly)for(unsigned i=1;i<n;++i)h.clockMem((pc+i)&0xffffff,h.lastread);
        else for(unsigned i=1;i<n;++i)h.read8(pc+i);
    }
    template<class T>void move(T& dst,int64_t src){dst=T(src);value(h,dst);}
    void store8(unsigned a,int8_t v){h.write8(v,a);value(h,v);}
    void store16(unsigned a,int16_t v){h.write16(v,a);value(h,v);}
    void store32(unsigned a,int32_t v){h.write32(v,a);value(h,v);}
    template<class T>void push(T v){if constexpr(sizeof(T)==4)h.pushL(uint32_t(v));else h.pushW(uint16_t(v));value(h,v);h.clockI(2);}
    template<class T>void pop(T& v){if constexpr(sizeof(T)==4)v=T(h.popL());else v=T(h.popW());value(h,v);h.clockI(2);}
    template<class T>void inc(T& v,int delta){int32_t old=v,result=int32_t(uint32_t(old)+uint32_t(delta));value(h,result);const uint32_t sign=1u<<(sizeof(T)*8-1);flag(h,2,(delta>0?(~uint32_t(old)&uint32_t(result)):(uint32_t(old)&~uint32_t(result)))&sign);v=T(result);}
    template<class T>void shift(T& v,unsigned n,bool left){constexpr unsigned bits=sizeof(T)*8;uint32_t u=typename std::make_unsigned<T>::type(v);flag(h,1,u&(1u<<(left?bits-n:n-1)));u=left?u<<n:u>>n;value(h,int32_t(u<<(32-bits)));v=T(u);}
    template<class T>void rotateRight(T& v,unsigned n){constexpr unsigned bits=sizeof(T)*8;uint32_t u=typename std::make_unsigned<T>::type(v),low=u&((1u<<n)-1);u=(u>>n)|(low<<(bits-n));flag(h,1,low&(1u<<(n-1)));value(h,int32_t(u<<(32-bits)));v=T(u);}
    void branch(bool take,unsigned target){if(length==4)h.clockI(2);if(take)next=target;}
    void call(unsigned target){h.pushPC(h.makepc(next));++h.indent;if(length==4)h.clockI(2);next=target;}
    void ret(){--h.indent;h.clockI(2);next=h.pcoff(h.popPC());}
};
}
