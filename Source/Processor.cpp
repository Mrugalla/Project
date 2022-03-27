#include "Processor.h"

#include "Editor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new audio::Processor();
}

juce::AudioProcessorEditor* audio::Processor::createEditor()
{
    return new gui::Editor(*this);
}