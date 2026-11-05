#include "ksWorkerPlacement.h"
#include <iostream>
#include <stdexcept>
static void check(bool ok){if(!ok)throw std::runtime_error("CPU selection regression");}
int main(){try{
    bool hybrid=false;
    // Deliberately non-positional CPU-set IDs, SMT siblings, a reserved CPU,
    // an excluded host-affinity CPU, and another group.
    const std::vector<ks::PlacementCpu> cpus{{79,0,0,0},{500,0,1,1},{17,0,2,1},{801,0,3,1,true},{900,1,0,2}};
    auto ids=ks::performanceCpuSets(cpus,~uint64_t(0),hybrid);
    check(hybrid&&ids==std::vector<uint32_t>({500,17}));
    ids=ks::performanceCpuSets(cpus,2,hybrid);check(ids==std::vector<uint32_t>({500}));
    ids=ks::performanceCpuSets(cpus,1,hybrid);check(hybrid&&ids.empty());
    ids=ks::performanceCpuSets({{1,0,0,0},{2,0,1,0}},3,hybrid);check(!hybrid&&ids.empty());
    ids=ks::performanceCpuSets({},~uint64_t(0),hybrid);check(!hybrid&&ids.empty());
    const auto off=ks::applyWorkerPlacement(false);check(!off.requested&&off.cpuSets.empty()&&off.error==0);
    std::cout<<"PASS: hybrid selection, host mask, reserved CPUs, homogeneous fallback and default no-op\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
