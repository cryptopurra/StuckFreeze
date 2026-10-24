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

    std::atomic<float>* freezeParam = nullptr;
    std::atomic<float>* sizeParam = nullptr;

    std::vector<float> circularBufferLeft;
    std::vector<float> circularBufferRight;
    int writeHead = 0;
    int bufferSize = 0;
    
    bool isFrozen = false;
    int freezeStartHead = 0;
    int currentLoopLength = 44100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StuckFreezeAudioProcessor)
};
