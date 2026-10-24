#include "PluginProcessor.h"
#include "PluginEditor.h"

StuckFreezeAudioProcessor::StuckFreezeAudioProcessor()
     : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    stutterKnobParam  = parameters.getRawParameterValue ("STUTTER_KNOB");
    freezeButtonParam = parameters.getRawParameterValue ("FREEZE_ON");

    freeTimeParam = parameters.getRawParameterValue ("FREE_TIME");
    smoothParam   = parameters.getRawParameterValue ("SMOOTH");
    pitchParam    = parameters.getRawParameterValue ("PITCH");
    lowCutParam   = parameters.getRawParameterValue ("LOW_CUT");
    keyOctParam   = parameters.getRawParameterValue ("KEY_OCT");

    smearParam     = parameters.getRawParameterValue ("SMEAR");
    freezePitchP   = parameters.getRawParameterValue ("FREEZE_PITCH");
    reducerParam   = parameters.getRawParameterValue ("REDUCER");
    freezeMixParam = parameters.getRawParameterValue ("FREEZE_MIX");
    masterMixParam = parameters.getRawParameterValue ("MIX");
    outputParam    = parameters.getRawParameterValue ("OUTPUT");
}

StuckFreezeAudioProcessor::~StuckFreezeAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout StuckFreezeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "STUTTER_KNOB", "Stutter Loop Size", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.35f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        "FREEZE_ON", "Freeze Trigger", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat>("FREE_TIME", "Free Time", 10.0f, 1000.0f, 250.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("SMOOTH", "Smooth", 0.0f, 100.0f, 35.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("PITCH", "Pitch", -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("LOW_CUT", "Low Cut", 20.0f, 2000.0f, 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("KEY_OCT", "Key Oct", 1.0f, 8.0f, 3.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>("SMEAR", "Smear", 0.0f, 100.0f, 30.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FREEZE_PITCH", "Freeze Pitch", -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("REDUCER", "Reducer", 1.0f, 16.0f, 16.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("FREEZE_MIX", "Freeze Mix", 0.0f, 100.0f, 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("MIX", "Mix", 0.0f, 100.0f, 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("OUTPUT", "Output Gain dB", -24.0f, 12.0f, 0.0f));

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
    circularBufferLeft.assign (bufferSize, 0.0f);
    circularBufferRight.assign (bufferSize, 0.0f);
    writeHead = 0;
}

void StuckFreezeAudioProcessor::releaseResources() {}

bool StuckFreezeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void StuckFreezeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float knobVal      = stutterKnobParam != nullptr ? stutterKnobParam->load() : 0.5f;
    bool  freezeActive = freezeButtonParam != nullptr && freezeButtonParam->load() > 0.5f;

    float smearVal   = smearParam != nullptr ? smearParam->load() / 100.0f : 0.3f;
    float frzPitch   = freezePitchP != nullptr ? freezePitchP->load() : 0.0f;
    float reducerBits= reducerParam != nullptr ? reducerParam->load() : 16.0f;
    float frzMix     = freezeMixParam != nullptr ? freezeMixParam->load() / 100.0f : 1.0f;
    float masterMix  = masterMixParam != nullptr ? masterMixParam->load() / 100.0f : 1.0f;
    float outGainDB  = outputParam != nullptr ? outputParam->load() : 0.0f;
    float outGain    = juce::Decibels::decibelsToGain (outGainDB);

    int minLoopLen = 128;
    int maxLoopLen = static_cast<int>(getSampleRate() * 1.5);
    int currentLoopLen = static_cast<int>(minLoopLen + knobVal * (maxLoopLen - minLoopLen));

    // Pitch ratio factor
    float pitchRatio = std::pow (2.0f, frzPitch / 12.0f);

    auto* channelL = buffer.getWritePointer (0);
    auto* channelR = totalNumInputChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float inL = channelL[i];
        float inR = channelR != nullptr ? channelR[i] : inL;

        circularBufferLeft[writeHead]  = inL;
        circularBufferRight[writeHead] = inR;

        if (freezeActive && !isFrozen)
        {
            isFrozen = true;
            freezeStartHead = writeHead;
        }
        else if (!freezeActive)
        {
            isFrozen = false;
        }

        float processedL = inL;
        float processedR = inR;

        if (isFrozen)
        {
            // Lettura del loop con pitch shift applicato
            float loopPos = std::fmod ((float)i * pitchRatio, (float)currentLoopLen);
            int readHead = (freezeStartHead + static_cast<int>(loopPos)) % bufferSize;
            if (readHead < 0) readHead += bufferSize;

            float frzL = circularBufferLeft[readHead];
            float frzR = circularBufferRight[readHead];

            // Effetto Smear (interpolazione/sfumatura granulare sul loop)
            if (smearVal > 0.0f)
            {
                int smearOffset = static_cast<int>(smearVal * 1000.0f) % bufferSize;
                int smearHead = (readHead + smearOffset) % bufferSize;
                frzL = frzL * (1.0f - smearVal * 0.5f) + circularBufferLeft[smearHead] * (smearVal * 0.5f);
                frzR = frzR * (1.0f - smearVal * 0.5f) + circularBufferRight[smearHead] * (smearVal * 0.5f);
            }

            // Effetto Reducer (Bit-crushing / Quantizzazione della risoluzione)
            if (reducerBits < 16.0f)
            {
                float steps = std::pow (2.0f, reducerBits);
                frzL = std::floor (frzL * steps) / steps;
                frzR = std::floor (frzR * steps) / steps;
            }

            // Freeze Mix
            processedL = juce::jmap (frzMix, inL, frzL);
            processedR = juce::jmap (frzMix, inR, frzR);
        }

        // Master Mix & Output Gain
        float finalL = juce::jmap (masterMix, inL, processedL) * outGain;
        float finalR = juce::jmap (masterMix, inR, processedR) * outGain;

        channelL[i] = finalL;
        if (channelR != nullptr)
            channelR[i] = finalR;

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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StuckFreezeAudioProcessor();
}
