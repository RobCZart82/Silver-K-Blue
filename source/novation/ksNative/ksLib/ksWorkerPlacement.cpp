#include "ksWorkerPlacement.h"
#include <algorithm>
#include <cstring>
#include <cstddef>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace ks {
std::vector<uint32_t> performanceCpuSets(const std::vector<PlacementCpu>& cpus,uint64_t allowed,bool& hybrid) {
    unsigned low=256,high=0;
    for(const auto& c:cpus)if(c.group==0){low=std::min(low,c.efficiency);high=std::max(high,c.efficiency);}
    hybrid=low<high;
    std::vector<uint32_t> ids;
    if(hybrid)for(const auto& c:cpus)
        if(c.group==0 && c.logical<64 && (allowed&(uint64_t(1)<<c.logical)) && !c.unavailable && c.efficiency==high)ids.push_back(c.id);
    return ids;
}
WorkerPlacement applyWorkerPlacement(bool requested) {
    WorkerPlacement result;result.requested=requested;
    if(!requested)return result;
    result.status="unsupported";
#ifdef _WIN32
    const auto module=GetModuleHandleW(L"kernel32.dll");
    using Enumerate=BOOL(WINAPI*)(PSYSTEM_CPU_SET_INFORMATION,ULONG,PULONG,HANDLE,ULONG);
    using Query=BOOL(WINAPI*)(HANDLE,PULONG,ULONG,PULONG);
    using Set=BOOL(WINAPI*)(HANDLE,const ULONG*,ULONG);
    const auto enumerate=reinterpret_cast<Enumerate>(GetProcAddress(module,"GetSystemCpuSetInformation"));
    const auto processSets=reinterpret_cast<Query>(GetProcAddress(module,"GetProcessDefaultCpuSets"));
    const auto threadSets=reinterpret_cast<Query>(GetProcAddress(module,"GetThreadSelectedCpuSets"));
    const auto set=reinterpret_cast<Set>(GetProcAddress(module,"SetThreadSelectedCpuSets"));
    if(!enumerate||!processSets||!threadSets||!set)return result;
    if(GetActiveProcessorGroupCount()!=1){result.status="multiple-groups";return result;}
    // Never override an explicit host CPU-set assignment, even in this experiment.
    ULONG count=0;
    if(!processSets(GetCurrentProcess(),nullptr,0,&count) && GetLastError()!=ERROR_INSUFFICIENT_BUFFER){result.status="query-failed";result.error=GetLastError();return result;}
    if(count){result.status="host-cpu-sets";return result;}
    if(!threadSets(GetCurrentThread(),nullptr,0,&count) && GetLastError()!=ERROR_INSUFFICIENT_BUFFER){result.status="query-failed";result.error=GetLastError();return result;}
    if(count){result.status="host-cpu-sets";return result;}
    DWORD_PTR processMask=0,systemMask=0;GROUP_AFFINITY affinity{};
    if(!GetProcessAffinityMask(GetCurrentProcess(),&processMask,&systemMask)||!GetThreadGroupAffinity(GetCurrentThread(),&affinity)){
        result.status="query-failed";result.error=GetLastError();return result;
    }
    ULONG bytes=0;
    enumerate(nullptr,0,&bytes,GetCurrentProcess(),0);
    if(!bytes){result.status="query-failed";result.error=GetLastError();return result;}
    // ULONG storage provides alignment; copy variable-sized records before reading.
    std::vector<ULONG> storage((bytes+sizeof(ULONG)-1)/sizeof(ULONG));
    if(!enumerate(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(storage.data()),bytes,&bytes,GetCurrentProcess(),0)){
        result.status="query-failed";result.error=GetLastError();return result;
    }
    std::vector<PlacementCpu> cpus;
    for(size_t offset=0;offset<bytes;) {
        SYSTEM_CPU_SET_INFORMATION item{};
        if(bytes-offset<8){result.status="invalid-topology";return result;}
        std::memcpy(&item,reinterpret_cast<const char*>(storage.data())+offset,std::min(sizeof(item),size_t(bytes-offset)));
        if(item.Size<8||item.Size>bytes-offset){result.status="invalid-topology";return result;}
        if(item.Type==CpuSetInformation){
            if(item.Size<offsetof(SYSTEM_CPU_SET_INFORMATION,CpuSet.AllocationTag)){result.status="invalid-topology";return result;}
            cpus.push_back({item.CpuSet.Id,item.CpuSet.Group,item.CpuSet.LogicalProcessorIndex,item.CpuSet.EfficiencyClass,
                bool(item.CpuSet.Allocated&&!item.CpuSet.AllocatedToTargetProcess)});
        }
        offset+=item.Size;
    }
    bool hybrid=false;
    auto ids=performanceCpuSets(cpus,uint64_t(processMask&affinity.Mask),hybrid);
    if(!hybrid){result.status="homogeneous";return result;}
    if(ids.empty()){result.status="no-eligible-p-cpus";return result;}
    const std::vector<ULONG> nativeIds(ids.begin(),ids.end());
    if(!set(GetCurrentThread(),nativeIds.data(),ULONG(nativeIds.size()))){result.status="set-failed";result.error=GetLastError();return result;}
    result.cpuSets=std::move(ids);result.status="p-cpu-sets";
#endif
    return result;
}
}
