#include "ksDevice.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include "dsp56kBase/threadtools.h"
namespace ks {
Device::Device(const std::vector<uint8_t>& os,const std::vector<uint8_t>& bank,const std::vector<uint8_t>& flash,double hostRate,int block,const std::vector<uint8_t>& program)
:lead(std::min(4096,std::max(512,block*2+256))),rate(hostRate){
 worker=std::thread([this,os,bank,flash,program]{
  dsp56k::ThreadTools::setCurrentThreadName("KS H8S / dual DSP");
  dsp56k::ThreadTools::setCurrentThreadPriority(dsp56k::ThreadPriority::High);
  try{board=std::make_unique<Board>(os,bank,flash);if(!board->boot())throw std::runtime_error("KS controller boot timeout");
   if(program.size()==270 || program.size()==1038){
    const uint8_t select[]{uint8_t(0xb0|(flash[0x1ff08]&15)),32,uint8_t(program.size()==1038?65:33),uint8_t(0xc0|(flash[0x1ff08]&15)),0};
    board->enqueueMidi(select,sizeof(select));board->enqueueMidi(program.data(),program.size());
    if(program.size()==270){const auto metadata=singleMetadata(program.data()+13,flash[0x1ff08]);board->enqueueMidi(metadata.data(),metadata.size());}
    for(unsigned n=0;n<20000;++n)board->renderSample();}
   // Compile ordinary voice branches before opening audio. Keep warm-up notes
   // out of the effect buffers, then restore the patch through original MIDI.
   const uint8_t channel=flash[0x1ff08]&15;
   const auto originalState=board->state();const auto original=stateMessage(originalState);
   if(originalState.performance){const uint8_t select[]{uint8_t(0xb0|channel),32,33,uint8_t(0xc0|channel),0};board->enqueueMidi(select,sizeof(select));
    State warm=originalState;warm.performance=false;const auto message=stateMessage(warm);board->enqueueMidi(message.data(),message.size());for(unsigned n=0;n<20000;++n)board->renderSample();}
   const uint8_t warmOn[]{uint8_t(0xb0|channel),7,127,uint8_t(0xb0|channel),91,0,uint8_t(0xb0|channel),92,0,uint8_t(0xb0|channel),93,0,
                         uint8_t(0x90|channel),36,64,uint8_t(0x90|channel),69,64,uint8_t(0x90|channel),96,64};
   board->enqueueMidi(warmOn,sizeof(warmOn));for(unsigned n=0;n<8192;++n)board->renderSample();
   // All Sound Off silences voices but leaves the original controller's held
   // keys intact. Pair every warm-up Note On before a mono patch can reuse them.
   const uint8_t warmOff[]{uint8_t(0x80|channel),36,0,uint8_t(0x80|channel),69,0,uint8_t(0x80|channel),96,0,
                           uint8_t(0xb0|channel),120,0};
   board->enqueueMidi(warmOff,sizeof(warmOff));for(unsigned n=0;n<45340;++n)board->renderSample();
   // Compile the shared 16-voice/multitimbral paths even when starting in
   // Single mode. First simultaneous part notes must not trigger a cold JIT
   // burst after audio opens. These are temporary original MIDI programs.
   State warm=originalState;warm.performance=true;warm.activeMask=15;
   for(unsigned part=0;part<PartCount;++part){if(!originalState.performance)warm.parts[part]=originalState.parts[0];auto& p=warm.parts[part];
    p[0]=uint8_t((p[0]&0x60)|0x10);p[65]=0;p[100]=p[107]=p[110]=0;
    p[179]=16;p[180]=part&1;p[181]=uint8_t(part);p[182]=0;p[183]=127;p[184]=p[185]=64;p[168]=0;
    if(board->zeroBank())for(unsigned osc=0;osc<3;++osc)if(p[128+osc]>=8)p[128+osc]=0;
   }
   warm.parts[0][189]=1;warm.parts[0][190]=15;
   const uint8_t selectMulti[]{uint8_t(0xb0|channel),32,65,uint8_t(0xc0|channel),0};board->enqueueMidi(selectMulti,sizeof(selectMulti));
   const auto warmDump=stateMessage(warm);board->enqueueMidi(warmDump.data(),warmDump.size());for(unsigned n=0;n<20000;++n)board->renderSample();
   for(uint8_t part=0;part<PartCount;++part)for(uint8_t note:{36,60,69,96}){const uint8_t on[]{uint8_t(0x90|part),note,32};board->enqueueMidi(on,3);}
   for(unsigned n=0;n<8192;++n)board->renderSample();
   for(uint8_t part=0;part<PartCount;++part){for(uint8_t note:{36,60,69,96}){const uint8_t off[]{uint8_t(0x80|part),note,0};board->enqueueMidi(off,3);}const uint8_t off[]{uint8_t(0xb0|part),120,0};board->enqueueMidi(off,3);}
   for(unsigned n=0;n<45340;++n)board->renderSample();
   if(!originalState.performance){const uint8_t select[]{uint8_t(0xb0|channel),32,33,uint8_t(0xc0|channel),0};board->enqueueMidi(select,sizeof(select));}
   board->enqueueMidi(original.data(),original.size());
   if(!originalState.performance){const auto metadata=singleMetadata(originalState.parts[0].data(),channel);board->enqueueMidi(metadata.data(),metadata.size());}
   for(unsigned n=0;n<20000;++n)board->renderSample();
   for(auto& p:history){auto a=board->renderSample();for(unsigned c=0;c<OutputCount;++c)p[c]=double(a[c])/8388608;}
   publish();
   for(unsigned i=0;i<lead;++i){output.push(Frame{produced,nextSample()});++produced;}
   running.store(true);{std::lock_guard lock(startMutex);started=true;}startCV.notify_one();
   while(!stopping.load()){
    if(pause.load()){paused.store(true);while(pause.load() && !stopping.load())std::this_thread::sleep_for(std::chrono::microseconds(50));paused.store(false);continue;}
    if(produced>=consumed.load()+lead){std::this_thread::sleep_for(std::chrono::microseconds(50));continue;}
    Event e{};while(events.peek(e) && e.due<=produced){if(!board->enqueueMidi(&e.value,1))break;events.pop(e);}
    Frame f{produced,nextSample()};if(produced>=consumed.load())output.push(f);++produced;
    if(!(produced&1023))publish();
   }
  }catch(const std::exception& e){failure=e.what();running.store(false);{std::lock_guard lock(startMutex);started=true;}startCV.notify_one();}
 });
 std::unique_lock lock(startMutex);startCV.wait(lock,[this]{return started;});
}
Device::~Device(){stopping.store(true);pause.store(false);if(worker.joinable())worker.join();}
std::array<float,OutputCount> Device::nextSample(){
 std::array<float,OutputCount> out{};
 for(unsigned c=0;c<OutputCount;++c){auto a=history[0][c],b=history[1][c],d=history[2][c],e=history[3][c];
  out[c]=float(b+0.5*phase*(d-a+phase*(2*a-5*b+4*d-e+phase*(3*(b-d)+e-a))));}
 phase+=DSP::SampleRate/rate;
 while(phase>=1){phase-=1;history[0]=history[1];history[1]=history[2];history[2]=history[3];auto a=board->renderSample();for(unsigned c=0;c<OutputCount;++c)history[3][c]=double(a[c])/8388608;}
 return out;
}
bool Device::midi(const uint8_t* b,size_t n,uint64_t offset){
 const auto due=consumed.load()+lead+offset;
 if(!events.pushGenerated(n,[&](size_t i){return Event{due,b[i]};})){++overflow;return false;}return true;
}
void Device::process(float* l,float* r,size_t n,bool offline){
 process({l,r,nullptr,nullptr},n,offline);
}
void Device::process(const std::array<float*,OutputCount>& channels,size_t n,bool offline){
 const auto begin=consumed.load();Frame f{};
 for(size_t i=0;i<n;++i){auto due=begin+i;
  // This wait is exclusively for a host explicitly selecting offline rendering.
  // The realtime branch has no waits, I/O, locks, heap allocation or guest execution.
  if(offline){while(running.load() && !stopping.load() && output.size()==0)std::this_thread::yield();}
  while(output.peek(f) && f.due<due)output.pop(f);
  if(output.peek(f) && f.due==due){output.pop(f);for(unsigned c=0;c<OutputCount;++c)if(channels[c])channels[c][i]=f.value[c];}
  else {for(auto* channel:channels)if(channel)channel[i]=0;++underrun;}
  consumed.store(due+1,std::memory_order_release);
 }
 // A fast offline caller can empty the queue at the final sample. Refill
 // before returning so an immediate offline -> realtime transition starts
 // with the advertised lead, without waiting in the realtime callback.
 if(offline)while(running.load() && !stopping.load() && output.size()<lead)std::this_thread::yield();
}
void Device::publish(){
 const auto state=board->state();if(!initialPublished){initialSnapshot=state;initialPublished=true;}
 const auto current=publishedSnapshot.load();
 for(unsigned slot=0;slot<snapshots.size();++slot)if(slot!=current && !snapshots[slot].readers.load()){
  snapshots[slot].value=state;publishedSnapshot.store(slot);return;
 }
 // Readers may pin both retired buffers. Skip this GUI update, never wait.
}
State Device::fullSnapshot()const{
 for(unsigned attempt=0;attempt<4;++attempt){const auto index=publishedSnapshot.load();auto& slot=snapshots[index];slot.readers.fetch_add(1);
  if(index!=publishedSnapshot.load()){slot.readers.fetch_sub(1);continue;}
  const auto state=slot.value;slot.readers.fetch_sub(1);return state;
 }
 return initialSnapshot; // A complete immutable state, never a torn/zero patch.
}
std::array<uint8_t,256> Device::snapshot(unsigned part)const{return fullSnapshot().parts[std::min(part,PartCount-1)];}
void Device::suspend(){pause.store(true);while(running.load() && !paused.load())std::this_thread::sleep_for(std::chrono::microseconds(50));}
void Device::resume(){pause.store(false);}
std::array<uint64_t,4> Device::diagnostics(){std::lock_guard lock(coldMutex);suspend();const std::array<uint64_t,4> result{produced,consumed.load(),board->midiReceived,board->waveReads};resume();return result;}
std::vector<uint8_t> Device::saveFlash(){std::lock_guard lock(coldMutex);suspend();auto data=board->saveUserFlash();resume();return data;}
std::vector<uint8_t> Device::dumpProgram(){
 std::lock_guard lock(coldMutex);suspend();std::vector<uint8_t> result;uint8_t byte;
 // The released OS does not dispatch request 0x42. Serialize the documented
 // controller buffers read-only; all loads still enter through original UART.
 if(board->read(0x167ba)){result=stateMessage(board->state());resume();return result;}
 while(board->popMidiOut(byte)){};
 const uint8_t request[]{0xf0,0,0x20,0x29,1,0x42,0x7f,0x40,0,0x11,2,0,0,0xf7};board->enqueueMidi(request,sizeof(request));
 for(unsigned n=0;n<16384;++n){board->renderSample();while(board->popMidiOut(byte)){if(byte==0xf0)result.clear();if(!result.empty() || byte==0xf0)result.push_back(byte);if(byte==0xf7 && result.size()==270){const auto state=board->state();std::copy_n(state.parts[0].data()+179,8,result.data()+13+179);std::fill(result.begin()+13+209,result.end()-1,0);board->restoreDesiredLevels(result.data()+13);resume();return result;}}}
 resume();return {};
}
}
