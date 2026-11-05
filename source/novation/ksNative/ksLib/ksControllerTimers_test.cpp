#include "ksControllerTimers.h"
#include <iostream>
#include <random>
#include <stdexcept>

namespace {
void compare(uint16_t initial,uint16_t limit,uint64_t elapsed,bool clear){
    unsigned count=initial;bool matched=false;
    for(uint64_t n=0;n<elapsed;++n){if(++count==limit){matched=true;if(clear)count=0;}count&=65535;}
    const auto result=ks::advanceControllerTimer(initial,limit,elapsed,clear);
    if(result.count!=count || result.matched!=matched)throw std::runtime_error("Native TPU0 closed-form result differs from original loop");
}
}
int main(){try{
    uint64_t cases=0;
    for(unsigned initial:{0,1,2,127,12500,32767,65534,65535})
        for(unsigned limit:{0,1,2,128,12500,32768,65535})
            for(unsigned elapsed:{0,1,2,3,127,128,256,65535,65536,65537,131073})
                for(bool clear:{false,true}){compare(uint16_t(initial),uint16_t(limit),elapsed,clear);++cases;}
    std::mt19937 random(0x4b535450);
    for(unsigned n=0;n<100000;++n){compare(uint16_t(random()),uint16_t(random()),random()%257,bool(random()&1));++cases;}
    std::cout<<"PASS: "<<cases<<" native timer cases vs original loop, including wrap/compare-zero/clear/multiple matches\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
