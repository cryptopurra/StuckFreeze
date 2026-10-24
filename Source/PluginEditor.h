#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class StuckFreezeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Button::Listener
{
public:
    StuckFreezeAudioProcessorEditor (StuckFreezeAudioProcessor&);
    ~StuckFreezeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    StuckFreezeAudioProcessor& audioProcessor;

    juce::Slider sizeKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttachment;

    juce::ToggleButton freezeButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;

    void buttonClicked (juce::Button* b) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StuckFreezeAudioProcessorEditor)
};
