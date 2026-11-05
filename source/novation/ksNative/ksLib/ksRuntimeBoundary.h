#pragma once
#include "ksControllerBoundary.h"

namespace ks::controllerDetail {
// Arithmetic/control helpers for the native compatibility port. Preserve the
// existing H8 reference's flags (including its quirks), not a revised ISA model.
struct RuntimeBoundary : Boundary {
    using Boundary::Boundary;
    void rte(){--h.indent;h.clockI(2);if(h.interruptMode2)h.exr=uint8_t(h.popW()>>8);next=h.pcoff(h.popPC(true));}
    void jump(unsigned target,bool direct){if(direct)h.clockI(2);next=target;}
    template<class T>void arithmeticRight(T& dst,unsigned n){int32_t v=dst;flag(h,1,v&(1u<<(n-1)));v>>=n;value(h,v);dst=T(v);}
    template<class T>void arithmeticLeft(T& dst,unsigned n){constexpr unsigned bits=sizeof(T)*8;uint32_t v=uint32_t(dst);const auto top=v>>(bits-1-n);flag(h,2,top!=0 && top!=((2u<<n)-1));flag(h,1,v&(1u<<(bits-n)));v<<=n;flag(h,4,!v);flag(h,8,v&(1u<<(bits-1)));dst=T(v);}
    template<class T>void rotate(T& dst,unsigned n,bool left,bool carry){
        constexpr unsigned bits=sizeof(T)*8;uint32_t v=typename std::make_unsigned<T>::type(dst);
        if(carry)for(unsigned i=0;i<n;++i){const bool old=h.ccr&1;
            const auto out=left?(v>>(bits-1))&1:v&1;
            v=left?(v<<1)|unsigned(old):(v>>1)|(unsigned(old)<<(bits-1));flag(h,1,out);
        }else if(left){auto out=v>>(bits-n);v=(v<<n)|out;flag(h,1,out&1);}
        else {auto out=v&((1u<<n)-1);v=(v>>n)|(out<<(bits-n));flag(h,1,out&(1u<<(n-1)));}
        value(h,int32_t(v<<(32-bits)));dst=T(v);
    }
    template<class T>void multiplySigned(int src,T& dst){
        if constexpr(sizeof(T)==2)dst=T(int8_t(src)*int8_t(dst));
        else dst=T(int16_t(src)*int32_t(int16_t(dst)));
        flag(h,8,dst<0);flag(h,4,!dst);h.clockI(sizeof(T)==2?12:20);
    }
    template<class T>void divideUnsigned(unsigned src,T& dst){
        constexpr bool word=sizeof(T)==4;const uint32_t divisor=word?uint16_t(src):uint8_t(src);
        const uint32_t numerator=word?uint32_t(dst):uint16_t(dst),q=divisor?numerator/divisor:0;
        flag(h,8,divisor&(word?0x8000:0x80));flag(h,4,!divisor);
        const uint32_t rem=numerator-q*divisor;
        if constexpr(word)dst=T((rem<<16)|uint16_t(q));else dst=T((rem<<8)|uint8_t(q));
        h.clockI(word?20:12);
    }
    template<class T>void divideSigned(int src,T& dst){
        constexpr bool word=sizeof(T)==4;const int32_t divisor=word?int16_t(src):int8_t(src),numerator=dst;
        flag(h,4,!divisor);h.clockI(word?20:12);
        if(word && !divisor && numerator)return; // Reference's zero-divisor early return.
        const int64_t q=divisor?int64_t(numerator)/divisor:(word?1:0),rem=int64_t(numerator)-q*divisor;
        if constexpr(word)dst=T((uint32_t(rem)<<16)|uint16_t(q));else dst=T((uint32_t(rem)<<8)|uint8_t(q));
        flag(h,8,(uint32_t(divisor)^uint32_t(numerator))&(word?0x8000:0x80));
    }
};
}
