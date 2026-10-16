#include "ksControlArithmetic.h"
#include "h8s/h8s.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>

namespace {
// This target alone includes the original interpreter. Calling its original
// arithmetic directly avoids testing the new adapter against itself.
struct Oracle final:H8SEmulator {
    using H8SEmulator::add8;using H8SEmulator::sub8;
    using H8SEmulator::add16;using H8SEmulator::sub16;
    using H8SEmulator::add32;using H8SEmulator::sub32;
};
uint64_t cases=0;
template<class T>void check(ks::controlArithmetic::Result<T> actual,T expected,uint8_t flags){
    ++cases;
    if(actual.value!=expected || actual.flags!=flags){
        std::cerr<<"case="<<cases<<" width="<<sizeof(T)*8<<" result="<<int64_t(actual.value)<<" expected="<<int64_t(expected)
                 <<" status="<<unsigned(actual.flags)<<" expected="<<unsigned(flags)<<'\n';
        throw std::runtime_error("Independent control arithmetic differs from original H8");
    }
}
}
int main(){try{
    auto oracle=std::make_unique<Oracle>();
    // Exhaust every byte operand pair and every incoming status. Carry is
    // masked to one bit in the reference; sticky zero is covered independently.
    for(unsigned a=0;a<256;++a)for(unsigned b=0;b<256;++b)for(unsigned flags=0;flags<256;++flags)
    for(unsigned carry=0;carry<4;++carry)for(bool sticky:{false,true}){
        oracle->ccr=uint8_t(flags);const auto sum=oracle->add8(int8_t(a),int8_t(b),int(carry),sticky);
        check(ks::controlArithmetic::add(int8_t(a),int8_t(b),uint8_t(flags),carry,sticky),sum,oracle->ccr);
        oracle->ccr=uint8_t(flags);const auto difference=oracle->sub8(int8_t(a),int8_t(b),int(carry),sticky);
        check(ks::controlArithmetic::subtract(int8_t(a),int8_t(b),uint8_t(flags),carry,sticky),difference,oracle->ccr);
    }
    const auto byteCases=cases;
    // Every 16-bit left operand against sign, half-carry and wrap boundaries.
    constexpr int16_t right[]{0,1,-1,7,8,15,16,0x7ff,0x800,0xfff,0x1000,0x3fff,0x4000,0x7ffe,0x7fff,-32768,-32767,-2048,-2049};
    for(unsigned a=0;a<65536;++a)for(auto b:right)for(unsigned flags:{0u,4u,0x2fu,0xd0u,0xffu}){
        oracle->ccr=uint8_t(flags);const auto sum=oracle->add16(int16_t(a),b);
        check(ks::controlArithmetic::add(int16_t(a),b,uint8_t(flags)),sum,oracle->ccr);
        oracle->ccr=uint8_t(flags);const auto difference=oracle->sub16(int16_t(a),b);
        check(ks::controlArithmetic::subtract(int16_t(a),b,uint8_t(flags)),difference,oracle->ccr);
    }
    const auto wordCases=cases-byteCases;
    constexpr int32_t edges[]{0,1,-1,7,8,15,16,0x7ff,0x800,0x7ffff,0x80000,0x7ffffff,0x8000000,
        0x3fffffff,0x40000000,0x7ffffffe,0x7fffffff,INT32_MIN,INT32_MIN+1,-0x8000000,-0x8000001};
    auto wide=[&](int32_t a,int32_t b,uint8_t flags){
        oracle->ccr=flags;const auto sum=oracle->add32(a,b);
        check(ks::controlArithmetic::add(a,b,flags),sum,oracle->ccr);
        oracle->ccr=flags;const auto difference=oracle->sub32(a,b);
        check(ks::controlArithmetic::subtract(a,b,flags),difference,oracle->ccr);
    };
    for(auto a:edges)for(auto b:edges)for(unsigned flags=0;flags<256;++flags)wide(a,b,uint8_t(flags));
    std::mt19937 random(0x4b53414c);
    for(unsigned n=0;n<1000000;++n){const auto a=int32_t(random()),b=int32_t(random());wide(a,b,uint8_t(random()));}
    const auto longCases=cases-byteCases-wordCases;
    std::cout<<"PASS: "<<cases<<" independent arithmetic cases (8-bit="<<byteCases<<", 16-bit="<<wordCases<<", 32-bit="<<longCases
             <<"); values, all status bits, sticky zero, carry masking, signed/half-carry/overflow edges exact\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
