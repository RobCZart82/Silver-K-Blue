#include "kstationDevice.h"
#include "kstationRomLoader.h"
#include <array>
#include <iostream>
#include <stdexcept>

namespace
{
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Run
{
    kstation::Device& device;
    std::array<float,256> l{},r{};
    synthLib::TAudioInputs input{};
    synthLib::TAudioOutputs output{};
    std::vector<synthLib::SMidiEvent> reply;
    explicit Run(kstation::Device& d):device(d){output[0]=l.data();output[1]=r.data();device.setNonRealtime(true);}
    void settle(unsigned blocks=48)
    {for(unsigned i=0;i<blocks;++i)device.process(input,output,256,{},reply);}
    void ratio(uint8_t value)
    {device.process(input,output,256,{synthLib::SMidiEvent(synthLib::MidiEventSource::Host,0xb0,23,value)},reply);settle();}
    std::array<uint8_t,128> patch()
    {
        std::array<uint8_t,128> p{};std::array<uint8_t,2048>cc{};uint64_t revision=0;
        for(unsigned i=0;i<100;++i){if(device.getControlSnapshot(p,cc,revision))return p;settle(1);}
        throw std::runtime_error("Snapshot unavailable");
    }
};
}
int main(int argc,char**argv)
{
    try
    {
        check(argc==2,"Usage: kstationRatioState_test external-firmware.bin/syx/os");
        kstation::Rom rom(argv[1]);check(rom.isValid(),rom.error().c_str());
        synthLib::DeviceCreateParams params;params.romData=rom.data();params.romName=rom.getFilename();
        kstation::Device a(params),b(params);check(a.isValid()&&b.isValid(),"Device boot failed");
        Run ra(a),rb(b);
        check(a.prepareAudio(256,48000)&&b.prepareAudio(256,48000),"Audio preparation failed");
        for(uint8_t value=0;value<=12;++value)
        {
            ra.ratio(value);auto expected=ra.patch();check(expected[106]==value,"Legal ratio did not reach firmware");
            std::vector<uint8_t> state;check(a.getState(state,synthLib::StateTypeGlobal),"State save failed");
            check(b.setState(state,synthLib::StateTypeGlobal),"Legal ratio state restore failed");rb.settle();
            check(rb.patch()==expected,"Valid ratio state changed program bytes");
        }
        // A state written by the old 0..127 slider, including its boot preset.
        ra.ratio(100);auto expected=ra.patch();check(expected[106]==100,"Invalid old-state fixture failed");
        std::vector<uint8_t> state;check(a.getState(state,synthLib::StateTypeGlobal),"Old state save failed");
        constexpr size_t userFlashStart=0x1fc00;
        state[8+0x20000+106-userFlashStart]=100;
        state[8+0x26400+106-userFlashStart]=100;
        check(b.setState(state,synthLib::StateTypeGlobal),"Old ratio state restore failed");rb.settle();
        expected[106]=0;check(rb.patch()==expected,"Old invalid ratio was not repaired alone to 1/1");
        state.clear();check(b.getState(state,synthLib::StateTypeGlobal),"Repaired state save failed");
        check(state[8+0x20000+106-userFlashStart]==0&&state[8+0x26400+106-userFlashStart]==0,"Boot patch ratio was not repaired");
        std::cout<<"RESULT {\"passed\":true,\"legalRatioRoundTrips\":13,\"invalidOldRatio\":100,\"repairedRatio\":0,\"other127PatchBytesExact\":true,\"bootFlashRatiosRepaired\":true}\n";
        return 0;
    }
    catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
