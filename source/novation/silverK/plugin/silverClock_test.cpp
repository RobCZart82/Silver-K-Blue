#include "silverClock.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace silverK;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static std::vector<HostClock::Event> block(HostClock& c,HostClock::Position p,double rate,int n){
 c.begin(p,rate,n);std::vector<HostClock::Event> events;HostClock::Event e{};
 while(c.next(e)){check(e.offset>=0 && e.offset<n,"Clock event outside block");if(!events.empty())check(e.offset>=events.back().offset,"Clock ordering");events.push_back(e);}
 return events;
}
int main(){try{
 for(double rate:{44100.,48000.,96000.})for(double bpm:{60.,123.456,180.})for(int size:{1,127,256,511,4096}){
  HostClock c;int64_t total=0,ticks=0;const int64_t length=int64_t(rate*5);
  for(;total<length;){const int n=int(std::min<int64_t>(size,length-total));const auto events=block(c,{true,true,true,bpm,total*bpm/(60*rate)},rate,n);
   for(auto e:events){if(e.status==0xfa)check(total==0,"Unexpected transport restart");if(e.status!=0xf8)continue;const double expected=ticks*rate*60/(bpm*24);check(std::abs(total+e.offset-expected)<=1.00001,"Clock drift or duplicate/missing tick");++ticks;}total+=n;
  }
  check(ticks==int64_t(std::ceil(length*bpm*24/(60*rate)-1e-7)),"24 PPQN count");
 }
 HostClock c;auto events=block(c,{true,true,true,120,0.01},48000,1000);
 check(events.size()==2 && events[0].status==0xfa && events[1].offset==760,"Fractional PPQ start");
 events=block(c,{true,true,true,60,0.01+1000./24000},48000,2000);
 check(events.size()==1 && events[0].status==0xf8 && events[0].offset==1520,"Tempo change must preserve tick phase");
 events=block(c,{true,true,true,60,4},48000,256);
 check(events.size()==3 && events[0].status==0xfc && events[1].status==0xfa && events[2].status==0xf8 && events[2].offset==0,"Loop/seek restart");
 events=block(c,{true,false,true,60,4+256./48000},48000,2048);
 check(events.size()==2 && events[0].status==0xfc && events[1].status==0xf8,"Stopped transport still supplies tempo to effects");
 events=block(c,{},48000,256);check(events.size()==1 && events[0].status==0xfc && !c.ownsTransport(),"Host loss handover");
 check(block(c,{},48000,256).empty(),"Standalone must not synthesize clock");
 check(block(c,{true,true,true,std::numeric_limits<double>::quiet_NaN(),0},48000,256).empty(),"Invalid BPM rejected");
 c.reset();events=block(c,{true,true,true,120,-0.02},48000,1000);
 check(events.size()==2 && events[1].offset==480,"Negative preroll PPQ");
 std::cout<<"PASS: 24 PPQN, sample offsets, no drift at 3 rates/5 block sizes, tempo changes, fractional/negative PPQ, seek, stop and host loss\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
