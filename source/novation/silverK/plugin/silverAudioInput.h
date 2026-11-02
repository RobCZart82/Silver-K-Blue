#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace silverK {
// Adapted from temaniak/Silver-K ab91ba1, silverKEngine.cpp. Capture host
// input before the resampler can overwrite an aliased output buffer. Keep
// the existing mono bus and controller; this adds no new routing/UI feature.
template<class Render>
void processMonoInputChunks(float* left,float* right,int count,const float* input,Render&& render) {
 for(int offset=0;offset<count;offset+=256){
  const int size=std::min(256,count-offset);
  std::array<float,256> mono{};
  if(input)for(int n=0;n<size;++n){const auto sample=input[offset+n];mono[n]=std::isfinite(sample)?sample:0.f;}
  render(left+offset,right+offset,mono.data(),size);
 }
}
}
