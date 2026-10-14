#pragma once
#include "kstationQueue.h"

namespace kstation {
// System realtime bytes may interrupt channel messages and SysEx on the MIDI
// wire. Keep their UART slots independent of an accumulated controller dump.
template<size_t Capacity,size_t RealtimeCapacity=256> class MidiSerialQueue {
public:
 bool push(uint8_t byte){return push(&byte,1);}
 bool push(const uint8_t* bytes,size_t count){
  size_t realtimeCount=0;for(size_t n=0;n<count;++n)realtimeCount+=bytes[n]>=0xf8;
  if(realtimeCount>RealtimeCapacity-realtime.size() || count-realtimeCount>Capacity-normal.size())return false;
  for(size_t n=0;n<count;++n){if(bytes[n]>=0xf8)realtime.push(bytes[n]);else normal.push(bytes[n]);}
  return true;
 }
 bool pop(uint8_t& byte){return realtime.pop(byte) || normal.pop(byte);}
 size_t size()const{return normal.size()+realtime.size();}
 void reset(){normal.reset();realtime.reset();}
 void clear(){reset();}
private:
 Queue<uint8_t,Capacity> normal;
 Queue<uint8_t,RealtimeCapacity> realtime;
};
}
