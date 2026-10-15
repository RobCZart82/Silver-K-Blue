// Offline validation: boot the original KS controller + both DSPs and play
// an external wave bank via the original UART parser. No guest firmware patches.
#include "ksLib/ksBoard.h"
#include "ksLib/ksRom.h"
#include "ksJucePlugin/ksControls.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace {
constexpr unsigned Rate=45340, Release=8192;
std::vector<uint8_t> read(const char* path) {
    std::ifstream file(path,std::ios::binary);
    if(!file)throw std::runtime_error(std::string("Cannot read ")+path);
    return {(std::istreambuf_iterator<char>(file)),{}};
}
void little(std::ostream& out,uint32_t v,unsigned n){for(unsigned i=0;i<n;++i)out.put(char(v>>(8*i)));}
void header(std::ostream& out,unsigned frames) {
    out.write("RIFF",4);little(out,36+frames*6,4);out.write("WAVEfmt ",8);
    little(out,16,4);little(out,1,2);little(out,2,2);little(out,Rate,4);
    little(out,Rate*6,4);little(out,6,2);little(out,24,2);
    out.write("data",4);little(out,frames*6,4);
}
struct Meter {
    double peak=0,square=0,dc=0;uint64_t count=0,clipped=0;
    void add(int32_t raw){double v=double(raw)/8388608.;peak=std::max(peak,std::abs(v));square+=v*v;dc+=v;++count;if(raw<=-8388608 || raw>=8388607)++clipped;}
    double rms()const{return count?std::sqrt(square/count):0;}
};
void send(ks::Board& board,const std::vector<uint8_t>& bytes){if(!board.enqueueMidi(bytes.data(),bytes.size()))throw std::runtime_error("MIDI overflow");}
void settle(ks::Board& board,unsigned samples){for(unsigned i=0;i<samples;++i)board.renderSample();}
ks::Program patch(unsigned wave) {
    ks::Program p{};
    for(const auto& c:ksPlugin::controls)if(c.patchOffset>=0)
        p[c.patchOffset]=uint8_t((p[c.patchOffset]&~c.patchMask)|((c.defaultValue<<c.patchShift)&c.patchMask));
    p[128]=uint8_t(wave);p[129]=p[130]=0;
    p[37]=127;p[38]=p[39]=p[40]=p[41]=p[42]=0;
    p[44]=0;p[46]=127;p[62]=p[63]=p[65]=0;p[64]=127;
    p[100]=p[107]=p[110]=p[125]=0;
    p[179]=127;p[180]=p[181]=p[182]=p[186]=0;
    p[183]=127;p[184]=p[185]=64;
    return p;
}
}
int main(int argc,char** argv) {
    try {
        if(argc<5 || argc>9){std::cerr<<"Usage: ks_wave_rom_probe OS.syx BANK.bin FLASH.bin OUTPUT [FIRST_WAVE [LAST_WAVE [NOTES_COMMA_SEPARATED [HOLD_FRAMES]]]]\n";return 2;}
        unsigned first=argc>5?unsigned(std::stoul(argv[5])):0,last=argc>6?unsigned(std::stoul(argv[6])):31;
        if(first>last || last>31)throw std::runtime_error("Wave range must be within 0..31");
        std::vector<unsigned> notes{24,36,48,60,72};
        if(argc>7){notes.clear();std::istringstream values(argv[7]);std::string value;while(std::getline(values,value,',')){auto note=unsigned(std::stoul(value));if(note>127)throw std::runtime_error("Invalid MIDI note");notes.push_back(note);}if(notes.empty())throw std::runtime_error("Empty note list");}
        const unsigned hold=argc>8?unsigned(std::stoul(argv[8])):16384;
        if(!hold || hold>Rate*10)throw std::runtime_error("Hold frames must be 1..453400");
        ks::Rom os(argv[1]);if(!os.isValid())throw std::runtime_error(os.error());
        auto bank=read(argv[2]),flash=read(argv[3]);
        if(bank.size()!=ks::Board::WaveSize || flash.size()!=0x80000)throw std::runtime_error("Invalid wave/Flash size");
        auto board=std::make_unique<ks::Board>(os.data(),bank,flash);
        if(!board->boot())throw std::runtime_error("Original KS boot failed");
        std::filesystem::path output(argv[4]);std::filesystem::create_directories(output);
        std::ofstream csv(output/"measurements.csv");
        csv<<"wave,name,note,sample_rate,held_frames,held_rms,held_peak,held_dc,held_clipped,voice_rms,release_rms,release_peak,last_release_rms\n";
        std::cout<<"original_boot=1 zero_bank="<<board->zeroBank()<<" wave_reads="<<board->waveReads<<" low=0x"<<std::hex<<board->bankLow<<" high=0x"<<board->bankHigh<<std::dec<<std::endl;
        for(unsigned wave=first;wave<=last;++wave){
            send(*board,{0xb0,120,0});settle(*board,4096);
            ks::State state;state.parts[0]=patch(wave);send(*board,ks::stateMessage(state));
            send(*board,{0xb0,7,127});settle(*board,8192);
            for(unsigned note:notes){
                send(*board,{0xb0,120,0});settle(*board,4096);
                send(*board,{0x90,uint8_t(note),100});settle(*board,1024);
                const auto name="wave-"+std::to_string(wave)+"-note-"+std::to_string(note)+".wav";
                std::ofstream wav(output/name,std::ios::binary);header(wav,hold+Release);
                Meter held,voice,released,lastRelease;
                for(unsigned n=0;n<hold+Release;++n){
                    if(n==hold)send(*board,{0x80,uint8_t(note),0});
                    auto frame=board->renderSample();
                    for(unsigned ch=0;ch<2;++ch){little(wav,uint32_t(frame[ch]),3);(n<hold?held:released).add(frame[ch]);if(n>=hold+Release-4096)lastRelease.add(frame[ch]);}
                    if(n<hold)voice.add(frame[19]);
                }
                if(!wav)throw std::runtime_error("WAV write failed");
                csv<<wave<<','<<ksPlugin::Wave[wave]<<','<<note<<','<<Rate<<','<<hold<<','<<held.rms()<<','<<held.peak<<','<<held.dc/held.count<<','<<held.clipped<<','<<voice.rms()<<','<<released.rms()<<','<<released.peak<<','<<lastRelease.rms()<<'\n';
                std::cout<<"wave="<<wave<<" note="<<note<<" rms="<<held.rms()<<" peak="<<held.peak<<" voice="<<voice.rms()<<" tail="<<lastRelease.rms()<<std::endl;
            }
        }
        std::ofstream telemetry(output/"telemetry.txt");
        telemetry<<"sample_rate="<<Rate<<"\noriginal_boot=1\nzero_bank="<<board->zeroBank()<<"\nwave_reads="<<board->waveReads<<"\nwave_low=0x"<<std::hex<<board->bankLow<<"\nwave_high=0x"<<board->bankHigh<<std::dec<<"\nmidi_received="<<board->midiReceived<<"\n";
        telemetry<<"serial_underruns="<<board->serialUnderruns[0]<<','<<board->serialUnderruns[1]<<"\nserial_overflows="<<board->serialOverflows[0]<<','<<board->serialOverflows[1]<<"\n";
        if(!csv || !telemetry)throw std::runtime_error("Metrics write failed");
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
