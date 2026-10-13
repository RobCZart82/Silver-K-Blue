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
private:
    struct Surface;
    std::unique_ptr<Surface> m_surface;
};
}
