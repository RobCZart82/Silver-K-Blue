#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace silverK {
// Adapted from temaniak/Silver-K ab91ba1, silverKEngine.cpp. Capture host
// input before the resampler can overwrite an aliased output buffer. Keep
// the existing mono bus and controller. Optional stereo mixing follows the same
// upstream finite-per-channel 0.5 L + 0.5 R rule; bus routing is not enabled here.
template<class Render>
void processInputChunks(float* left,float* right,int count,const float* input,const float* inputRight,Render&& render) {
 for(int offset=0;offset<count;offset+=256){
  const int size=std::min(256,count-offset);
  std::array<float,256> mono{};
  if(input)for(int n=0;n<size;++n){
   const auto sample=input[offset+n];const float l=std::isfinite(sample)?sample:0.f;
   const float r=inputRight && std::isfinite(inputRight[offset+n])?inputRight[offset+n]:0.f;
   mono[n]=inputRight?0.5f*l+0.5f*r:l;
  }
  render(left+offset,right+offset,mono.data(),size);
 }
}
// Preserve the existing mono call sites; stereo bus routing is a later step.
template<class Render>
void processMonoInputChunks(float* left,float* right,int count,const float* input,Render&& render) {
 processInputChunks(left,right,count,input,nullptr,std::forward<Render>(render));
}
}
