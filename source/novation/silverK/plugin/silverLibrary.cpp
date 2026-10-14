#include "silverLibrary.h"
#include "silverVStation.h"
#include <cstring>
#include "baseLib/md5.h"
#include "jucePluginLib/patchdb/search.h"
#include "jucePluginLib/patchdb/patchmodifications.h"
namespace silverK {
using namespace pluginLib::patchDB;
namespace {
constexpr uint8_t NamedRecord[]{'S','K','P','N'};
juce::String numberedName(Model model,unsigned number){return modelName(model)+" "+juce::String(number).paddedLeft('0',3);}
Data namedRecord(const Data& sysex,const juce::String& name){Data record(std::begin(NamedRecord),std::end(NamedRecord));record.insert(record.end(),sysex.begin(),sysex.end());const auto utf8=name.toRawUTF8();record.insert(record.end(),utf8,utf8+std::strlen(utf8));return record;}
bool unpackNamedRecord(Data& record,std::string& name){
 if(record.size()<=146 || record.size()>146+512 || !std::equal(std::begin(NamedRecord),std::end(NamedRecord),record.begin()) || !validPatch(record.data()+4,142))return false;
 name=juce::String::fromUTF8(reinterpret_cast<const char*>(record.data()+146),int(record.size()-146)).trim().toStdString();
 Data sysex(record.begin()+4,record.begin()+146);record=std::move(sysex);return true;
}
}
Model patchModel(const Patch& patch){
 const auto model=modelOf(patch.sysex);
 // CustomA is the source synth type. Recognize the old release's V tag too,
 // so cached banks keep their identity before the one-time source refresh.
 if(model==Model::KStation && (patch.tags.containsAdded(TagType::CustomA,"V-Station") || patch.tags.containsAdded(TagType::Tag,"V-Station")))return Model::VStation;
 return model;
}
juce::String patchName(const Patch& patch){
 const auto name=juce::String(patch.getName()).trim();const auto model=patchModel(patch);
 if(patch.modifications && !patch.modifications->name.empty())return name;
 if((model==Model::KStation || model==Model::AStation) && (name.isEmpty() || name.containsOnly("0123456789")))return numberedName(model,patch.program==g_invalidProgram?1:patch.program+1);
 return name;
}
std::shared_ptr<Library> sharedLibrary(const juce::File& dir){static std::mutex mutex;static std::map<juce::String,std::weak_ptr<Library>> instances;std::lock_guard guard(mutex);auto& weak=instances[dir.getFullPathName()];if(auto existing=weak.lock())return existing;auto library=std::make_shared<Library>(dir);weak=library;return library;}

Library::Library(const juce::File& dir):DB(dir),importVersion(dir.getChildFile("silver-import-version.txt")),migrateImports(importVersion.loadFileAsString().trim()!="3"){
 startLoaderThread();
 if(migrateImports)runOnLoaderThread([this]{std::vector<DataSourceNodePtr> sources;getDataSources(sources);for(const auto& ds:sources)if(ds->origin==DataSourceOrigin::Manual && (ds->type==SourceType::File || ds->type==SourceType::Folder) && juce::File(ds->name).exists())refreshDataSource(ds);});
}
Library::~Library(){stopLoaderThread();}
void Library::onLoadFinished(){if(migrateImports && importVersion.replaceWithText("3\n"))migrateImports=false;}
PatchPtr Library::initializePatch(Data&& data,const std::string& fallback){const auto original=data;std::string importedName;const bool vStation=unpackVStationPatch(data,importedName);if(!vStation)unpackNamedRecord(data,importedName);if(!validPatch(data))return {};auto p=std::make_shared<Patch>();const auto model=modelOf(data);p->sysex=std::move(data);p->bank=p->sysex[11];p->program=p->sysex[12];const bool perf=p->sysex.size()==1038;const unsigned offset=perf?192:(model==Model::KS || model==Model::XStation)?151:0;
 if(model==Model::KS || model==Model::XStation){juce::String name;for(unsigned i=offset;i<offset+16;++i){auto c=p->sysex[13+i];name+=juce::String::charToString(c>=32 && c<127?c:' ');}p->name=name.trim().toStdString();}if(!importedName.empty())p->name=importedName;if(p->name.empty()){const auto stem=juce::String(fallback).trim();p->name=(stem.isNotEmpty()?stem:numberedName(vStation?Model::VStation:model,p->bank*100+p->program+1)).toStdString();}
 p->tags.add(TagType::CustomA,modelName(vStation?Model::VStation:model).toStdString());p->tags.add(TagType::Category,perf?"Performance":"Program");p->setHashFromMessages(13,1);if(vStation){p->tags.add(TagType::Tag,"V-Station");const baseLib::MD5 digest(original.data(),uint32_t(original.size()));std::memcpy(p->hash.data(),digest.getWords().data(),p->hash.size());}return p;}
Data Library::applyModifications(const PatchPtr& p,const pluginLib::FileType&,pluginLib::ExportType)const{return p->sysex;}
void Library::processDirty(const Dirty& d)const{if(d.patches || d.dataSources || !d.tags.empty())++revision;if(!d.errors.empty())error=d.errors.back();}
void Library::addPath(const juce::File& f){DataSource ds;ds.type=f.isDirectory()?SourceType::Folder:SourceType::File;ds.origin=DataSourceOrigin::Manual;ds.name=f.getFullPathName().toStdString();if(getDataSource(ds))return;addDataSource(ds);}
void Library::requestBankSources(std::function<void(std::vector<DataSourceNodePtr>&&) > callback){
 // Read the import tree on its owning loader thread. Individual files remain
 // searchable in the table; only collections belong in the bank selector.
 runOnLoaderThread([this,callback=std::move(callback)]()mutable{
  std::vector<DataSourceNodePtr> all,banks;getDataSources(all);std::set<DataSourceNodePtr> populatedFolders;
  for(const auto& ds:all)if(!ds->patches.empty())for(auto parent=ds->getParent();parent;parent=parent->getParent())populatedFolders.insert(parent);
  for(const auto& ds:all)if((ds->type==SourceType::Folder && populatedFolders.count(ds)) || (ds->type==SourceType::File && ds->patches.size()>1) || ((ds->type==SourceType::LocalStorage || ds->type==SourceType::Rom) && !ds->patches.empty()))banks.push_back(ds);
  runOnUiThread([callback=std::move(callback),banks=std::move(banks)]()mutable{callback(std::move(banks));});
 });
}
bool Library::parseFileData(DataList& output,const Data& input,const std::string& filename){
 if(juce::File(filename).hasFileExtension("fxb;fxp") || (input.size()>=4 && std::memcmp(input.data(),"CcnK",4)==0))return parseVStationBank(output,input) || parseVStationProgram(output,input);
 DataList frames;if(!DB::parseFileData(frames,input,filename))return false;
 const auto before=output.size();
 const bool singleNamed=frames.size()==1 && frames.front().size()==142;
 auto append=[&](Data&& frame){
  const auto model=modelOf(frame);
  if(model==Model::KStation || model==Model::AStation){
   const auto stem=juce::File(filename).getFileNameWithoutExtension();
   const auto address=frame[7]==1 && (frame[11] || frame[12])?unsigned(frame[11])*100+frame[12]:unsigned(output.size()-before)+1;
   const auto name=singleNamed && stem.isNotEmpty()?stem:numberedName(model,address);
   output.push_back(namedRecord(frame,name));
  }else output.push_back(std::move(frame));
 };
 for(auto& frame:frames){
  if(validPatch(frame)){append(std::move(frame));continue;}
  const auto model=modelOf(frame);
  // A/K bank messages hold two consecutive 128-byte programs, rather than
  // a KS Performance. Split them without altering either sound payload.
  if((model!=Model::AStation && model!=Model::KStation) || frame.size()!=270 || frame[7]!=2 || frame.back()!=0xf7 || (frame[12]>98 || (frame[12]&1)) || !std::all_of(frame.begin()+1,frame.end()-1,[](uint8_t b){return b<128;}))continue;
  for(unsigned part=0;part<2;++part){
   Data program(frame.begin(),frame.begin()+13);program[7]=1;program[12]+=uint8_t(part);
   program.insert(program.end(),frame.begin()+13+128*part,frame.begin()+13+128*(part+1));program.push_back(0xf7);
   append(std::move(program));
  }
 }
 return output.size()!=before;
}
bool Library::loadFile(DataList& data,const std::string& path){const juce::File file(path);if(!file.hasFileExtension("syx;mid;midi;fxb;fxp;zip"))return false;if(!file.hasFileExtension("zip"))return DB::loadFile(data,path);if(file.getSize()>8*1024*1024)return false;juce::ZipFile zip(file);for(int i=0;i<zip.getNumEntries();++i){const auto* entry=zip.getEntry(i);if(entry->uncompressedSize>8*1024*1024 || !juce::File(entry->filename).hasFileExtension("syx;mid;midi;fxb;fxp"))continue;std::unique_ptr<juce::InputStream> in(zip.createStreamForEntry(i));juce::MemoryBlock block;if(!in)continue;in->readIntoMemoryBlock(block,8*1024*1024);const auto* p=static_cast<const uint8_t*>(block.getData());parseFileData(data,Data(p,p+block.getSize()),entry->filename.toStdString());}return !data.empty();}
void Library::addFactory(const std::vector<std::vector<uint8_t>>& data,const juce::String& name){DataSource ds;ds.type=SourceType::LocalStorage;ds.origin=DataSourceOrigin::Manual;ds.name=name.toStdString();if(getDataSource(ds))return;auto node=addDataSource(ds);std::vector<PatchPtr> patches;for(const auto& bytes:data)if(auto p=createPatch(Data(bytes.begin(),bytes.end()),name.toStdString()))patches.push_back(p);if(!patches.empty())copyPatchesTo(node,patches);}
}
