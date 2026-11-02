#include "silverAudioInput.h"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void exercise(int count,int alias){
 constexpr float guard=12345.f;
 std::array<float,1026> left,right,source;
 left.fill(guard);right.fill(guard);source.fill(guard);
 float* input=alias==1?left.data()+1:alias==2?right.data()+1:source.data()+1;
 for(int n=0;n<count;++n)input[n]=n%11==0?std::numeric_limits<float>::quiet_NaN():n%13==0?std::numeric_limits<float>::infinity():float(n)*.001f;
 const auto original=alias==1?left:alias==2?right:source;
 int rendered=0,calls=0;
 silverK::processMonoInputChunks(left.data()+1,right.data()+1,count,alias==3?nullptr:input,
  [&](float* l,float* r,const float* mono,int size){
   check(size>0 && size<=256,"Invalid chunk size");++calls;
   // A host/resampler can write output before it has consumed the input.
   // Destroy both aliased buffers first, then verify the captured samples.
   std::fill_n(l,size,-999.f);std::fill_n(r,size,-999.f);
   for(int n=0;n<size;++n){const float raw=original[1+rendered+n];const float expected=alias==3 || !std::isfinite(raw)?0.f:raw;
    check(mono[n]==expected,"Aliased/non-finite input corrupted render");l[n]=mono[n];r[n]=-mono[n];}
   rendered+=size;
  });
 check(rendered==count && calls==(count+255)/256,"Partial final chunk was lost");
 check(left[0]==guard && right[0]==guard && left[count+1]==guard && right[count+1]==guard,"Render wrote outside host block");
}
void stereo(int count,bool swapped){
 constexpr float guard=12345.f;
 std::array<float,1026> left,right;left.fill(guard);right.fill(guard);
 for(int n=0;n<count;++n){
  left[n+1]=n%7==0?std::numeric_limits<float>::quiet_NaN():float(n)*.001f;
  right[n+1]=n%9==0?-std::numeric_limits<float>::infinity():-float(n)*.002f;
 }
 const auto originalLeft=left,originalRight=right;int rendered=0;
 silverK::processInputChunks(left.data()+1,right.data()+1,count,swapped?right.data()+1:left.data()+1,swapped?left.data()+1:right.data()+1,
  [&](float* l,float* r,const float* mono,int size){
   std::fill_n(l,size,-999.f);std::fill_n(r,size,-999.f);
   for(int n=0;n<size;++n){
    const auto a=originalLeft[1+rendered+n],b=originalRight[1+rendered+n];
    const float expected=.5f*(std::isfinite(a)?a:0.f)+.5f*(std::isfinite(b)?b:0.f);
    check(mono[n]==expected,"Stereo aliased/invalid channel mix corrupted");
   }rendered+=size;
  });
 check(rendered==count,"Stereo final chunk lost");
 check(left[0]==guard&&right[0]==guard&&left[count+1]==guard&&right[count+1]==guard,"Stereo wrote outside block");
}
void mixLimits(){
 float l=0,r=0;
 const float max=std::numeric_limits<float>::max();const float a[]{max},b[]{max};
 silverK::processInputChunks(&l,&r,1,a,b,[&](float*,float*,const float* m,int){check(m[0]==max,"Stereo average overflowed finite maximum");});
 silverK::processInputChunks(&l,&r,1,nullptr,b,[&](float*,float*,const float* m,int){check(m[0]==0,"Missing primary input must remain silent");});
}

}
int main(){try{
 for(int count:{0,1,32,63,64,255,256,257,511,1024})for(int alias=0;alias<4;++alias)exercise(count,alias);
 for(int count:{0,1,255,256,257,511,1024})for(bool swapped:{false,true})stereo(count,swapped);mixLimits();
 std::cout<<"PASS: stereo finite mixing and aliasing; mono input capture, left/right aliasing, NaN/Inf, disconnected input and 0..1024-sample block boundaries\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
