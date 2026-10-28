// Offline Windows ETL -> scheduler TSV. No plugin/guest code is linked.
// Build: cl /std:c++17 /EHsc /O2 decode_scheduler_etw.cpp /link advapi32.lib tdh.lib
// CSwitch/ReadyThread layouts: learn.microsoft.com/windows/win32/etw/{cswitch,readythread}
#define NOMINMAX
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
const GUID threadGuid={0x3d6fa8d1,0xfe05,0x11d0,{0x9d,0xda,0x00,0xc0,0x4f,0xd7,0xba,0x7c}};
const GUID perfGuid={0xce1dbfb4,0x137e,0x4da6,{0x87,0xb0,0x3f,0x59,0xaa,0x10,0x2c,0xbc}};
std::ofstream events,schemas;
uint64_t emitted=0,unsupported=0;
std::map<unsigned,bool> validSchemas;
std::string utf8(const wchar_t* data,size_t count) {
    if(!count)return {};
    const auto n=WideCharToMultiByte(CP_UTF8,0,data,int(count),nullptr,0,nullptr,nullptr);
    std::string result(n,' ');WideCharToMultiByte(CP_UTF8,0,data,int(count),result.data(),n,nullptr,nullptr);
    for(auto& c:result)if(c=='\t'||c=='\r'||c=='\n')c=' ';
    return result;
}
bool schema(EVENT_RECORD* event) {
    const auto op=event->EventHeader.EventDescriptor.Opcode,version=event->EventHeader.EventDescriptor.Version;
    const auto key=unsigned(op)*256+version;
    if(auto found=validSchemas.find(key);found!=validSchemas.end())return found->second;
    ULONG size=0;TdhGetEventInformation(event,0,nullptr,nullptr,&size);
    std::vector<BYTE> buffer(size);
    auto* info=reinterpret_cast<TRACE_EVENT_INFO*>(buffer.data());
    const auto error=size?TdhGetEventInformation(event,0,nullptr,info,&size):ERROR_NOT_FOUND;
    bool valid=error==ERROR_SUCCESS;
    if(!valid) {
        // MOF lookup may be unavailable in a restricted/offline SDK environment.
        // CSwitch V4 retains the offsets used by Microsoft's PerfView parser:
        // github.com/microsoft/perfview, KernelTraceEventParser.cs / CSwitchTraceData.
        valid=((version==2||version==4) && op==36 && event->UserDataLength==24)
            ||(version==2 && op==50 && event->UserDataLength==8)
            ||(op>=1 && op<=4 && (version==2||version==3) && event->UserDataLength>=8)
            ||(op==72 && version==2 && event->UserDataLength>=10 && event->UserDataLength%2==0);
        schemas<<"opcode="<<unsigned(op)<<" version="<<unsigned(version)<<" bytes="<<event->UserDataLength
               <<" TDH_error="<<error<<" known_fixed_layout="<<valid<<'\n';
        validSchemas[key]=valid;return valid;
    }
    const std::vector<std::wstring> expected=op==36?
        std::vector<std::wstring>{L"NewThreadId",L"OldThreadId",L"NewThreadPriority",L"OldThreadPriority",L"PreviousCState",L"SpareByte",L"OldThreadWaitReason",L"OldThreadWaitMode",L"OldThreadState"}:
        op==50?std::vector<std::wstring>{L"TThreadId",L"AdjustReason",L"AdjustIncrement",L"Flag"}:std::vector<std::wstring>{};
    if(valid) {
        schemas<<"opcode="<<unsigned(op)<<" version="<<unsigned(version)<<" fields=";
        if(info->TopLevelPropertyCount<expected.size())valid=false;
        for(ULONG i=0;i<info->TopLevelPropertyCount;++i) {
            const auto& p=info->EventPropertyInfoArray[i];
            const auto* name=reinterpret_cast<const wchar_t*>(buffer.data()+p.NameOffset);
            schemas<<utf8(name,wcslen(name))<<':'<<p.nonStructType.InType<<' ';
            if(i<expected.size() && (expected[i]!=name || p.Flags!=0 ||
                p.nonStructType.InType!=(i<(op==36?2u:1u)?TDH_INTYPE_UINT32:(op==36&&i==4?TDH_INTYPE_UINT8:TDH_INTYPE_INT8))))valid=false;
        }
        schemas<<" layout_supported="<<valid<<'\n';
    }
    validSchemas[key]=valid;return valid;
}
uint32_t u32(const BYTE* p){uint32_t v;std::memcpy(&v,p,4);return v;}
void WINAPI consume(EVENT_RECORD* e) {
    if(IsEqualGUID(e->EventHeader.ProviderId,perfGuid) && e->EventHeader.EventDescriptor.Opcode==46) {
        // x64 SampledProfile V2: IP(8), TID(4), count(2), flags(1), rank(1).
        // Microsoft's SampledProfileTraceData defines the DPC/ISR/priority bits.
        if(e->EventHeader.EventDescriptor.Version!=2 || e->UserDataLength!=16){++unsupported;return;}
        static bool logged=false;
        if(!logged){schemas<<"PerfInfoSample version=2 bytes=16 known_fixed_layout=1\n";logged=true;}
        const auto* p=static_cast<const BYTE*>(e->UserData);
        uint64_t ip;std::memcpy(&ip,p,8);
        events<<e->EventHeader.TimeStamp.QuadPart<<'\t'<<unsigned(e->BufferContext.ProcessorNumber)
              <<"\tP\t"<<u32(p+8)<<'\t'<<(ip>>63)<<'\t'<<unsigned(p[14]&3)<<"\t0\t"<<unsigned((p[14]>>3)&31)<<"\t0\n";
        ++emitted;return;
    }
    if(!IsEqualGUID(e->EventHeader.ProviderId,threadGuid))return;
    const auto op=e->EventHeader.EventDescriptor.Opcode;
    const auto* p=static_cast<const BYTE*>(e->UserData);
    if(op==36 || op==50) {
        if(!schema(e) || e->UserDataLength<(op==36?16:8)){++unsupported;return;}
        events<<e->EventHeader.TimeStamp.QuadPart<<'\t'<<unsigned(e->BufferContext.ProcessorNumber)<<'\t';
        if(op==36)events<<"C\t"<<u32(p)<<'\t'<<u32(p+4)<<'\t'<<int(int8_t(p[14]))<<'\t'<<int(int8_t(p[12]))<<'\t'<<int(int8_t(p[8]))<<'\t'<<int(int8_t(p[9]));
        else events<<"R\t"<<u32(p)<<'\t'<<e->EventHeader.ThreadId<<'\t'<<unsigned(p[6])<<'\t'<<int(int8_t(p[4]))<<"\t0\t0";
        events<<'\n';++emitted;
    } else if(op==1||op==2||op==3||op==4||op==72) {
        if(!schema(e)||e->UserDataLength<8){++unsupported;return;}
        events<<e->EventHeader.TimeStamp.QuadPart<<'\t'<<unsigned(e->BufferContext.ProcessorNumber)<<'\t'<<(op==72?'N':'T')<<'\t'<<u32(p)<<'\t'<<u32(p+4)<<'\t'<<unsigned(op)<<"\t0\t0\t0";
        if(op==72 && e->UserDataLength>8) {
            const auto* name=reinterpret_cast<const wchar_t*>(p+8);
            const auto count=(e->UserDataLength-8)/2;size_t n=0;while(n<count&&name[n])++n;
            events<<'\t'<<utf8(name,n);
        }
        events<<'\n';++emitted;
    }
}
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=4){std::cerr<<"Usage: decode_scheduler_etw trace.etl events.tsv schemas.txt\n";return 2;}
    if(std::filesystem::exists(argv[2])||std::filesystem::exists(argv[3])
        ||std::filesystem::weakly_canonical(argv[2])==std::filesystem::weakly_canonical(argv[3])) {
        std::cerr<<"Use two distinct new output files; existing evidence is never overwritten.\n";return 2;
    }
    events.open(argv[2]);schemas.open(argv[3]);if(!events||!schemas)return 2;
    EVENT_TRACE_LOGFILEW file{};file.LogFileName=argv[1];
    file.ProcessTraceMode=PROCESS_TRACE_MODE_EVENT_RECORD;file.EventRecordCallback=consume;
    auto handle=OpenTraceW(&file);
    if(handle==INVALID_PROCESSTRACE_HANDLE){std::cerr<<"OpenTrace error "<<GetLastError()<<'\n';return 1;}
    schemas<<"EventsLost="<<file.LogfileHeader.EventsLost<<" BuffersLost="<<file.LogfileHeader.BuffersLost<<" Processors="<<file.LogfileHeader.NumberOfProcessors<<"\n";
    const auto result=ProcessTrace(&handle,1,nullptr,nullptr);CloseTrace(handle);
    std::cout<<"ProcessTrace="<<result<<" emitted="<<emitted<<" unsupported="<<unsupported<<"\n";
    return result==ERROR_SUCCESS && unsupported==0?0:1;
}
