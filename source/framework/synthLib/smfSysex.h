#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <vector>

namespace synthLib
{
// Extract only complete messages. Track/event lengths are authoritative; a
// continuation's F7 status and delta time are never part of the wire message.
template<class Bytes>
inline bool extractSmfSysex(const Bytes& file, std::vector<Bytes>& output)
{
    struct Reader
    {
        const Bytes& bytes;
        size_t pos, end;
        bool vlq(uint32_t& value)
        {
            value=0;
            for(int n=0;n<4 && pos<end;++n)
            {
                const auto b=bytes[pos++];value=(value<<7)|(b&127);
                if(!(b&128))return true;
            }
            return false;
        }
    };
    auto be16=[&](size_t p){return (uint32_t(file[p])<<8)|file[p+1];};
    auto be32=[&](size_t p){return (be16(p)<<16)|be16(p+2);};
    if(file.size()<14 || std::memcmp(file.data(),"MThd",4))return false;
    const auto header=be32(4),format=be16(8),tracks=be16(10);
    if(header<6 || header>file.size()-8 || format>2 || !tracks ||
       (format==0 && tracks!=1) || !be16(12))return false;
    size_t pos=8+header;uint32_t seen=0;
    std::vector<Bytes> parsed;
    while(pos<file.size())
    {
        if(file.size()-pos<8)return false;
        const auto length=be32(pos+4);
        if(length>file.size()-pos-8)return false;
        const bool track=std::memcmp(file.data()+pos,"MTrk",4)==0;
        pos+=8;const size_t end=pos+length;
        if(!track){pos=end;continue;}
        if(++seen>tracks)return false;
        Reader r{file,pos,end};uint8_t running=0;
        Bytes pending;
        while(r.pos<r.end)
        {
            uint32_t delta=0;if(!r.vlq(delta) || r.pos==r.end)return false;
            auto status=file[r.pos];
            if(status&128)++r.pos;
            else if(running)status=running;
            else return false;
            if(status==0xff)
            {
                running=0;
                if(r.pos==r.end)return false;
                const auto type=file[r.pos++];uint32_t size=0;
                if(type>127 || !r.vlq(size) || size>r.end-r.pos)return false;
                r.pos+=size;
                if(type==0x2f)
                {
                    if(size || r.pos!=r.end)return false;
                    break;
                }
            }
            else if(status==0xf0 || status==0xf7)
            {
                running=0;uint32_t size=0;
                if(!r.vlq(size) || size>r.end-r.pos)return false;
                const auto bodyEnd=r.pos+size;
                if(status==0xf0)
                {
                    if(!pending.empty())return false;
                    pending.push_back(0xf0);
                }
                else if(pending.empty())
                {
                    // Standalone F7 is an escape: only explicit F0...F7
                    // frames in its payload can represent complete SysEx.
                    for(;r.pos<bodyEnd;++r.pos)
                    {
                        const auto b=file[r.pos];
                        if(b==0xf0)pending={b};
                        else if(!pending.empty())
                        {
                            if(b==0xf7){pending.push_back(b);parsed.push_back(std::move(pending));pending.clear();}
                            else if(b<128)pending.push_back(b);
                            else pending.clear();
                        }
                    }
                    if(!pending.empty())return false;
                    continue;
                }
                for(;r.pos<bodyEnd;++r.pos)
                {
                    const auto b=file[r.pos];
                    if(b==0xf7 && r.pos+1==bodyEnd)
                    {
                        pending.push_back(b);parsed.push_back(std::move(pending));pending.clear();
                    }
                    else if(b<128)pending.push_back(b);
                    else return false;
                }
            }
            else if(status>=0x80 && status<0xf0)
            {
                if(!pending.empty())return false;
                running=status;
                const size_t count=(status&0xe0)==0xc0?1:2;
                if(count>r.end-r.pos)return false;
                for(size_t n=0;n<count;++n)if(file[r.pos++]>=128)return false;
            }
            else return false;
        }
        if(!pending.empty())return false;
        pos=end;
    }
    if(seen!=tracks)return false;
    output.insert(output.end(),std::make_move_iterator(parsed.begin()),std::make_move_iterator(parsed.end()));
    return true;
}
}
