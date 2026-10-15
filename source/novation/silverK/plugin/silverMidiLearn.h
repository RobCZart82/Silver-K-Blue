#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
namespace silverK {
// Message thread arms/polls; the audio thread publishes one raw Note On.
// No UI callbacks, parameter changes, allocation or MIDI consumption here.
class MidiNoteLearn final {
 static_assert(std::atomic<uint64_t>::is_always_lock_free,"MIDI Learn requires lock-free atomics");
 std::atomic<uint64_t> state{0};
 uint64_t serial=0; // Message thread only; the low byte is reserved for note+1.
public:
 uint64_t arm() noexcept {const auto token=(++serial)<<8;state.store(token,std::memory_order_relaxed);return token;}
 void cancel(uint64_t token) noexcept {
  auto value=state.load(std::memory_order_relaxed);
  while(token && (value&~uint64_t(255))==token && !state.compare_exchange_weak(value,0,std::memory_order_relaxed)){}
 }
 int take(uint64_t token) noexcept {
  auto value=state.load(std::memory_order_relaxed);
  if(!token || (value&~uint64_t(255))!=token || !(value&255))return -1;
  return state.compare_exchange_strong(value,0,std::memory_order_relaxed)?int(value&255)-1:-1;
 }
 void observe(const uint8_t* bytes,size_t size) noexcept {
  if(size!=3 || bytes[0]<0x90 || bytes[0]>0x9f || bytes[1]>127 || !bytes[2] || bytes[2]>127)return;
  auto value=state.load(std::memory_order_relaxed);
  if(value && !(value&255))state.compare_exchange_strong(value,value|uint64_t(bytes[1]+1),std::memory_order_relaxed);
 }
};
}
