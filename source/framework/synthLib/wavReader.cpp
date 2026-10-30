#include "wavReader.h"
#include "wavTypes.h"
#include <cstring>
#include <map>
#include <limits>

namespace synthLib
{
namespace
{
uint16_t le16(const uint8_t* p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }
uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
bool tag(const uint8_t* p, const char* id) { return std::memcmp(p,id,4)==0; }
}
bool WavReader::load(Data& result, std::vector<CuePoint>* outputCues, const uint8_t* buffer, size_t size)
{
    // Parse into local state: failure must not publish pointers into a broken file.
    if (!buffer || size < 12 || !tag(buffer,"RIFF") || !tag(buffer+8,"WAVE")) return false;
    const auto riffSize=le32(buffer+4);
    if (riffSize < 4 || size_t(riffSize) > size-8) return false;
    const size_t end=size_t(riffSize)+8;
    size_t dataOffset=0,dataSize=0;
    uint16_t format=0,channels=0,bits=0,alignment=0;
    uint32_t rate=0;
    bool haveFormat=false;
    std::map<uint32_t,SWaveFormatChunkCuePoint> cues;
    std::map<uint32_t,std::string> labels;
    for (size_t pos=12;pos<end;)
    {
        if (end-pos<8) return false;
        const auto* header=buffer+pos;
        const size_t length=le32(header+4);
        pos+=8;
        if (length>end-pos) return false;
        const auto* chunk=buffer+pos;
        if (tag(header,"fmt "))
        {
            if (length<16) return false;
            format=le16(chunk);channels=le16(chunk+2);rate=le32(chunk+4);
            alignment=le16(chunk+12);bits=le16(chunk+14);haveFormat=true;
        }
        else if (tag(header,"data")) { dataOffset=pos;dataSize=length; }
        else if (tag(header,"cue "))
        {
            if (length<4) return false;
            const auto count=le32(chunk);
            if (count>(length-4)/24) return false;
            for (uint32_t n=0;n<count;++n)
            {
                const auto* p=chunk+4+size_t(n)*24;
                SWaveFormatChunkCuePoint cue{};
                cue.cueId=le32(p);cue.playOrderPosition=le32(p+4);
                std::memcpy(cue.dataChunkId,p+8,4);
                cue.chunkStart=le32(p+12);cue.blockStart=le32(p+16);cue.sampleOffset=le32(p+20);
                cues.emplace(cue.cueId,cue);
            }
        }
        else if (outputCues && (tag(header,"LIST") || tag(header,"list")))
        {
            if (length<4) return false;
            if (tag(chunk,"adtl")) for (size_t offset=4;offset<length;)
            {
                if (length-offset<8) return false;
                const auto* sub=chunk+offset;
                const size_t subSize=le32(sub+4);offset+=8;
                if (subSize>length-offset) return false;
                if (tag(sub,"labl") || tag(sub,"note"))
                {
                    if (subSize<4) return false;
                    const auto id=le32(chunk+offset);
                    std::string name(reinterpret_cast<const char*>(chunk+offset+4),subSize-4);
                    while (!name.empty() && name.back()==0) name.pop_back();
                    if (!name.empty()) labels.emplace(id,std::move(name));
                }
                offset+=subSize;
                if (subSize&1) { if (offset==length) return false; ++offset; }
            }
        }
        pos+=length;
        if (length&1) { if (pos==end) return false; ++pos; }
    }
    const bool pcm=format==eFormat_PCM && (bits==8 || bits==16 || bits==24 || bits==32);
    const bool floating=format==eFormat_IEEE_FLOAT && (bits==32 || bits==64);
    if (!haveFormat || !dataOffset || (!pcm && !floating) || !channels || !rate) return false;
    const uint32_t bytesPerFrame=uint32_t(channels)*(bits/8);
    if (!alignment || alignment!=bytesPerFrame || dataSize%alignment) return false;
    Data parsed;
    parsed.data=buffer+dataOffset;parsed.dataByteSize=dataSize;parsed.bitsPerSample=bits;
    parsed.channels=channels;parsed.samplerate=rate;parsed.isFloat=floating;
    std::vector<CuePoint> parsedCues;
    if (outputCues) for (const auto& entry:cues)
    {
        const auto& src=entry.second;
        CuePoint dst{};
        // RIFF cue offsets are sample-frame positions, independent of bit depth.
        dst.sampleOffset=src.playOrderPosition>0?src.playOrderPosition:src.sampleOffset;
        const auto label=labels.find(entry.first);
        if (label!=labels.end()) dst.name=label->second;
        parsedCues.push_back(std::move(dst));
    }
    result=parsed;
    if (outputCues) *outputCues=std::move(parsedCues);
    return true;
}
}
