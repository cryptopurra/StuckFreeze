#include "PluginProcessor.h"
#include "PluginEditor.h"

StuckFreezeAudioProcessorEditor::StuckFreezeAudioProcessorEditor (StuckFreezeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    sizeKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    sizeKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible (sizeKnob);
    sizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "SIZE", sizeKnob);

    freezeButton.setButtonText ("FREEZE");
    addAndMakeVisible (freezeButton);
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "FREEZE", freezeButton);

    setSize (300, 350);
}

StuckFreezeAudioProcessorEditor::~StuckFreezeAudioProcessorEditor() {}

void StuckFreezeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawFittedText ("STUCK FREEZE", getLocalBounds().removeFromTop(50), juce::Justification::centred, 1);
}

void StuckFreezeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(40);

    sizeKnob.setBounds (area.removeFromTop(180).reduced(20));
    freezeButton.setBounds (area.removeFromTop(50).reduced(40, 5));
}

void StuckFreezeAudioProcessorEditor::buttonClicked (juce::Button*) {}
