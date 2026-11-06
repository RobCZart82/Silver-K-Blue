#include "ksLib/ksBoard.h"
#include "ksLib/ksDevice.h"
#include <array>
#include "ksLib/ksRom.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <cmath>
#include <filesystem>
// Developer-only private-fixture probe; outputs are not release artifacts.
void check(bool x,const char*m){if(!x)throw std::runtime_error(m);}
std::vector<uint8_t> read(const char*p){std::ifstream f(p,std::ios::binary);check(bool(f),"fixture missing");return {std::istreambuf_iterator<char>(f),{}};}
void save(const std::string&p,const std::vector<uint8_t>&v){std::ofstream f(p,std::ios::binary);f.write((const char*)v.data(),v.size());check(bool(f),"write failed");}
// Start each group from stored Performance data, before sending any host MIDI.
void coldChannels(const std::vector<uint8_t>& os, const std::vector<uint8_t>& wave,
                  const std::vector<uint8_t>& originalFlash)
{
 constexpr unsigned performanceOffset = 0x45800;
 for (unsigned group = 0; group < 4; ++group) {
  auto flash = originalFlash;
  flash[0x1ff01] = 0;
  flash[0x1ff04] = 1;
  for (unsigned part = 0; part < 4; ++part) {
   auto* program = flash.data() + performanceOffset + part * 256;
   std::copy_n(originalFlash.data() + 0x2c800, 256, program);
   for (unsigned index : {38, 39, 40, 41, 62, 65, 100, 107, 110})
    program[index] = 0;
   program[37] = program[179] = 127;
   program[128] = 0; // Saw wave works with a zero Wave ROM.
   program[180] = 0;
   program[181] = uint8_t(group * 4 + part);
   program[182] = 0;
   program[183] = 127;
   program[184] = program[185] = 64;
  }
  flash[performanceOffset + 189] = 1;
  flash[performanceOffset + 190] = 15;
  auto board = std::make_unique<ks::Board>(os, wave, flash, true, false);
  check(board->boot(), "cold Performance boot failed");
#ifdef KS_SEMANTIC_PROBE
  check(board->nativeOwnerState() && board->h8InterpretedSteps() == 0,
        "cold Performance did not use the semantic owner");
#endif
  const auto startup = board->state();
  check(startup.performance && startup.activeMask == 15, "cold Performance parts not active");
  auto send = [&](std::initializer_list<uint8_t> data) {
   check(board->enqueueMidi(data.begin(), data.size()), "cold Performance MIDI overflow");
  };
  auto peak = [&](unsigned frames, unsigned skip = 0) {
   int64_t result = 0;
   for (unsigned n = 0; n < frames; ++n) {
    const auto frame = board->renderSample();
    if (n >= skip)
     for (unsigned c = 0; c < 4; ++c)
      result = std::max(result, std::abs(int64_t(frame[c])));
   }
   return result;
  };
  check(peak(8192) == 0, "cold Performance produced audio before host MIDI");
  for (unsigned part = 0; part < 4; ++part) {
   const auto channel = uint8_t(group * 4 + part);
   check((startup.parts[part][181] & 15) == channel, "cold Performance channel mismatch");
   const auto cc = uint8_t(0xb0 | channel);
   send({cc, 64, 127, uint8_t(0x90 | channel), 60, 100});
   const auto played = peak(16384, 8192);
   send({uint8_t(0x80 | channel), 60, 0});
   const auto held = peak(16384, 8192);
   send({cc, 64, 0});
   const auto released = peak(32768, 24576);
   std::cout << "cold part=" << part + 1 << " channel=" << int(channel) + 1
             << " played=" << played << " held=" << held << " released=" << released << '\n';
   check(played > 1000 && held > 1000, "cold Performance sustain control was silent");
   check(released == 0, "cold Performance retained audio after pedal release");
  }
#ifdef KS_SEMANTIC_PROBE
  check(board->h8InterpretedSteps() == 0, "cold Performance interpreted H8");
#endif
 }
 std::cout << "PASS: cold Performance across four parts and 16 MIDI channels\n";
}
// Compare the host-rate Device path without Processor parameter traffic.
void deviceRelease(const std::vector<uint8_t>& os, const std::vector<uint8_t>& wave,
                   const std::vector<uint8_t>& flash, const std::string& prefix,
                   unsigned block, unsigned leadFloor, unsigned noteOnDelay, unsigned noteOffDelay,
                   double rate)
{
 const auto path = prefix + ".pcm";
 check(!std::filesystem::exists(path), "output exists; use a new private prefix");
 ks::Device device(os, wave, flash, rate, int(block), {}, nullptr, ks::ControllerMode::Native);
 device.setBufferFloor(leadFloor);
 std::array<std::vector<float>, ks::OutputCount> samples;
 std::array<float*, ks::OutputCount> outputs{};
 for (unsigned c = 0; c < outputs.size(); ++c) {
  samples[c].resize(block);
  outputs[c] = samples[c].data();
 }
 std::ofstream pcm(path, std::ios::binary);
 check(bool(pcm), "cannot open capture");
 float peak = 0;
 for (unsigned frame = 0; frame < 65536; frame += block) {
  for (const unsigned event : {4800u, 52800u}) {
   if (event >= frame && event < frame + block) {
    const uint8_t message[]{uint8_t(event == 4800 ? 0x90 : 0x80), 60,
                            uint8_t(event == 4800 ? 100 : 0)};
    check(device.midi(message, sizeof(message), event - frame + (event == 4800 ? noteOnDelay : noteOffDelay)), "Device MIDI rejected");
   }
  }
  device.process(outputs, block, true);
  for (unsigned i = 0; i < block; ++i)
   for (const auto& channel : samples) {
    const float value = channel[i];
    check(std::isfinite(value), "Device output is non-finite");
    peak = std::max(peak, std::abs(value));
    pcm.write(reinterpret_cast<const char*>(&value), sizeof(value));
   }
 }
 pcm.close();
 check(bool(pcm) && peak > 1.e-5f && peak <= 16.f && !device.underruns(),
       "Device capture failed audio or underrun checks");
 std::cout << "PASS Device rate=" << rate << " block=" << block << " leadFloor=" << leadFloor << " noteOnDelay=" << noteOnDelay << " noteOffDelay=" << noteOffDelay << " peak=" << peak
           << " backend=" << device.controllerBackend() << '\n';
}
int main(int argc,char**argv){try{check(argc==5 || argc==6,"OS FLASH WAVE PREFIX [single|fourpart|cold-channels|note-release|device-release-128|device-release-512|device-release-128-lead640|device-release-128-delay384|device-release-128-on384|device-release-128-off384|device-release-128-native-rate]");const std::string scenario=argc==6?argv[5]:"single";check(scenario=="single" || scenario=="fourpart" || scenario=="cold-channels" || scenario=="note-release" || scenario=="device-release-128" || scenario=="device-release-512" || scenario=="device-release-128-lead640" || scenario=="device-release-128-delay384" || scenario=="device-release-128-on384" || scenario=="device-release-128-off384" || scenario=="device-release-128-native-rate","unknown scenario");ks::Rom rom(argv[1]);check(rom.isValid(),"invalid OS");auto flash=read(argv[2]),wave=read(argv[3]);
check(flash.size()==0x80000 && wave.size()==ks::Board::WaveSize,"invalid fixture sizes");
if(scenario=="cold-channels"){coldChannels(rom.data(),wave,flash);return 0;}
if(scenario=="device-release-128" || scenario=="device-release-512" || scenario=="device-release-128-lead640" || scenario=="device-release-128-delay384" || scenario=="device-release-128-on384" || scenario=="device-release-128-off384" || scenario=="device-release-128-native-rate"){
 deviceRelease(rom.data(),wave,flash,argv[4],scenario=="device-release-512"?512:128,
               scenario=="device-release-128-lead640"?640:0,
               (scenario=="device-release-128-delay384" || scenario=="device-release-128-on384")?384:0,
               (scenario=="device-release-128-delay384" || scenario=="device-release-128-off384")?384:0,
               scenario=="device-release-128-native-rate"?ks::DSP::SampleRate:48000.0);
 return 0;
}
for(const auto* suffix:{".pcm",".state",".flash",".midi"})check(!std::filesystem::exists(std::string(argv[4])+suffix),"output exists; use a new private prefix");auto board=std::make_unique<ks::Board>(rom.data(),wave,flash,true,false);check(board->boot(),"boot failed");
#ifdef KS_SEMANTIC_PROBE
check(board->nativeOwnerState() && board->h8InterpretedSteps()==0,"not semantic native");
#endif
std::ofstream pcm(std::string(argv[4])+".pcm",std::ios::binary);int64_t peak=0;uint64_t frames=0;
auto send=[&](std::initializer_list<uint8_t> x){std::vector<uint8_t>v(x);check(board->enqueueMidi(v.data(),v.size()),"midi overflow");};
auto render=[&](unsigned n){for(unsigned i=0;i<n;++i){auto a=board->renderSample();for(auto v:a){peak=std::max(peak,std::abs(int64_t(v)));pcm.write((const char*)&v,sizeof(v));}++frames;}};
if(scenario=="fourpart"){
 auto state=board->state();state.performance=true;state.activeMask=15;
 for(unsigned part=0;part<4;++part){state.parts[part]=state.parts[0];state.parts[part][179]=100;state.parts[part][181]=uint8_t(part);}
 state.parts[0][189]=1;state.parts[0][190]=15;
 send({0xb0,32,65,0xc0,0});
 auto message=ks::stateMessage(state);check(board->enqueueMidi(message.data(),message.size()),"state overflow");
 render(16384);
 for(uint8_t part=0;part<4;++part)send({uint8_t(0x90|part),uint8_t(48+7*part),100});
 render(16384);
 for(uint8_t part=0;part<4;++part)send({uint8_t(0xb0|part),120,0});
 render(8192);
 auto current=board->state();check(current.performance && current.activeMask==15,"four-part state not retained");
}else if(scenario=="note-release"){
 render(8192);
 send({0x90,60,100});
 render(48000);
 send({0x80,60,0});
 render(8192);
}else{
render(8192);send({0xb0,7,127,91,0,92,0,93,0,0x90,60,100});render(16384);send({0xb0,64,127,0x80,60,0,0x90,67,96});render(8192);send({0xb0,64,0,120,0});render(8192);}
check(peak>1000 && bool(pcm),"silent or invalid audio");auto state=board->state();save(std::string(argv[4])+".state",ks::stateMessage(state));save(std::string(argv[4])+".flash",board->saveUserFlash());std::vector<uint8_t>midi;uint8_t b;while(board->popMidiOut(b))midi.push_back(b);save(std::string(argv[4])+".midi",midi);
#ifdef KS_SEMANTIC_PROBE
check(board->h8InterpretedSteps()==0,"native interpreted H8");
#endif
std::cout<<"PASS scenario="<<scenario<<" frames="<<frames<<" peak="<<peak<<" backend="<<board->controllerBackend()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
