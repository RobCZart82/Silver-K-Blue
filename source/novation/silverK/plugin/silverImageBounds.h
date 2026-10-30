#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace silverK {
// Read dimensions before invoking a decoder (PNG, JPEG and GIF).
inline bool boundedImage(const void* data, size_t size, uint64_t& pixels) {
 const auto* p=static_cast<const uint8_t*>(data);uint32_t w=0,h=0;
 auto be16=[](const uint8_t* b){return uint32_t(b[0])*256+b[1];};
 auto be32=[](const uint8_t* b){return (uint32_t(b[0])<<24)|(uint32_t(b[1])<<16)|(uint32_t(b[2])<<8)|b[3];};
 if(size>=24 && std::memcmp(p,"\x89PNG\r\n\x1a\n",8)==0 && be32(p+8)==13 && std::memcmp(p+12,"IHDR",4)==0){w=be32(p+16);h=be32(p+20);}
 else if(size>=10 && (std::memcmp(p,"GIF87a",6)==0 || std::memcmp(p,"GIF89a",6)==0)){w=p[6]+uint32_t(p[7])*256;h=p[8]+uint32_t(p[9])*256;}
 else if(size>=2 && p[0]==255 && p[1]==216){
  size_t i=2;
  while(i<size){
   if(p[i++]!=255)return false;
   while(i<size && p[i]==255)++i;
   if(i==size)return false;
   const auto marker=p[i++];
   if(marker==0xda || marker==0xd9)break;
   if(marker==1 || (marker>=0xd0 && marker<=0xd8))continue;
   if(size-i<2)return false;
   const auto length=be16(p+i);if(length<2 || length>size-i)return false;
   if(marker>=0xc0 && marker<=0xcf && marker!=0xc4 && marker!=0xc8 && marker!=0xcc){if(length<8)return false;h=be16(p+i+3);w=be16(p+i+5);break;}
   i+=length;
  }
 }else return false;
 if(!w || !h)return false;
 const auto shortEdge=w<h?w:h,longEdge=w>h?w:h;
 if((w>8192 || h>8192) && !(shortEdge<=256 && uint64_t(longEdge)==uint64_t(shortEdge)*128))return false;
 pixels=uint64_t(w)*h;
 return pixels<=64ull*1024*1024;
}
}
