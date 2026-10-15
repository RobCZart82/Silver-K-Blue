#pragma once
#include "silverProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace silverK
{
class Editor final : public juce::AudioProcessorEditor
{
public:
    explicit Editor(Processor&);
    ~Editor() override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void setUiScale(double);
    double getUiScale() const;
private:
    void showSizeMenu();
    void showCustomSize();
    Processor& m_processor;
    struct Surface;
    std::unique_ptr<Surface> m_surface;
    std::unique_ptr<juce::AlertWindow> m_sizeDialog;
    bool m_sizeMenuOpen=false;
};
}
