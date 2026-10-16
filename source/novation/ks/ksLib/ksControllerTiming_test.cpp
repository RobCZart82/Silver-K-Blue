#include "ksControllerBoundary.h"
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <tuple>
#include <vector>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Bus final:H8SDevice{
    std::vector<std::tuple<unsigned,uint64_t,unsigned>> reads;
    Bus(){reads.reserve(64);}
    uint8_t read(uint32_t a)override{reads.emplace_back(a,state->cycles,state->getPC());return uint8_t(a*17+3);}
    void write(uint32_t,uint8_t)override{throw std::runtime_error("Unexpected code-fetch write");}
};
}
int main(){try{
    auto original=std::make_unique<ks::ControllerCpu>(),native=std::make_unique<ks::ControllerCpu>();
    Bus x,y;original->memmap(&x,0,1<<24);native->memmap(&y,0,1<<24);
    std::mt19937 random(0x4b534354);uint64_t cases=0;
    for(bool nativeFetch:{false,true})
    for(unsigned pc:{0u,1u,2u,0x4000u,0x4001u,0x3ffffcu,0x3ffffdu,0x3ffffeu,0x3fffffu,
                     0x400000u,0x600000u,0xfffd0fu,0xfffd10u,0xffff9eu,0xffff9fu,0xffffa0u,0xffffffu,0xffffec00u})
    for(unsigned length:{0u,1u,2u,3u,4u,6u,8u,10u,12u,64u})for(unsigned trial=0;trial<32;++trial){
        const int histories[]{-1,int(pc)-1,int(pc),int(pc)+1,0,0xffffff,int(random()),int(random())};
        for(auto* h:{original.get(),native.get()}){
            h->pc=h->makepc(pc);h->cycles=1000;h->lastread=histories[trial%8];h->lastwrite=-1;
            h->nativeFetchClocksOnly=nativeFetch;
        }
        x.reads.clear();y.reads.clear();
        // Actual original read8 masks physical addresses to 24 bits before
        // bus timing. Preserve its signed reset-trampoline PC independently
        // from that physical bus address; keep old zero-length behavior.
        original->read8(pc);
        original->pc=original->makepc(pc+1);
        for(unsigned i=1;i<length;++i)original->read8(pc+i);
        ks::controllerDetail::Boundary boundary(*native,pc,length);
        check(original->cycles==native->cycles,"Code-fetch clocks differ");
        check(original->getPC()==native->getPC() && original->lastread==native->lastread
            && original->lastwrite==native->lastwrite,"Code-fetch PC/bus history differs");
        if(nativeFetch)check(y.reads.empty(),"Clock-only fetch made a data callback");
        else check(x.reads==y.reads,"Code-fetch callback address/cycle/in-flight PC differs");
        check(boundary.next==pc+length && boundary.length==length,"Code-fetch continuation differs");
        ++cases;
    }
    check(cases==11520,"Code-fetch timing coverage differs");
    std::cout<<"PASS: "<<cases<<" code-fetch clock/PC/history/callback cases; native and mapped fetches, alignment, boundary crossings, zero/odd lengths, off-chip/on-chip/24-bit wrap fallbacks exact\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
