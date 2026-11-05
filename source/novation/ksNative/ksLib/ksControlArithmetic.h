#pragma once
#include <cstdint>
#include <type_traits>

namespace ks::controlArithmetic {
// Pure fixed-width control arithmetic. The retained status byte is the current
// compatibility boundary, not an H8 dependency. Native tasks can use Result
// directly; only the migration adapter writes its flags back to that boundary.
template<class T>struct Result {T value;uint8_t flags;};
template<class T,bool subtract>
inline Result<T> calculate(T lhs,T rhs,uint8_t before,unsigned carry=0,bool keepZero=false){
    static_assert(std::is_signed_v<T> && (sizeof(T)==1 || sizeof(T)==2 || sizeof(T)==4));
    using U=std::make_unsigned_t<T>;
    constexpr unsigned bits=sizeof(T)*8;
    constexpr uint32_t sign=uint32_t(1)<<(bits-1),half=uint32_t(1)<<(bits-5);
    const uint32_t a=U(lhs),b=U(rhs);
    // Unsigned intermediates define wrap at every width, including 32 bits.
    const uint32_t result=subtract?a-b-(carry&1):a+b+(carry&1);
    const U wrapped=U(result);
    const uint32_t carries=subtract?((~a&b)|(~a&result)|(b&result)):
                                     ((a&b)|(a&~result)|(b&~result));
    const uint32_t overflow=subtract?((a&~b&~result)|(~a&b&result)):
                                      ((a&b&~result)|(~a&~b&result));
    uint8_t flags=uint8_t(before&0xd0);
    flags|=uint8_t((carries&sign)?1:0);
    flags|=uint8_t((overflow&sign)?2:0);
    flags|=uint8_t(!wrapped && (!keepZero || (before&4))?4:0);
    flags|=uint8_t((result&sign)?8:0);
    flags|=uint8_t((carries&half)?32:0);
    // Convert through a representable signed value, avoiding an out-of-range
    // unsigned-to-signed conversion even on a different host compiler.
    const int64_t signedValue=(uint32_t(wrapped)&sign)?int64_t(uint64_t(wrapped))-int64_t(uint64_t(1)<<bits):int64_t(wrapped);
    return {T(signedValue),flags};
}
template<class T>inline Result<T> add(T a,T b,uint8_t flags,unsigned carry=0,bool keepZero=false){return calculate<T,false>(a,b,flags,carry,keepZero);}
template<class T>inline Result<T> subtract(T a,T b,uint8_t flags,unsigned carry=0,bool keepZero=false){return calculate<T,true>(a,b,flags,carry,keepZero);}
}
