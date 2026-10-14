#include "silverBrowser.h"
namespace silverK {
using namespace pluginLib::patchDB;
namespace {juce::String tagText(const PatchPtr& p,TagType type){juce::StringArray tags;for(const auto& t:p->getTags(type).getAdded())tags.add(t);tags.sort(true);return tags.joinIntoString(", ");}}
Browser::Browser(Processor& processor):p(processor){
 for(auto* c:std::initializer_list<juce::Component*>{&table,&search,&tags,&tagFilter,&model,&source,&favourite,&onlyFavourites,&addFolder,&addFile,&refresh,&remove,&saveTags,&exportPatch,&saveCurrent,&info})addAndMakeVisible(*c);
 table.setComponentID("patchTable");table.setModel(this);table.setColour(juce::ListBox::backgroundColourId,juce::Colour(0xffe5e7ea));table.setRowHeight(29);int id=1;for(const auto& col:std::initializer_list<std::pair<const char*,int>>{{"Name",260},{"Model",90},{"Type",100},{"Tags",210},{"Compatibility",190},{"Source",280}})table.getHeader().addColumn(col.first,id++,col.second,40,900);
 search.setComponentID("patchSearch");model.setComponentID("patchModel");tagFilter.setComponentID("patchTagFilter");source.setComponentID("patchSource");
 table.getHeader().setSortColumnId(1,true);
 search.setTextToShowWhenEmpty("Search patch name",juce::Colours::grey);tagFilter.setTextToShowWhenEmpty("Filter tag",juce::Colours::grey);tags.setTextToShowWhenEmpty("Tags, separated by commas",juce::Colours::grey);
 for(const auto& name:juce::StringArray{"All models","K-Station","KS","A-Station","X-Station","V-Station"})model.addItem(name,model.getNumItems()+1);model.setSelectedId(1,juce::dontSendNotification);
 search.onTextChange=tagFilter.onTextChange=[this]{dirty=true;};model.onChange=onlyFavourites.onClick=[this]{dirty=true;};source.onChange=[this]{const int n=source.getSelectedId()-2;preferredSource=n>=0 && n<int(sources.size())?sources[n]:nullptr;dirty=true;};
 addFolder.onClick=[this]{choose(true);};addFile.onClick=[this]{choose(false);};
 refresh.onClick=[this]{const int n=source.getSelectedId()-2;if(n>=0 && n<int(sources.size()))p.patchLibrary.refreshDataSource(sources[n]);else{std::vector<DataSourceNodePtr> all;p.patchLibrary.getDataSources(all);for(const auto& ds:all)if(ds->origin==DataSourceOrigin::Manual)p.patchLibrary.refreshDataSource(ds);}};
 remove.onClick=[this]{const int n=source.getSelectedId()-2;if(n>=0 && n<int(sources.size()))p.patchLibrary.removeDataSource(*sources[n]);};
 exportPatch.onClick=[this]{choose(false,true);};saveCurrent.onClick=[this]{choose(false,true,true);};
 saveTags.onClick=[this]{const int row=table.getSelectedRow();if(row<0 || row>=int(rows.size()))return;const auto& patch=rows[row];TypedTags edits;juce::StringArray added;added.addTokens(tags.getText(),",",{});added.trim();added.removeEmptyStrings();for(const auto& old:patch->getTags(TagType::Tag).getAdded())if(!added.contains(old))edits.addRemoved(TagType::Tag,old);for(const auto& t:added)edits.add(TagType::Tag,t.toStdString());if(favourite.getToggleState())edits.add(TagType::Favourites,"Favourite");else for(const auto& old:patch->getTags(TagType::Favourites).getAdded())edits.addRemoved(TagType::Favourites,old);p.patchLibrary.modifyTags({patch},edits);};
 startTimerHz(10);
}
Browser::~Browser(){stopTimer();if(handle!=g_invalidSearchHandle)p.patchLibrary.cancelSearch(handle);table.setModel(nullptr);}
void Browser::resized(){auto area=getLocalBounds().reduced(8);auto top=area.removeFromTop(32);search.setBounds(top.removeFromLeft(270).reduced(2));model.setBounds(top.removeFromLeft(160).reduced(2));tagFilter.setBounds(top.removeFromLeft(180).reduced(2));source.setBounds(top.removeFromLeft(320).reduced(2));onlyFavourites.setBounds(top);area.removeFromTop(8);auto actions=area.removeFromTop(32);for(auto* b:{&addFolder,&addFile,&refresh,&remove,&saveCurrent})b->setBounds(actions.removeFromLeft(140).reduced(2));area.removeFromTop(8);auto bottom=area.removeFromBottom(72);info.setBounds(bottom.removeFromBottom(32));tags.setBounds(bottom.removeFromLeft(420).reduced(2));favourite.setBounds(bottom.removeFromLeft(130));for(auto* b:{&saveTags,&exportPatch})b->setBounds(bottom.removeFromLeft(135).reduced(2));table.setBounds(area);}
void Browser::query(){resultsDirty=true;if(handle!=g_invalidSearchHandle)p.patchLibrary.cancelSearch(handle);SearchRequest request;request.name=search.getText().toStdString();const int n=source.getSelectedId()-2;if(n>=0 && n<int(sources.size()))request.sourceNode=sources[n];const auto selected=model.getSelectedId()>1?model.getText():juce::String{};const auto tag=tagFilter.getText().trim();const bool favourites=onlyFavourites.getToggleState();request.customCompareFunc=[selected,tag,favourites](const Patch& patch){if(!selected.isEmpty() && modelName(patchModel(patch))!=selected)return false;if(favourites && patch.getTags(TagType::Favourites).getAdded().empty())return false;if(!tag.isEmpty()){bool match=false;for(const auto& t:patch.getTags(TagType::Tag).getAdded())match|=juce::String(t).containsIgnoreCase(tag);if(!match)return false;}return true;};handle=p.patchLibrary.search(std::move(request));}
void Browser::timerCallback(){
 p.patchLibrary.uiProcess();
 if(lastK!=p.isKStation()){lastK=p.isKStation();updateRows(rows);table.repaint();}
 if(revision!=p.patchLibrary.revision.load()){
  revision=p.patchLibrary.revision.load();
  const auto request=++bankRequest;const juce::Component::SafePointer<Browser> self(this);
  p.patchLibrary.requestBankSources([self,request](std::vector<DataSourceNodePtr>&& banks){if(self && self->bankRequest==request)self->updateBanks(std::move(banks));});
 }
 if(dirty){dirty=false;query();}
 if(auto result=p.patchLibrary.getSearch(handle)){
  std::vector<PatchPtr> next;
  {std::shared_lock lock(result->resultsMutex);next.assign(result->results.begin(),result->results.end());}
  if(resultsDirty || next!=searchRows){resultsDirty=false;searchRows=next;updateRows(std::move(next));}
 }
}
void Browser::updateBanks(std::vector<DataSourceNodePtr>&& banks){
 sources=std::move(banks);source.clear(juce::dontSendNotification);source.addItem("All sources",1);int selectedId=1;
 for(size_t i=0;i<sources.size();++i){source.addItem(juce::File(sources[i]->name).getFileName(),int(i)+2);if(preferredSource && *preferredSource==*sources[i]){selectedId=int(i)+2;preferredSource=sources[i];}}
 source.setSelectedId(selectedId,juce::dontSendNotification);dirty=true;
}
void Browser::sortOrderChanged(int column,bool ascending){sortColumn=column;sortAscending=ascending;updateRows(rows);}
void Browser::updateRows(std::vector<PatchPtr> next){
 std::sort(next.begin(),next.end(),[this](const auto& a,const auto& b){
  int order=cellText(a,sortColumn).compareNatural(cellText(b,sortColumn));
  if(!order)order=patchName(*a).compareNatural(patchName(*b));
  if(!order){const auto as=a->source.lock(),bs=b->source.lock();order=juce::String(as?as->name:"").compareNatural(bs?bs->name:"");}
  if(!order && a->program!=b->program)order=a->program<b->program?-1:1;
  if(!order && a->hash!=b->hash)order=a->hash<b->hash?-1:1;
  return sortAscending?order<0:order>0;
 });
 if(next==rows){table.repaint();return;}
 const int selected=table.getSelectedRow();const auto old=selected>=0 && selected<int(rows.size())?rows[selected]:nullptr;
 const juce::ScopedValueSetter<bool> updating(updatingRows,true);
 table.deselectAllRows();rows=std::move(next);table.updateContent();
 if(old){const auto it=std::find(rows.begin(),rows.end(),old);if(it!=rows.end())table.selectRow(int(it-rows.begin()));}
 selectedRowsChanged(table.getSelectedRow());table.repaint();
}
void Browser::paintRowBackground(juce::Graphics& g,int row,int width,int height,bool selected){g.fillAll(selected?juce::Colour(0xff087fe5):row%2?juce::Colour(0xffd5d8dc):juce::Colour(0xffe5e7ea));}
juce::String Browser::cellText(const PatchPtr& patch,int column)const{
 const auto m=patchModel(*patch);
 switch(column){
 case 1:return patchName(*patch);case 2:return modelName(m);
 case 3:return patch->sysex.size()==1038?"Performance":"Program";
 case 4:return (patch->getTags(TagType::Favourites).getAdded().empty()?juce::String{}:juce::String::fromUTF8("★ "))+tagText(patch,TagType::Tag);
 case 5:if((p.isKStation() && (m==Model::KStation || m==Model::AStation || m==Model::VStation)) || (!p.isKStation() && (m==Model::KS || m==Model::XStation)))return "Native compatible";return p.isKStation()?"Converted: sine / LP 12":"Converted";
 case 6:if(auto src=patch->source.lock())return src->name;break;
 }
 return {};
}
void Browser::paintCell(juce::Graphics& g,int row,int column,int width,int height,bool selected){if(row<0 || row>=int(rows.size()))return;g.setColour(selected?juce::Colours::white:juce::Colour(0xff171b20));g.setFont(13.f);g.drawText(cellText(rows[row],column),5,0,width-10,height,juce::Justification::centredLeft,true);}
void Browser::selectedRowsChanged(int row){if(row<0 || row>=int(rows.size())){tags.clear();favourite.setToggleState(false,juce::dontSendNotification);info.setText({},juce::dontSendNotification);return;}tags.setText(tagText(rows[row],TagType::Tag),false);favourite.setToggleState(!rows[row]->getTags(TagType::Favourites).getAdded().empty(),juce::dontSendNotification);info.setText(patchName(*rows[row])+" / "+modelName(patchModel(*rows[row])),juce::dontSendNotification);if(!updatingRows)activate(row);}
void Browser::cellClicked(int row,int,const juce::MouseEvent& event){
 if(row<0 || row>=int(rows.size()) || updatingRows)return;
 // Selection changes precede cellClicked. Re-clicking the selected row must
 // also load (e.g. after choosing another part), without loading twice for
 // the same mouse gesture that just changed the selection.
 const auto down=event.mouseDownTime.toMilliseconds();
 if(down>0 && lastActivatedPatch==rows[row] && lastActivationTime>=down)return;
 activate(row);
}
void Browser::activate(int row){if(row<0 || row>=int(rows.size()))return;lastActivatedPatch=rows[row];lastActivationTime=juce::Time::currentTimeMillis();juce::String message;const bool ok=p.activatePatch(rows[row],message);info.setText(ok?(message.isEmpty()?"Loaded "+patchName(*rows[row]):"Loaded; "+message.replace("\n","; ")):message,juce::dontSendNotification);info.setTooltip(message);}
void Browser::choose(bool folder,bool save,bool current){chooser=std::make_unique<juce::FileChooser>(save?"Save patch":folder?"Add patch folder":"Add patches",p.patchesFolder(),save?"*.syx":"*.syx;*.mid;*.midi;*.fxb;*.fxp;*.zip");const juce::Component::SafePointer<Browser> self(this);chooser->launchAsync((save?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode)|(folder?juce::FileBrowserComponent::canSelectDirectories:juce::FileBrowserComponent::canSelectFiles),[self,save,current](const juce::FileChooser& c){if(!self)return;auto file=c.getResult();if(file==juce::File{})return;if(!save){self->p.patchLibrary.addPath(file);return;}file=file.withFileExtension("syx");juce::String error;if(current){self->p.saveCurrentPatch(file,error);self->info.setText(error.isEmpty()?"Saved current sound":error,juce::dontSendNotification);}else{int row=self->table.getSelectedRow();if(row>=0 && row<int(self->rows.size()))self->p.patchLibrary.writePatchesToFile(file,{self->rows[row]});}});}
}
