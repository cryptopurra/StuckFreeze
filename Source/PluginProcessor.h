#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "StutterEngine.h"
#include "FreezeEngine.h"

class StuckFreezeProcessor : public juce::AudioProcessor
{
public:
    StuckFreezeProcessor();
    ~StuckFreezeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "StuckFreeze"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // letti dall'interfaccia (meter e display)
    std::atomic<float> inLevel { 0.0f }, outLevel { 0.0f }, loopMs { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    StutterEngine stutter[2];
    FreezeEngine  freeze[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StuckFreezeProcessor)
};
