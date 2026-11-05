#pragma once
#include <cstdint>

namespace ks {
struct TimerAdvance {uint16_t count;bool matched;};
// Same result as the original TPU0 per-clock loop, with no intermediate IO or
// interrupt dispatch. A pending IRQ is a bit: repeated matches coalesce.
// Preserve the reference's compare-before-16-bit-wrap behavior: compare=0
// never matches, even when the counter wraps from 65535 to zero.
inline TimerAdvance advanceControllerTimer(uint16_t count,uint16_t compare,uint64_t elapsed,bool clearOnMatch){
    const uint64_t distance=compare>count?uint64_t(compare-count):65536ull+compare-count;
    const bool matched=compare && elapsed>=distance;
    if(matched && clearOnMatch)return {uint16_t((elapsed-distance)%compare),true};
    return {uint16_t(count+elapsed),matched};
}
}
