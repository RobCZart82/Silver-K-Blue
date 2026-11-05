#include "ksControllerState.h"
#include "h8s/h8s.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {struct Original final:H8SEmulator {using H8SEmulator::lastread;using H8SEmulator::lastwrite;};}
int main(){try{
    if(ks::controllerReferenceAvailable())throw std::runtime_error("Reset-vector oracle must not install the synchronized test adapter");
    auto reference=std::make_unique<Original>();auto native=std::make_unique<ks::ControllerCpu>();
    for(unsigned n=0;n<8;++n)reference->regs[n].er=native->regs[n].er=int32_t(0x12345678u+n*0x1020304u);
    uint64_t cases=0;
    // Original boot() itself admits the independent starting state used by the
    // instruction oracle. Exhaust the 24-bit reset target and vary upper vector
    // bits, previous mask/status, cycles, pending IRQs and interrupt mode.
    for(uint32_t target=0;target<0x1000000;++target){
        const uint32_t encoded=target|(((target*17u)&0xffu)<<24);
        for(unsigned byte=0;byte<4;++byte)reference->memory[byte]=native->memory[byte]=uint8_t(encoded>>(24-byte*8));
        reference->ccr=native->ccr=uint8_t(target);reference->exr=native->exr=uint8_t(target>>8);
        reference->cycles=native->cycles=uint64_t(target)+123;
        reference->pending_irqs=native->pending_irqs=~uint64_t(target);
        reference->pending_irqs_high=native->pending_irqs_high=~uint64_t(target*3u);
        reference->interruptMode2=native->interruptMode2=bool(target&1);
        reference->boot();native->boot();
        if(uint32_t(reference->getPC())!=target || uint32_t(native->getPC())!=target
            || reference->cycles!=native->cycles || native->cycles || reference->ccr!=native->ccr || native->ccr!=128
            || reference->exr!=native->exr || native->exr || reference->pending_irqs!=native->pending_irqs || native->pending_irqs
            || reference->pending_irqs_high!=native->pending_irqs_high || native->pending_irqs_high
            || reference->lastread!=native->lastread || reference->lastwrite!=native->lastwrite
            || reference->interruptMode2!=native->interruptMode2)
            throw std::runtime_error("Independent original reset-vector admission differs");
        ++cases;
    }
    for(unsigned n=0;n<8;++n)if(reference->regs[n].er!=native->regs[n].er || uint32_t(native->regs[n].er)!=0x12345678u+n*0x1020304u)
        throw std::runtime_error("Reset changed retained working values");
    if(native->interpretedSteps)throw std::runtime_error("Reset-vector initialization called the interpreter");
    std::cout<<"PASS: "<<cases<<" independent original-H8 reset-vector initializations; target masking/status/IRQ clearing/clocks/bus history and retained values exact\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
