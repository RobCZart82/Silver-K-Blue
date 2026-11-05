#pragma once
#include <cstdint>
#include <vector>

namespace ks {
struct WorkerPlacement {
    bool requested=false;
    const char* status="default";
    uint32_t error=0;
    std::vector<uint32_t> cpuSets;
};
struct PlacementCpu {
    uint32_t id;
    unsigned group,logical,efficiency;
    bool unavailable=false;
};
// Pure selection: no hard-coded CPU numbers; preserve the host's allowed mask.
std::vector<uint32_t> performanceCpuSets(const std::vector<PlacementCpu>&,uint64_t allowed,bool& hybrid);
// Called once by the KS worker after boot/warm-up. No process-wide setters.
WorkerPlacement applyWorkerPlacement(bool requestPerformance);
}
