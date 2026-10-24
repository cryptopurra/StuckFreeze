#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class StuckFreezeAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    StuckFreezeAudioProcessorEditor (StuckFreezeAudioProcessor&);
    ~StuckFreezeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    StuckFreezeAudioProcessor& audioProcessor;

    juce::Slider stutterKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stutterAttachment;

    juce::ToggleButton freezeButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;

    juce::Slider freeTimeKnob, smoothKnob, pitchKnob, lowCutKnob, keyOctKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freeTimeAttach, smoothAttach, pitchAttach, lowCutAttach, keyOctAttach;

    juce::Slider smearKnob, freezePitchKnob, reducerKnob, freezeMixKnob, mixKnob, outputKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> smearAttach, freezePitchAttach, reducerAttach, freezeMixAttach, mixAttach, outputAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StuckFreezeAudioProcessorEditor)
};
