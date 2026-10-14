#include "silverVStation.h"
#include "silverPatch.h"
#include <cstring>
namespace silverK {
namespace {
constexpr unsigned Programs=400,SoundSize=128,NameSize=21;
constexpr size_t ChunkSize=1+Programs*(SoundSize+NameSize)+7;
constexpr uint8_t Record[]{'S','K','V','S'};
uint32_t big32(const uint8_t* p){return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];}
}
bool parseVStationBank(pluginLib::patchDB::DataList& output,const pluginLib::patchDB::Data& input){
 // VST2 chunk-bank header; NvS0 legacy V-Station chunk version 1.
 // Unlike K SysEx, the bank stores its 400 names in a separate table.
 if(input.size()!=160+ChunkSize || std::memcmp(input.data(),"CcnK",4) || std::memcmp(input.data()+8,"FBCh",4)
    || std::memcmp(input.data()+16,"NvS0",4) || big32(input.data()+12)!=1 || big32(input.data()+24)!=Programs
    || big32(input.data()+156)!=ChunkSize)return false;
 const auto declared=big32(input.data()+4);if(declared && size_t(declared)+8!=input.size())return false; // Some original banks leave byteSize at zero.
 const auto* chunk=input.data()+160;if(chunk[0]!=1)return false;
 if(!std::all_of(chunk+1,chunk+1+Programs*SoundSize,[](uint8_t b){return b<128;}))return false;
 const auto* names=chunk+1+Programs*SoundSize;
 if(!std::all_of(names,names+Programs*NameSize,[](uint8_t b){return b==0 || (b>=32 && b<127);} ))return false;
 for(unsigned n=0;n<Programs;++n){
  pluginLib::patchDB::Data record(std::begin(Record),std::end(Record));
  const uint8_t header[]{0xf0,0,0x20,0x29,1,0x41,0x7f,1,0,0x11,1,uint8_t(1+n/100),uint8_t(n%100)};
  record.insert(record.end(),std::begin(header),std::end(header));
  record.insert(record.end(),chunk+1+n*SoundSize,chunk+1+(n+1)*SoundSize);record.push_back(0xf7);
  record.insert(record.end(),names+n*NameSize,names+(n+1)*NameSize);output.push_back(std::move(record));
 }
 return true;
}
bool parseVStationProgram(pluginLib::patchDB::DataList& output,const pluginLib::patchDB::Data& input){
 // NvS0/FPCh stores one 128-byte sound followed by its 21-byte name.
 // The VST header's 28-byte label is often stale ("User 300"); use the chunk.
 constexpr size_t size=60+SoundSize+NameSize;
 if(input.size()!=size || std::memcmp(input.data(),"CcnK",4) || std::memcmp(input.data()+8,"FPCh",4)
    || std::memcmp(input.data()+16,"NvS0",4) || big32(input.data()+12)!=1 || big32(input.data()+24)!=1
    || big32(input.data()+56)!=SoundSize+NameSize)return false;
 const auto declared=big32(input.data()+4);if(declared && size_t(declared)+8!=input.size())return false;
 const auto* sound=input.data()+60;const auto* name=sound+SoundSize;
 if(!std::all_of(sound,name,[](uint8_t b){return b<128;})
    || !std::all_of(name,name+NameSize,[](uint8_t b){return b==0 || (b>=32 && b<127);} ))return false;
 pluginLib::patchDB::Data record(std::begin(Record),std::end(Record));
 const uint8_t header[]{0xf0,0,0x20,0x29,1,0x41,0x7f,0,0,0x11,1,0,0};
 record.insert(record.end(),std::begin(header),std::end(header));record.insert(record.end(),sound,name);record.push_back(0xf7);
 record.insert(record.end(),name,name+NameSize);output.push_back(std::move(record));return true;
}
bool unpackVStationPatch(pluginLib::patchDB::Data& record,std::string& name){
 if(record.size()!=sizeof(Record)+142+NameSize || !std::equal(std::begin(Record),std::end(Record),record.begin()) || !validPatch(record.data()+sizeof(Record),142))return false;
 const auto* text=record.data()+sizeof(Record)+142;const auto end=std::find(text,text+NameSize,uint8_t(0));
 name=juce::String::fromUTF8(reinterpret_cast<const char*>(text),int(end-text)).trim().toStdString();
 pluginLib::patchDB::Data sysex(record.begin()+sizeof(Record),record.begin()+sizeof(Record)+142);record=std::move(sysex);return true;
}
}
