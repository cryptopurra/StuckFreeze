#include "PluginProcessor.h"
#include "PluginEditor.h"

StuckFreezeAudioProcessorEditor::StuckFreezeAudioProcessorEditor (StuckFreezeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    stutterKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    stutterKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (stutterKnob);
    stutterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "STUTTER_KNOB", stutterKnob);

    freezeButton.setButtonText ("FREEZE");
    addAndMakeVisible (freezeButton);
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "FREEZE_ON", freezeButton);

    auto setupKnob = [this](juce::Slider& knob, const juce::String& paramID, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach)
    {
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        addAndMakeVisible (knob);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.parameters, paramID, knob);
    };

    setupKnob (freeTimeKnob,  "FREE_TIME",    freeTimeAttach);
    setupKnob (smoothKnob,    "SMOOTH",       smoothAttach);
    setupKnob (pitchKnob,     "PITCH",        pitchAttach);
    setupKnob (lowCutKnob,    "LOW_CUT",      lowCutAttach);
    setupKnob (keyOctKnob,    "KEY_OCT",      keyOctAttach);

    setupKnob (smearKnob,       "SMEAR",        smearAttach);
    setupKnob (freezePitchKnob, "FREEZE_PITCH", freezePitchAttach);
    setupKnob (reducerKnob,     "REDUCER",      reducerAttach);
    setupKnob (freezeMixKnob,   "FREEZE_MIX",   freezeMixAttach);
    setupKnob (mixKnob,         "MIX",          mixAttach);
    setupKnob (outputKnob,      "OUTPUT",       outputAttach);

    setSize (950, 580);
}

StuckFreezeAudioProcessorEditor::~StuckFreezeAudioProcessorEditor() {}

void StuckFreezeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff12131a));

    g.setFont (juce::FontOptions (32.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText ("STUCK", 40, 20, 110, 40, juce::Justification::left);
    g.setColour (juce::Colour (0xff8a63ff));
    g.drawText ("FREEZE", 150, 20, 150, 40, juce::Justification::left);

    juce::Rectangle<int> displayArea (380, 110, 190, 120);
    g.setColour (juce::Colour (0xff1c1d27));
    g.fillRoundedRectangle (displayArea.toFloat(), 10.0f);
    g.setColour (juce::Colour (0xffff5252));
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText ("500 MS", displayArea.removeFromTop(70), juce::Justification::centred);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText ("SYNC 1/4", displayArea, juce::Justification::centred);

    g.setColour (juce::Colour (0xff181924));
    juce::Rectangle<int> stutterBox (40, 310, 490, 230);
    juce::Rectangle<int> freezeBox  (560, 310, 350, 230);
    g.fillRoundedRectangle (stutterBox.toFloat(), 12.0f);
    g.fillRoundedRectangle (freezeBox.toFloat(),  12.0f);

    g.setColour (juce::Colour (0xffa08aff));
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("STUTTER", 55, 320, 150, 20, juce::Justification::left);
    g.drawText ("FREEZE / MASTER", 575, 320, 200, 20, juce::Justification::left);
}

void StuckFreezeAudioProcessorEditor::resized()
{
    stutterKnob.setBounds (140, 90, 200, 200);
    freezeButton.setBounds (610, 90, 190, 190);

    int startX = 50;
    int startY = 410;
    int spacingX = 95;

    freeTimeKnob.setBounds (startX, startY, 80, 110);
    smoothKnob.setBounds   (startX + spacingX, startY, 80, 110);
    pitchKnob.setBounds    (startX + spacingX * 2, startY, 80, 110);
    lowCutKnob.setBounds   (startX + spacingX * 3, startY, 80, 110);
    keyOctKnob.setBounds   (startX + spacingX * 4, startY, 80, 110);

    int freezeX = 575;
    smearKnob.setBounds       (freezeX, 350, 75, 80);
    freezePitchKnob.setBounds (freezeX + 80, 350, 75, 80);
    reducerKnob.setBounds     (freezeX + 160, 350, 75, 80);
    freezeMixKnob.setBounds   (freezeX + 240, 350, 75, 80);

    mixKnob.setBounds    (freezeX + 40, 440, 75, 80);
    outputKnob.setBounds (freezeX + 120, 440, 75, 80);
}
