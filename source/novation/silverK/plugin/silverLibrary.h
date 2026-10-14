#pragma once
#include "jucePluginLib/patchdb/db.h"
#include "silverPatch.h"
#include <atomic>
namespace silverK {
Model patchModel(const pluginLib::patchDB::Patch&);
juce::String patchName(const pluginLib::patchDB::Patch&);
class Library final:public pluginLib::patchDB::DB {
public:
 explicit Library(const juce::File& directory);
 ~Library()override;
 mutable std::atomic<unsigned> revision{0};
 mutable juce::String error;
 bool requestPatchForPart(pluginLib::patchDB::Data&,uint32_t,uint64_t)override{return false;}
 bool loadRomData(pluginLib::patchDB::DataList&,uint32_t,uint32_t)override{return false;}
 pluginLib::patchDB::PatchPtr initializePatch(pluginLib::patchDB::Data&&,const std::string&)override;
 pluginLib::patchDB::Data applyModifications(const pluginLib::patchDB::PatchPtr&,const pluginLib::FileType&,pluginLib::ExportType)const override;
 void processDirty(const pluginLib::patchDB::Dirty&)const override;
 bool parseFileData(pluginLib::patchDB::DataList&,const pluginLib::patchDB::Data&,const std::string&)override;
 bool loadFile(pluginLib::patchDB::DataList&,const std::string&)override;
 void addPath(const juce::File&);
 void addFactory(const std::vector<std::vector<uint8_t>>&,const juce::String&);
 void requestBankSources(std::function<void(std::vector<pluginLib::patchDB::DataSourceNodePtr>&&) >);
protected:
 void onLoadFinished()override;
private:
 juce::File importVersion;
 bool migrateImports=false;
};
std::shared_ptr<Library> sharedLibrary(const juce::File&);
}
