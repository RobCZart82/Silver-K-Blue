#pragma once
#include "silverProcessor.h"
#include "jucePluginLib/patchdb/search.h"
namespace silverK {
class Browser final:public juce::Component,private juce::TableListBoxModel,private juce::Timer {
public:
 explicit Browser(Processor&);~Browser()override;
 void resized()override;
private:
 Processor& p;
 juce::TableListBox table;
 juce::TextEditor search,tags,tagFilter;
 juce::ComboBox model,source;
 juce::ToggleButton favourite{"Favourite"},onlyFavourites{"Favourites only"};
 juce::TextButton addFolder{"Add folder"},addFile{"Add file"},refresh{"Rescan"},remove{"Remove source"},load{"Load"},saveTags{"Save tags"},exportPatch{"Export .syx"},saveCurrent{"Save current"};
 juce::Label info;
 std::unique_ptr<juce::FileChooser> chooser;
 std::vector<pluginLib::patchDB::PatchPtr> rows;
 std::vector<pluginLib::patchDB::DataSourceNodePtr> sources;
 unsigned revision=~0u;
 pluginLib::patchDB::SearchHandle handle=pluginLib::patchDB::g_invalidSearchHandle;
 bool dirty=true,lastK=false;
 void query();void timerCallback()override;void choose(bool folder,bool save=false,bool current=false);void activate(int);
 int getNumRows()override{return int(rows.size());}
 void paintRowBackground(juce::Graphics&,int,int,int,bool)override;
 void paintCell(juce::Graphics&,int,int,int,int,bool)override;
 void selectedRowsChanged(int)override;
 void cellDoubleClicked(int row,int,const juce::MouseEvent&)override{activate(row);}
};
}
