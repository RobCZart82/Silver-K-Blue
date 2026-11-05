#pragma once
#include "ksLiveMidiController.h"
namespace ks::controllerDetail {
inline bool canDeferInterrupts(const ControllerCpu& h){
    if(!h.interruptMode2)return false;
    const unsigned mask=h.exr&7;
    for(unsigned base:{0u,64u}){
        auto bits=base?h.pending_irqs_high:h.pending_irqs;
        while(bits){
#if defined(__clang__) || defined(__GNUC__)
            const unsigned bit=unsigned(__builtin_ctzll(bits));
#else
            auto word=bits;unsigned bit=0;while(!(word&1)){++bit;word>>=1;}
#endif
            if(h.interruptPriority[base+bit]>mask)return false;
            bits&=bits-1;
        }
    }
    return true;
}
}
