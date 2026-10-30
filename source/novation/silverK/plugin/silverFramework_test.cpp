#include "baseLib/binarystream.h"
#include "synthLib/wavReader.h"
#include "synthLib/midiBufferParser.h"
#include "synthLib/sysexToMidi.h"
#include "synthLib/vstpreset.h"
#include "jucePluginLib/patchdb/jobqueue.h"
#include <atomic>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <limits>

namespace
{
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class F> void rejects(F f) { bool caught=false;try { f(); } catch (const std::range_error&) { caught=true; } require(caught,"Invalid stream read was accepted"); }
void put32(std::vector<uint8_t>& b,size_t at,uint32_t v,bool big=false) { for (int n=0;n<4;++n) b[at+n]=uint8_t(v>>(8*(big?3-n:n))); }
void four(std::vector<uint8_t>& b,size_t at,const char* s) { std::memcpy(b.data()+at,s,4); }
void streams()
{
    baseLib::BinaryStream s(std::vector<uint8_t>{'A','B','C','D'});
    require(s.read4CC("ABCD"),"FourCC comparison failed");s.setReadPos(0);
    require(!s.read4CC("EFGH"),"FourCC mismatch accepted");
    s.setReadPos(100);rejects([&]{s.read<uint8_t>();});
    baseLib::BinaryStream truncated(std::vector<uint8_t>{1,2});uint32_t out=0;
    rejects([&]{truncated.read(&out,1);});
    baseLib::BinaryStream parent(std::vector<uint8_t>{1,2,3,4});parent.setReadPos(100);
    baseLib::BinaryStream child(parent,4);rejects([&]{child.read<uint8_t>();});
    baseLib::BinaryStream goodParent(std::vector<uint8_t>{1,2,3,4});baseLib::BinaryStream goodChild(goodParent,2);
    require(goodChild.read<uint8_t>()==1 && goodParent.getReadPos()==2,"Valid substream broken");
    baseLib::BinaryStream huge(std::vector<uint8_t>{255,255,255,255});rejects([&]{huge.readString();});
    uint8_t fixed[4]{};baseLib::StreamBuffer bounded(fixed,4);bounded.seekp(100);
    require(!bounded.write(fixed,1),"Fixed write outside buffer accepted");
    baseLib::StreamBuffer overflow;overflow.seekp(std::numeric_limits<size_t>::max());
    require(!overflow.write(fixed,1),"Write position overflow accepted");
}
std::vector<uint8_t> wave()
{
    std::vector<uint8_t> b(48);four(b,0,"RIFF");put32(b,4,40);four(b,8,"WAVE");
    four(b,12,"fmt ");put32(b,16,16);b[20]=1;b[22]=1;put32(b,24,48000);put32(b,28,96000);
    b[32]=2;b[34]=16;four(b,36,"data");put32(b,40,4);b[44]=1;b[46]=2;return b;
}
void waves()
{
    auto b=wave();synthLib::Data d;std::vector<synthLib::CuePoint> cues(1);
    require(synthLib::WavReader::load(d,&cues,b.data(),b.size()),"Valid PCM WAV rejected");
    require(d.samplerate==48000 && d.dataByteSize==4 && d.bitsPerSample==16 && cues.empty(),"Incorrect WAV data");
    for (size_t n=0;n<b.size();++n) require(!synthLib::WavReader::load(d,nullptr,b.data(),n),"Truncated WAV accepted");
    require(!synthLib::WavReader::load(d,nullptr,nullptr,0),"Null WAV accepted");
    for (const auto offset:{20,22,32,34}) { auto bad=b;bad[size_t(offset)]=0;require(!synthLib::WavReader::load(d,nullptr,bad.data(),bad.size()),"Invalid WAV format accepted"); }
    auto bad=b;put32(bad,40,1000);require(!synthLib::WavReader::load(d,nullptr,bad.data(),bad.size()),"Oversized WAV data accepted");
    // Malformed cue and LIST lengths must fail before touching their payloads.
    bad=b;four(bad,36,"cue ");put32(bad,44,100);require(!synthLib::WavReader::load(d,&cues,bad.data(),bad.size()),"Invalid cue count accepted");
    bad=b;four(bad,36,"LIST");four(bad,44,"adtl");require(!synthLib::WavReader::load(d,&cues,bad.data(),bad.size()),"Missing data accepted");
    // Valid odd-sized ancillary chunk with its RIFF pad byte.
    auto odd=b;odd.insert(odd.begin()+36,10,0);four(odd,36,"JUNK");put32(odd,40,1);put32(odd,4,uint32_t(odd.size()-8));
    require(synthLib::WavReader::load(d,nullptr,odd.data(),odd.size()),"Padded WAV chunk rejected");
    auto withCue=b;withCue.resize(84);four(withCue,48,"cue ");put32(withCue,52,28);put32(withCue,56,1);
    put32(withCue,60,7);four(withCue,68,"data");put32(withCue,80,1);put32(withCue,4,76);
    require(synthLib::WavReader::load(d,&cues,withCue.data(),withCue.size()) && cues.size()==1 && cues[0].sampleOffset==1,"WAV cue sample offset changed");
    auto floating=b;floating[20]=3;floating[34]=32;floating[32]=4;put32(floating,28,192000);
    require(synthLib::WavReader::load(d,nullptr,floating.data(),floating.size()) && d.isFloat,"Valid float WAV rejected");
    for (size_t n=0;n<b.size();++n) for (const auto value:{uint8_t(0),uint8_t(255)}) { auto fuzz=b;fuzz[n]=value;synthLib::WavReader::load(d,&cues,fuzz.data(),fuzz.size()); }
}
std::vector<synthLib::SMidiEvent> parse(const std::vector<uint8_t>& b)
{
    synthLib::MidiBufferParser parser(synthLib::MidiEventSource::Host);parser.write(b);
    std::vector<synthLib::SMidiEvent> result;parser.getEvents(result);return result;
}
void midi()
{
    auto events=parse({0x90,60,0xf8,100});
    require(events.size()==2 && events[0].a==0xf8 && events[1].a==0x90 && events[1].b==60 && events[1].c==100,"Realtime byte corrupts note");
    events=parse({0x90,60,100,61,0xfe,101});
    require(events.size()==3 && events[1].a==0xfe && events[2].b==61 && events[2].c==101,"Running status or active sensing broken");
    events=parse({0xf0,1,0xf8,2,0xf7});
    require(events.size()==2 && events[0].a==0xf8 && events[1].sysex==synthLib::SysexBuffer({0xf0,1,2,0xf7}),"SysEx realtime handling broken");
    events=parse({0x90,60,100,0xf1,1,61,101});
    require(events.size()==2 && events[1].a==0xf1 && events[1].b==1 && events[1].c==0,"System common did not cancel running status");
    events=parse({0x90,60,0x80,60,0});require(events.size()==1 && events[0].a==0x80,"New status did not replace incomplete note");
    events=parse({0x90,60,100,0xf0,1,0xf7,61,100});require(events.size()==2,"SysEx did not cancel running status");
}
void exports()
{
    for (const auto count:{0,1,2})
    {
        synthLib::SysexBufferList messages;for(int n=0;n<count;++n) messages.push_back({0xf0,1,0xf7});
        std::ostringstream out(std::ios::binary);synthLib::SysexToMidi::write(out,messages);const auto b=out.str();
        uint32_t length=0;for(size_t n=18;n<22;++n) length=(length<<8)|uint8_t(b[n]);
        require(b.size()==size_t(length)+22,"Incorrect MIDI track length");
        require(b.substr(b.size()-4)==std::string("\0\xff\x2f\0",4),"End of track missing zero delta");
    }
}
void presets()
{
    std::vector<uint8_t> bank(164);four(bank,0,"CcnK");put32(bank,4,156,true);four(bank,8,"FBCh");put32(bank,12,1,true);put32(bank,24,128,true);put32(bank,156,4,true);bank[160]=1;
    auto loaded=synthLib::VstPreset::read(bank);require(loaded && loaded->size()==1 && loaded->front().data.size()==4,"Opaque FXB bank rejected");
    put32(bank,156,0xffffffff,true);require(!synthLib::VstPreset::read(bank),"Oversized FXB allocation accepted");
    std::vector<uint8_t> vst3(80);four(vst3,0,"VST3");put32(vst3,4,1);put32(vst3,40,52);four(vst3,48,"data");four(vst3,52,"List");put32(vst3,56,1);four(vst3,60,"Comp");put32(vst3,64,48);put32(vst3,72,4);
    loaded=synthLib::VstPreset::read(vst3);require(loaded && loaded->size()==1 && loaded->front().data[0]=='d',"Valid VST3 chunk rejected");
    put32(vst3,44,1);require(!synthLib::VstPreset::read(vst3),"Unrepresentable VST3 list offset accepted");
    put32(vst3,44,0);put32(vst3,64,1000);require(!synthLib::VstPreset::read(vst3),"VST3 offset beyond file accepted");
    put32(vst3,64,48);put32(vst3,72,1);loaded=synthLib::VstPreset::read(vst3);
    require(loaded && loaded->front().data.size()==1,"Short opaque VST3 chunk rejected");
    vst3.resize(3);require(!synthLib::VstPreset::read(vst3),"Truncated VST header accepted");
}
void queues()
{
    using pluginLib::patchDB::JobQueue;
    for(int run=0;run<8;++run)
    {
        JobQueue idle("",true,dsp56k::ThreadPriority::Normal,4);idle.waitEmpty();idle.destroy();require(idle.destroyed(),"Queue did not stop");
        std::atomic<int> count{0};JobQueue work("",false,dsp56k::ThreadPriority::Normal,4);
        for(int n=0;n<100;++n) work.add([&]{++count;});work.start();work.destroy();
        require(count==100 && work.empty(),"Stopping lost queued jobs");
        work.start();work.add([&]{++count;});work.waitEmpty();work.destroy();require(count==101,"Queue restart failed");
    }
}
}
int main()
{
    try { streams();waves();midi();exports();presets();queues();std::cout<<"PASS: stream/WAV safety, MIDI parsing/export, preset import and queue shutdown\n";return 0; }
    catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
