#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
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
    std::atomic<float> inLevel { 0.0f }, outLevel { 0.0f }, loopMs { 0.0f },
                       sliceDenom { 0.0f }, rideProgress { 0.0f };
    std::atomic<bool>  rideActive { false }, rideDone { false };

    // ---- inviluppi disegnabili del Freeze (0 = Speed, 1 = Pitch, 2 = Smear) ----
    static constexpr int envRes = 128;
    std::array<std::atomic<float>, 3 * envRes> envTable;
    std::vector<juce::Point<float>> envPoints[3];     // usati solo dall'interfaccia
    std::atomic<float> envPhase { -1.0f };
    void setEnvPoints (int idx, std::vector<juce::Point<float>> pts);
    float envValue (int idx, float phase) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    bool prevGo = false, prevFreeze = false;
    double envPos = 0.0;
    double rideElapsed = 0.0, rideTotal = 1.0;

    StutterEngine stutter[2];
    FreezeEngine  freeze[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StuckFreezeProcessor)
};
