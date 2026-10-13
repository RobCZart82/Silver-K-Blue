#include "h8s/h8s.hpp"
#include <memory>
#include <stdexcept>
#include <iostream>
void require(bool v){if(!v)throw std::runtime_error("H8S mode-2 instruction test failed");}
int main(){auto c=std::make_unique<H8SEmulator>();c->write32(0x400,0);c->write32(0x500,89*4);c->write32(0x600,32*4);c->memory[0x400]=0;c->memory[0x401]=0;c->memory[0x500]=0;c->memory[0x501]=0;c->memory[0x502]=0x56;c->memory[0x503]=0x70;c->memory[0x600]=0x56;c->memory[0x601]=0x70;c->boot();c->setSP(0xfffc00);c->interruptMode2=true;c->exr=0;c->interruptPriority[89]=1;c->interruptPriority[32]=6;c->interrupt(89);require(c->pending_irqs_high==(1ULL<<25));c->step();require(c->getPC()==0x502 && c->getSP()==0xfffbfa && c->exr==1);c->interrupt(32);c->step();require(c->getPC()==0x502 && c->getSP()==0xfffbfa && c->exr==1);c->step();require(c->getPC()==0x400 && c->getSP()==0xfffc00 && c->exr==0 && c->ccr==128);c->exr=7;c->interrupt(89);c->step();require(c->getPC()==0x402 && c->pending_irqs_high);c->boot();require(!c->pending_irqs_high);std::cout<<"mode2 high IRQ, nested priority, mask and RTE frame passed\n";}
