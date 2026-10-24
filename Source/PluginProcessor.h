#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class StuckFreezeAudioProcessor : public juce::AudioProcessor
{
public:
    StuckFreezeAudioProcessor();
    ~StuckFreezeAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Parametri
    std::atomic<float>* stutterKnobParam = nullptr;
    std::atomic<float>* freezeButtonParam = nullptr;

    std::atomic<float>* freeTimeParam = nullptr;
    std::atomic<float>* smoothParam   = nullptr;
    std::atomic<float>* pitchParam    = nullptr;
    std::atomic<float>* lowCutParam   = nullptr;
    std::atomic<float>* keyOctParam   = nullptr;

    std::atomic<float>* smearParam     = nullptr;
    std::atomic<float>* freezePitchP   = nullptr;
    std::atomic<float>* reducerParam   = nullptr;
    std::atomic<float>* freezeMixParam = nullptr;
    std::atomic<float>* masterMixParam = nullptr;
    std::atomic<float>* outputParam    = nullptr;

    // Buffer di Freeze / Loop
    std::vector<float> circularBufferLeft;
    std::vector<float> circularBufferRight;
    int writeHead = 0;
    int bufferSize = 0;

    bool isFrozen = false;
    int freezeStartHead = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StuckFreezeAudioProcessor)
};
