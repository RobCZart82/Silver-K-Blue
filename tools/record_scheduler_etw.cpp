// Standalone elevated ETW controller. Benchmark processes remain unelevated.
// Avoids the Windows 19041 WPR COM merge failure (0x80010106).
// cl /std:c++17 /EHsc /O2 record_scheduler_etw.cpp /link advapi32.lib rpcrt4.lib
#define NOMINMAX
#include <windows.h>
#include <evntrace.h>
#include <rpc.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int wmain(int argc,wchar_t** argv) {
    if(argc!=3 && !(argc==4 && std::wstring(argv[3])==L"sampled"))return 2;
    const auto duration=wcstoul(argv[2],nullptr,10);
    if(duration<5||duration>600)return 2;
    const auto directory=std::filesystem::canonical(argv[1]);
    const auto path=directory/L"scheduler.etl";
    const auto stopPath=directory/L"STOP";
    if(std::filesystem::exists(path)||std::filesystem::exists(directory/L"native-status.txt")
        ||std::filesystem::exists(directory/L"native-recorder.log")||std::filesystem::exists(stopPath))return 2;
    std::ofstream log(directory/L"native-recorder.log");
    HANDLE token=nullptr;TOKEN_PRIVILEGES privileges{};
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&token))return 1;
    privileges.PrivilegeCount=1;
    LookupPrivilegeValueW(nullptr,L"SeSystemProfilePrivilege",&privileges.Privileges[0].Luid);
    privileges.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
    SetLastError(ERROR_SUCCESS);
    const auto adjusted=AdjustTokenPrivileges(token,FALSE,&privileges,sizeof(privileges),nullptr,nullptr);
    const auto privilegeError=GetLastError();CloseHandle(token);
    if(!adjusted||privilegeError!=ERROR_SUCCESS){log<<"Profile privilege: "<<privilegeError<<'\n';return 1;}
    const auto name=L"SilverKS-Scheduler-"+std::to_wstring(GetCurrentProcessId());
    const auto filename=path.wstring();
    const auto bytes=sizeof(EVENT_TRACE_PROPERTIES)+(name.size()+filename.size()+2)*sizeof(wchar_t);
    std::vector<BYTE> buffer(bytes);
    auto* p=reinterpret_cast<EVENT_TRACE_PROPERTIES*>(buffer.data());
    p->Wnode.BufferSize=ULONG(bytes);p->Wnode.Flags=WNODE_FLAG_TRACED_GUID;p->Wnode.ClientContext=1;
    UuidCreate(&p->Wnode.Guid); // A private system logger, never NT Kernel Logger.
    p->LogFileMode=EVENT_TRACE_FILE_MODE_SEQUENTIAL|EVENT_TRACE_SYSTEM_LOGGER_MODE;
    p->BufferSize=64;p->MinimumBuffers=128;p->MaximumBuffers=512;p->FlushTimer=1;p->MaximumFileSize=512;
    p->EnableFlags=EVENT_TRACE_FLAG_PROCESS|EVENT_TRACE_FLAG_THREAD|EVENT_TRACE_FLAG_CSWITCH|
        EVENT_TRACE_FLAG_DISPATCHER|EVENT_TRACE_FLAG_DPC|EVENT_TRACE_FLAG_INTERRUPT;
    if(argc==4)p->EnableFlags|=EVENT_TRACE_FLAG_PROFILE; // Existing OS interval; no TraceSetInformation changes.
    log<<"EnableFlags="<<p->EnableFlags<<" clock=QPC max_seconds="<<duration<<" max_MB=512\n";
    p->LoggerNameOffset=sizeof(EVENT_TRACE_PROPERTIES);
    p->LogFileNameOffset=p->LoggerNameOffset+ULONG((name.size()+1)*sizeof(wchar_t));
    memcpy(buffer.data()+p->LoggerNameOffset,name.c_str(),(name.size()+1)*sizeof(wchar_t));
    memcpy(buffer.data()+p->LogFileNameOffset,filename.c_str(),(filename.size()+1)*sizeof(wchar_t));
    TRACEHANDLE handle=0;
    const auto start=StartTraceW(&handle,name.c_str(),p);log<<"StartTrace="<<start<<'\n';log.flush();
    if(start!=ERROR_SUCCESS){std::ofstream(directory/L"native-status.txt")<<"failed "<<start;return 1;}
    std::ofstream(directory/L"native-status.txt")<<"recording\n";
    const auto deadline=GetTickCount64()+duration*1000;
    std::error_code error;
    while(GetTickCount64()<deadline && !std::filesystem::exists(stopPath,error) && !error)Sleep(100);
    // Stop only our session handle; never cancel unrelated WPR/ETW sessions.
    const auto stop=ControlTraceW(handle,name.c_str(),p,EVENT_TRACE_CONTROL_STOP);
    log<<"ControlTrace="<<stop<<" EventsLost="<<p->EventsLost<<" LogBuffersLost="<<p->LogBuffersLost
       <<" RealTimeBuffersLost="<<p->RealTimeBuffersLost<<" BuffersWritten="<<p->BuffersWritten<<'\n';
    std::ofstream(directory/L"native-status.txt")<<(stop==ERROR_SUCCESS?"complete":"failed")<<' '<<stop<<'\n';
    return stop==ERROR_SUCCESS && p->EventsLost==0 && p->LogBuffersLost==0?0:1;
}
