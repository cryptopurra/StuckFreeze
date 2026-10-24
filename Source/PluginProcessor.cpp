#include "PluginProcessor.h"
#include "PluginEditor.h"

using PID = juce::ParameterID;

static juce::AudioParameterFloatAttributes unitAttr (const juce::String& unit, int decimals)
{
    return juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([unit, decimals] (float v, int) { return juce::String (v, decimals) + " " + unit; });
}

static juce::AudioParameterFloatAttributes percentAttr()
{
    return juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });
}

juce::AudioProcessorValueTreeState::ParameterLayout StuckFreezeProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    // Master
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "mode", 1 }, "Routing",
           juce::StringArray { "Stutter > Freeze", "Freeze > Stutter", "Parallel" }, 0));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "mix", 1 }, "Mix",
           juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, percentAttr()));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "outGain", 1 }, "Output",
           juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, unitAttr ("dB", 1)));

    // Stutter
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "stOn", 1 }, "Stutter On", false));
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "stSync", 1 }, "Stutter Sync", true));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "stLen", 1 }, "Stutter Length",
           juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16" }, 2));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "stKey", 1 }, "Stutter Key",
           juce::StringArray { "Off", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "stKeyOct", 1 }, "Stutter Key Octave",
           juce::NormalisableRange<float> (1.0f, 5.0f, 1.0f), 3.0f, unitAttr ("oct", 0)));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "stFreeMs", 1 }, "Stutter Free Time",
           juce::NormalisableRange<float> (20.0f, 2000.0f, 1.0f, 0.5f), 250.0f, unitAttr ("ms", 0)));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "stSmooth", 1 }, "Stutter Smooth",
           juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f, percentAttr()));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "stPitch", 1 }, "Stutter Pitch",
           juce::NormalisableRange<float> (-12.0f, 12.0f, 1.0f), 0.0f, unitAttr ("st", 0)));
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "stReverse", 1 }, "Stutter Reverse", false));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "stLowCut", 1 }, "Stutter Low Cut",
           juce::NormalisableRange<float> (20.0f, 2000.0f, 1.0f, 0.4f), 20.0f, unitAttr ("Hz", 0)));

    // Freeze
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "frOn", 1 }, "Freeze", false));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "frSmear", 1 }, "Freeze Smear",
           juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f, percentAttr()));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "frPitch", 1 }, "Freeze Pitch",
           juce::NormalisableRange<float> (-12.0f, 12.0f, 1.0f), 0.0f, unitAttr ("st", 0)));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "frReducer", 1 }, "Freeze Reducer",
           juce::NormalisableRange<float> (4.0f, 16.0f, 1.0f), 16.0f, unitAttr ("bit", 0)));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "frMix", 1 }, "Freeze Mix",
           juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, percentAttr()));

    return l;
}

StuckFreezeProcessor::StuckFreezeProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

void StuckFreezeProcessor::prepareToPlay (double sampleRate, int)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        stutter[ch].prepare (sampleRate);
        freeze[ch].prepare (sampleRate);
    }
}

bool StuckFreezeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void StuckFreezeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    // tempo dell'host
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                if (*b > 20.0) bpm = *b;

    const double sr = getSampleRate();

    float inPk = 0.0f;
    for (int ch = 0; ch < numCh; ++ch) inPk = juce::jmax (inPk, buffer.getMagnitude (ch, 0, n));
    inLevel.store (juce::jmax (inLevel.load(), inPk));

    const int   mode      = (int) get ("mode");
    const float mix       = get ("mix");
    const float outGain   = juce::Decibels::decibelsToGain (get ("outGain"));

    const bool  stOn      = get ("stOn") > 0.5f;
    const bool  stSync    = get ("stSync") > 0.5f;
    const int   stLenIdx  = juce::jlimit (0, 4, (int) get ("stLen"));
    const float stFreeMs  = get ("stFreeMs");
    const float stSmooth  = get ("stSmooth");
    const float stPitch   = get ("stPitch");
    const bool  stReverse = get ("stReverse") > 0.5f;
    const float stLowCut  = get ("stLowCut");

    const bool  frOn      = get ("frOn") > 0.5f;
    const float frSmear   = get ("frSmear");
    const float frPitch   = get ("frPitch");
    const float frBits    = get ("frReducer");
    const float frMix     = get ("frMix");

    static const double beats[5] = { 4.0, 2.0, 1.0, 0.5, 0.25 };
    // Key: la lunghezza del loop diventa il periodo della nota scelta -> il loop suona intonato
    const int keyIdx = (int) get ("stKey");
    int lenSamples;
    if (keyIdx > 0)
    {
        const int midi = 12 * ((int) get ("stKeyOct") + 1) + (keyIdx - 1);
        const double freq = 440.0 * std::pow (2.0, (midi - 69) / 12.0);
        lenSamples = (int) (sr / freq);
    }
    else
    {
        lenSamples = stSync ? (int) (beats[stLenIdx] * 60.0 / bpm * sr)
                            : (int) ((double) stFreeMs * 0.001 * sr);
    }
    loopMs.store ((float) ((double) lenSamples / sr * 1000.0));

    for (int ch = 0; ch < numCh; ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        auto& st = stutter[ch];
        auto& fr = freeze[ch];

        for (int i = 0; i < n; ++i)
        {
            const float in = d[i];
            float wet;

            if (mode == 0)       // Stutter > Freeze
            {
                const float s = st.process (in, stOn, lenSamples, stSmooth, stPitch, stReverse, stLowCut);
                wet = fr.process (s, frOn, frSmear, frPitch, frBits, frMix);
            }
            else if (mode == 1)  // Freeze > Stutter
            {
                const float f = fr.process (in, frOn, frSmear, frPitch, frBits, frMix);
                wet = st.process (f, stOn, lenSamples, stSmooth, stPitch, stReverse, stLowCut);
            }
            else                 // Parallelo
            {
                const float a = st.process (in, stOn, lenSamples, stSmooth, stPitch, stReverse, stLowCut);
                const float b = fr.process (in, frOn, frSmear, frPitch, frBits, frMix);
                wet = a + b - in;
            }

            d[i] = (in * (1.0f - mix) + wet * mix) * outGain;
        }
    }

    float outPk = 0.0f;
    for (int ch = 0; ch < numCh; ++ch) outPk = juce::jmax (outPk, buffer.getMagnitude (ch, 0, n));
    outLevel.store (juce::jmax (outLevel.load(), outPk));
}

juce::AudioProcessorEditor* StuckFreezeProcessor::createEditor()
{
    return new StuckFreezeEditor (*this);
}

void StuckFreezeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void StuckFreezeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StuckFreezeProcessor();
}
