#include "ksBoard.h"
#include "ksRom.h"
#include "dsp56kBase/threadtools.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace {
using Clock=std::chrono::steady_clock;
void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
std::vector<uint8_t> bytes(const char* path){std::ifstream s(path,std::ios::binary);return {(std::istreambuf_iterator<char>(s)),{}};}
double cpu(){
#ifdef _WIN32
    FILETIME c{},e{},k{},u{};check(GetProcessTimes(GetCurrentProcess(),&c,&e,&k,&u)!=0,"GetProcessTimes failed");
    return double((uint64_t(k.dwHighDateTime)<<32)|k.dwLowDateTime)*1.e-7+double((uint64_t(u.dwHighDateTime)<<32)|u.dwLowDateTime)*1.e-7;
#else
    return 0; // Portability/correctness runs only; do not label as CPU evidence.
#endif
}
struct Stats {
    std::array<std::array<uint64_t,16>,2> reads{},writes{};
    std::array<uint64_t,2> dataLowWrites{},interrupts{};
    explicit Stats(const ks::ControllerTape& tape){for(const auto& e:tape.events){
        ((e.flags&1)?writes:reads)[e.dsp][e.offset]++;
        if((e.flags&3)==3 && e.offset==14)++dataLowWrites[e.dsp];
        if((e.flags&3)==3 && e.offset==2 && (e.value&0x80))++interrupts[e.dsp];
    }}
    std::string json()const{std::ostringstream s;s<<'[';for(unsigned i=0;i<2;++i){if(i)s<<',';
        s<<"{\"dsp\":\""<<(i?'B':'A')<<"\",\"reads_by_register\":[";
        for(unsigned n=0;n<16;++n){if(n)s<<',';s<<reads[i][n];}s<<"],\"writes_by_register\":[";
        for(unsigned n=0;n<16;++n){if(n)s<<',';s<<writes[i][n];}s<<"],\"accepted_low_data_writes\":"<<dataLowWrites[i]<<",\"interrupt_commands\":"<<interrupts[i]<<'}';
    }s<<']';return s.str();}
};
}
int main(int argc,char** argv){try{
    if(argc!=8){std::cout<<"SKIP: KS OS, Flash, wave, report JSON, h8|record|replay|boot|live|live-record, single|multi|midi, trace path required\n";return 77;}
    const std::string mode=argv[5],scenario=argv[6];
    const bool liveMode=mode=="live" || mode=="live-record" || mode=="live-toggle" || mode=="live-toggle-record";
    const bool recording=mode=="record" || mode=="live-record" || mode=="live-toggle-record";
    const bool toggling=mode=="live-toggle" || mode=="live-toggle-record";
    check(mode=="h8" || mode=="record" || mode=="replay" || mode=="boot" || liveMode,"Unknown controller probe mode");
    check(scenario=="single" || scenario=="multi" || scenario=="midi","Unknown controller probe scenario");
    const auto* threads=std::getenv("SILVER_KS_DSP_THREADS");const bool parallel=threads&&std::string(threads)=="2";
    const auto setup=Clock::now();ks::Rom os(argv[1]);check(os.isValid(),"Invalid KS OS");
    auto board=std::make_unique<ks::Board>(os.data(),bytes(argv[3]),bytes(argv[2]),true);
    if(mode=="boot")board->startControllerTrace();
    const auto bootBegin=Clock::now();const double bootCpu=cpu();check(board->boot(),"Original H8 boot failed");
    const double bootCost=cpu()-bootCpu,bootWall=std::chrono::duration<double>(Clock::now()-bootBegin).count();
    auto send=[&](const std::vector<uint8_t>& m){check(board->enqueueMidi(m.data(),m.size()),"MIDI rejected");};
    auto discard=[&](unsigned n){for(unsigned i=0;i<n;++i)board->renderSample();};
    std::shared_ptr<const ks::ControllerTape> tape;
    uint64_t frames=0,hash=14695981039346656037ull;int64_t peak=0;double cost=bootCost,wall=bootWall;
    uint64_t h8Start=board->cpu().cycles;
    if(mode=="boot"){
        tape=board->finishControllerTrace();h8Start=tape->beginCycle;tape->validate();tape->save(argv[7]);
    }else{
        send({0xb0,7,127,108,0,111,0,110,127,91,0,92,0,93,0,72,127,73,0,74,0,105,127});discard(8192);
        // Use original MIDI for setup; no controller replacement during boot.
        if(scenario=="multi"){
            auto state=board->state();state.performance=true;state.activeMask=15;
            for(unsigned i=0;i<4;++i){state.parts[i]=state.parts[0];auto& p=state.parts[i];p[179]=100;p[181]=uint8_t(i);p[182]=0;p[183]=127;}
            state.parts[0][189]=1;state.parts[0][190]=15;
            send({0xb0,32,65,0xc0,0});send(ks::stateMessage(state));discard(24000);
        }
        const unsigned parts=scenario=="multi"?4:1,perPart=8/parts;
        for(unsigned p=0;p<parts;++p)for(unsigned n=0;n<perPart;++n)send({uint8_t(0x90|p),uint8_t(36+7*(p*perPart+n)),90});discard(8192);
        for(unsigned p=0;p<parts;++p){for(unsigned n=0;n<perPart;++n)send({uint8_t(0x80|p),uint8_t(36+7*(p*perPart+n)),0});send({uint8_t(0xb0|p),120,0});}discard(16384);
        if(parallel)board->enableDspWorkers(48000,128);discard(4096);
        struct Event {uint64_t frame;std::vector<uint8_t> bytes;};std::vector<Event> events;
        auto native=[](double seconds){return uint64_t(std::llround(seconds*ks::DSP::SampleRate));};
        frames=native(2.0);
        for(unsigned tick=0;.1+tick*.1<2.;++tick){std::vector<uint8_t> packet;
            // Include sustain and original per-part Program Change/control paths.
            if(tick==4)packet.insert(packet.end(),{0xb0,64,127});
            if(tick==8)packet.insert(packet.end(),{0xc0,1});
            if(tick==12)packet.insert(packet.end(),{0xb0,64,0,105,100});
            for(unsigned p=0;p<parts;++p)for(unsigned n=0;n<perPart;++n){
                if(tick)packet.insert(packet.end(),{uint8_t(0x80|p),uint8_t(36+((tick-1)*7+p*perPart+n)*5%60),0});
                packet.insert(packet.end(),{uint8_t(0x90|p),uint8_t(36+(tick*7+p*perPart+n)*5%60),uint8_t(75+n*3)});
            }
            events.push_back({native(.1+tick*.1),std::move(packet)});
        }
        if(scenario=="midi"){
            events.clear();
            // New, non-replayed MIDI values: partial packets/running status,
            // velocity-zero release, repeated pitches, CCs and fallback paths.
            const std::vector<std::vector<uint8_t>> packets{
                {0x90,53},{97},{57,82,60,71},{0xb0,1,93},{7,83},{11,74},
                {64,127},{0x80,53,31},{0x90,57,0},{0xb0,10,23},{105,91},
                {0x90,60,86,60,109},{0xb0,64,0},{123,0},{0x90,66,111},
                {0xb0,120,0},{0xc0,1},{0x90,41,102},{0xb0,1,0},{7,127},
                {11,127},{10,64},{64,127},{0x90,41,0},{0xb0,64,0},
                {0xf8,0x90,76,99},{0xf1,0,0x90,79,72},{0xb0,123,0},{120,0}};
            for(size_t n=0;n<packets.size();++n)events.push_back({native(.04+n*.06),packets[n]});
        }
        if(liveMode)board->enableLiveMidiController();
        if(recording)board->startControllerTrace();
        if(mode=="replay"){tape=ks::ControllerTape::load(argv[7]);board->startRecordedController(tape);}
        h8Start=board->cpu().cycles;size_t next=0;
        const auto begin=Clock::now();const double before=cpu();
        for(uint64_t n=0;n<frames;++n){
            if(toggling && n==native(.65))board->enableLiveMidiController(false);
            if(toggling && n==native(.95))board->enableLiveMidiController();
            if(mode!="replay")while(next<events.size() && events[next].frame==n)send(events[next++].bytes);
            const auto a=board->renderSample();for(auto v:a){hash^=uint32_t(v);hash*=1099511628211ull;peak=std::max(peak,std::abs(int64_t(v)));}
        }
        cost=cpu()-before;wall=std::chrono::duration<double>(Clock::now()-begin).count();
        check(peak>1000,"Controller probe rendered silence");
        if(recording){tape=board->finishControllerTrace();tape->validate();tape->save(argv[7]);}
        if(liveMode)check(board->liveMidiController()->messages>0,"Live controller handled no MIDI messages");
        if(mode=="replay"){
            check(board->cpu().cycles==h8Start,"Prototype executed H8 instructions");
            check(board->controllerCycles()==tape->endCycle && board->checkedControllerPorts()==tape->events.size(),"Prototype did not consume exact trace");
            const uint8_t note[]{0x90,60,100};check(!board->enqueueMidi(note,3),"Prototype accepted unsupported live MIDI");
        }
    }
    const auto depth=board->serialDepth();std::ostringstream report;report<<std::setprecision(12)
        <<"{\"schema\":\"silver-k.controller-probe.v1\",\"mode\":\""<<mode<<"\",\"scenario\":\""<<scenario<<"\",\"controller_backend\":\""<<board->controllerBackend()
        <<"\",\"dsp_threads\":"<<(parallel?2:1)<<",\"frames\":"<<frames<<",\"audio_hash\":\""<<hash<<"\",\"peak\":"<<peak
        <<",\"cpu_seconds\":"<<cost<<",\"wall_seconds\":"<<wall<<",\"setup_wall_seconds\":"<<std::chrono::duration<double>(Clock::now()-setup).count()-wall
        <<",\"h8_start_cycles\":"<<h8Start<<",\"h8_end_cycles\":"<<board->cpu().cycles<<",\"controller_end_cycles\":"<<board->controllerCycles()
        <<",\"dsp_a_cycles\":"<<board->dsp(0).cycles()<<",\"dsp_b_cycles\":"<<board->dsp(1).cycles()
        <<",\"dsp_a_instructions\":"<<board->dsp(0).instructions()<<",\"dsp_b_instructions\":"<<board->dsp(1).instructions()
        <<",\"serial_depth\":["<<depth[0]<<','<<depth[1]<<"],\"serial_underruns\":["<<board->serialUnderruns[0]<<','<<board->serialUnderruns[1]
        <<"],\"serial_overflows\":["<<board->serialOverflows[0]<<','<<board->serialOverflows[1]<<']'
        <<",\"checked_ports\":"<<board->checkedControllerPorts();
    if(tape)report<<",\"trace_events\":"<<tape->events.size()<<",\"trace_slices\":"<<tape->slices.size()<<",\"trace_digest\":\""<<tape->digest()<<"\",\"bus\":"<<Stats(*tape).json();
    if(const auto* live=board->liveMidiController())report<<",\"native_midi_steps\":"<<live->nativeSteps<<",\"native_midi_bytes\":"<<live->bytes
        <<",\"native_midi_messages\":"<<live->messages<<",\"native_note_on\":"<<live->noteOn<<",\"native_note_off\":"<<live->noteOff<<",\"native_cc\":"<<live->controls;
    report<<",\"notes\":\"Original H8 always boots/prepares. Live mode ports only Note/CC byte decoding; UART/routing/voice/parameter handlers remain H8. Replay is separate finite tape playback, with no live MIDI/state API. CPU counts whole process user+kernel on Windows, no boot/setup/trace file IO. Raw trace may contain private firmware/presets. No complete native-controller saving claimed.\"}";
    std::ifstream existing(argv[4]);check(!existing.good(),"Refusing to overwrite controller report");std::ofstream out(argv[4]);out<<report.str();check(bool(out),"Cannot save controller report");
    std::cout<<report.str()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
