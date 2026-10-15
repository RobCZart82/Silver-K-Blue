#pragma once
#include "silverProcessor.h"
namespace silverK {
class Browser;
class PerformancePanel final:public juce::Component {
public:
 PerformancePanel(Processor&,Browser&);
 ~PerformancePanel()override;
 void resized()override;
 void refresh();
 void closeNoteEditor();
 std::function<void(unsigned)> onEdit;
 std::function<void()> onModeChanged;
private:
 struct Impl;
 std::unique_ptr<Impl> impl;
};
}
