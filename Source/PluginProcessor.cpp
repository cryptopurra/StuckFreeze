#include "PluginProcessor.h"
#include "PluginEditor.h"

StuckFreezeAudioProcessor::StuckFreezeAudioProcessor()
     : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

StuckFreezeAudioProcessor::~StuckFreezeAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout StuckFreezeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "FREEZE", "Freeze", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "SIZE", "Loop Size", 
        juce::NormalisableRange<float>(0.01f, 1.0f, 0.001f, 0.4f), 0.5f));

    return { params.begin(), params.end() };
}

const juce::String StuckFreezeAudioProcessor::getName() const { return JucePlugin_Name; }
bool StuckFreezeAudioProcessor::acceptsMidi() const { return false; }
bool StuckFreezeAudioProcessor::producesMidi() const { return false; }
bool StuckFreezeAudioProcessor::isMidiEffect() const { return false; }
double StuckFreezeAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int StuckFreezeAudioProcessor::getNumPrograms() { return 1; }
int StuckFreezeAudioProcessor::getCurrentProgram() { return 0; }
void StuckFreezeAudioProcessor::setCurrentProgram (int) {}
const juce::String StuckFreezeAudioProcessor::getProgramName (int) { return {}; }
void StuckFreezeAudioProcessor::changeProgramName (int, const juce::String&) {}

void StuckFreezeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    bufferSize = static_cast<int>(sampleRate * 4.0);
    circularBufferLeft.resize(bufferSize, 0.0f);
    circularBufferRight.resize(bufferSize, 0.0f);
    writeHead = 0;
}

void StuckFreezeAudioProcessor::releaseResources() {}

bool StuckFreezeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainOutputChannelSet() == layouts.getInputChannelSet();
}

void StuckFreezeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto* freezeParam = parameters.getRawParameterValue("FREEZE");
    auto* sizeParam   = parameters.getRawParameterValue("SIZE");

    bool freezeState = freezeParam != nullptr && *freezeParam > 0.5f;
    float sizeVal = sizeParam != nullptr ? *sizeParam : 0.5f;

    int minLen = 512;
    int maxLen = static_cast<int>(getSampleRate() * 2.0);
    int targetLoopLength = static_cast<int>(minLen + sizeVal * (maxLen - minLen));

    auto* channelDataL = buffer.getWritePointer (0);
    auto* channelDataR = totalNumInputChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float inL = channelDataL[i];
        float inR = channelDataR != nullptr ? channelDataR[i] : inL;

        circularBufferLeft[writeHead] = inL;
        circularBufferRight[writeHead] = inR;

        if (freezeState && !isFrozen)
        {
            isFrozen = true;
            freezeStartHead = writeHead;
            currentLoopLength = targetLoopLength;
        }
        else if (!freezeState)
        {
            isFrozen = false;
        }

        if (isFrozen)
        {
            currentLoopLength = targetLoopLength;
            int readHead = (freezeStartHead + (i % currentLoopLength)) % bufferSize;
            channelDataL[i] = circularBufferLeft[readHead];
            if (channelDataR != nullptr)
                channelDataR[i] = circularBufferRight[readHead];
        }

        writeHead = (writeHead + 1) % bufferSize;
    }
}

bool StuckFreezeAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* StuckFreezeAudioProcessor::createEditor() { return new StuckFreezeAudioProcessorEditor (*this); }

void StuckFreezeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void StuckFreezeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}
