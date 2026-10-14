#pragma once
#include "silverProcessor.h"
#include "jucePluginLib/patchdb/search.h"
namespace silverK {
class Browser final:public juce::Component,private juce::TableListBoxModel,private juce::Timer {
public:
 explicit Browser(Processor&);~Browser()override;
 void resized()override;
 void pollLibrary(){timerCallback();}
private:
 Processor& p;
 juce::TableListBox table;
 juce::TextEditor search,tags,tagFilter;
 juce::ComboBox model,source;
 juce::ToggleButton favourite{"Favourite"},onlyFavourites{"Favourites only"};
 juce::TextButton addFolder{"Add folder"},addFile{"Add file"},refresh{"Rescan"},remove{"Remove source"},saveTags{"Save tags"},exportPatch{"Export .syx"},saveCurrent{"Save current"};
 juce::Label info;
 std::unique_ptr<juce::FileChooser> chooser;
 std::vector<pluginLib::patchDB::PatchPtr> rows,searchRows;
 std::vector<pluginLib::patchDB::DataSourceNodePtr> sources;
 pluginLib::patchDB::DataSourceNodePtr preferredSource;
 uint64_t bankRequest=0;
 unsigned revision=~0u;
 pluginLib::patchDB::SearchHandle handle=pluginLib::patchDB::g_invalidSearchHandle;
 bool dirty=true,lastK=false,updatingRows=false,resultsDirty=true;
 int sortColumn=1;bool sortAscending=true;
 pluginLib::patchDB::PatchPtr lastActivatedPatch;
 juce::int64 lastActivationTime=0;
 juce::String cellText(const pluginLib::patchDB::PatchPtr&,int)const;
 void updateRows(std::vector<pluginLib::patchDB::PatchPtr>);
 void updateBanks(std::vector<pluginLib::patchDB::DataSourceNodePtr>&&);
 void sortOrderChanged(int,bool)override;
 void query();void timerCallback()override;void choose(bool folder,bool save=false,bool current=false);void activate(int);
 juce::String getCellTooltip(int row,int column)override{return row>=0 && row<int(rows.size())?cellText(rows[row],column):juce::String{};}
 int getNumRows()override{return int(rows.size());}
 void paintRowBackground(juce::Graphics&,int,int,int,bool)override;
 void paintCell(juce::Graphics&,int,int,int,int,bool)override;
 void selectedRowsChanged(int)override;
 void cellClicked(int,int,const juce::MouseEvent&)override;
 void returnKeyPressed(int row)override{activate(row);}
};
}
