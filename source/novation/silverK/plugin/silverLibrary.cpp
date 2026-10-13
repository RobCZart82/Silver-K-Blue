#include "silverLibrary.h"
#include "jucePluginLib/patchdb/search.h"
namespace silverK {
using namespace pluginLib::patchDB;
std::shared_ptr<Library> sharedLibrary(const juce::File& dir){static std::mutex mutex;static std::map<juce::String,std::weak_ptr<Library>> instances;std::lock_guard guard(mutex);auto& weak=instances[dir.getFullPathName()];if(auto existing=weak.lock())return existing;auto library=std::make_shared<Library>(dir);weak=library;return library;}

Library::Library(const juce::File& dir):DB(dir){startLoaderThread();}
Library::~Library(){stopLoaderThread();}
PatchPtr Library::initializePatch(Data&& data,const std::string& fallback){if(!validPatch(data))return {};auto p=std::make_shared<Patch>();const auto model=modelOf(data);p->sysex=std::move(data);p->bank=p->sysex[11];p->program=p->sysex[12];const bool perf=p->sysex.size()==1038;const unsigned offset=perf?192:model==Model::KS?151:0;
 if(model==Model::KS){juce::String name;for(unsigned i=offset;i<offset+16;++i){auto c=p->sysex[13+i];name+=juce::String::charToString(c>=32 && c<127?c:' ');}p->name=name.trim().toStdString();}if(p->name.empty())p->name=fallback+" "+std::to_string(p->bank*100+p->program);
 p->tags.add(TagType::CustomA,modelName(model).toStdString());p->tags.add(TagType::Category,perf?"Performance":"Program");p->setHashFromMessages(13,1);return p;}
Data Library::applyModifications(const PatchPtr& p,const pluginLib::FileType&,pluginLib::ExportType)const{return p->sysex;}
void Library::processDirty(const Dirty& d)const{if(d.patches || d.dataSources || !d.tags.empty())++revision;if(!d.errors.empty())error=d.errors.back();}
void Library::addPath(const juce::File& f){DataSource ds;ds.type=f.isDirectory()?SourceType::Folder:SourceType::File;ds.origin=DataSourceOrigin::Manual;ds.name=f.getFullPathName().toStdString();addDataSource(ds);}
bool Library::loadFile(DataList& data,const std::string& path){const juce::File file(path);if(!file.hasFileExtension("syx;mid;midi;zip"))return false;if(!file.hasFileExtension("zip"))return DB::loadFile(data,path);if(file.getSize()>8*1024*1024)return false;juce::ZipFile zip(file);for(int i=0;i<zip.getNumEntries();++i){const auto* entry=zip.getEntry(i);if(entry->uncompressedSize>8*1024*1024 || !juce::File(entry->filename).hasFileExtension("syx;mid;midi"))continue;std::unique_ptr<juce::InputStream> in(zip.createStreamForEntry(i));juce::MemoryBlock block;if(!in)continue;in->readIntoMemoryBlock(block,8*1024*1024);const auto* p=static_cast<const uint8_t*>(block.getData());parseFileData(data,Data(p,p+block.getSize()),entry->filename.toStdString());}return !data.empty();}
void Library::addFactory(const std::vector<std::vector<uint8_t>>& data,const juce::String& name){DataSource ds;ds.type=SourceType::LocalStorage;ds.origin=DataSourceOrigin::Manual;ds.name=name.toStdString();if(getDataSource(ds))return;auto node=addDataSource(ds);std::vector<PatchPtr> patches;for(const auto& bytes:data)if(auto p=createPatch(Data(bytes.begin(),bytes.end()),name.toStdString()))patches.push_back(p);if(!patches.empty())copyPatchesTo(node,patches);}
}
