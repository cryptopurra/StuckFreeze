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

// Key: porta la frequenza di ripetizione sulla nota piu' vicina della scala minore naturale
static double snapToKey (double f, int rootPc)
{
    static const bool inScale[12] = { true, false, true, true, false, true, false, true, true, false, true, false };
    const double m = 69.0 + 12.0 * std::log2 (f / 440.0);
    const int base = (int) std::round (m);
    double best = m, bestDist = 1.0e9;
    for (int off = -3; off <= 3; ++off)
    {
        const int cand = base + off;
        const int rel = ((cand - rootPc) % 12 + 12) % 12;
        if (! inScale[rel]) continue;
        const double dist = std::abs (m - (double) cand);
        if (dist < bestDist) { bestDist = dist; best = (double) cand; }
    }
    return 440.0 * std::pow (2.0, (best - 69.0) / 12.0);
}

static std::vector<juce::Point<float>> defaultEnv (int idx)
{
    if (idx == 0) return { { 0.0f, 0.5f }, { 1.0f, 0.5f } };   // Speed: 0 %
    if (idx == 1) return { { 0.0f, 0.5f }, { 1.0f, 1.0f } };   // Pitch: sale fino a +12 st
    return { { 0.0f, 0.0f }, { 1.0f, 1.0f } };                 // Smear: da 0 a 100 %
}

static juce::String envToString (const std::vector<juce::Point<float>>& pts)
{
    juce::String s;
    for (auto& p : pts) s << juce::String (p.x, 4) << "," << juce::String (p.y, 4) << ";";
    return s;
}

static std::vector<juce::Point<float>> envFromString (const juce::String& text)
{
    std::vector<juce::Point<float>> pts;
    juce::StringArray tokens;
    tokens.addTokens (text, ";", "");
    for (auto& t : tokens)
    {
        juce::StringArray xy;
        xy.addTokens (t, ",", "");
        if (xy.size() == 2) pts.push_back ({ xy[0].getFloatValue(), xy[1].getFloatValue() });
    }
    return pts;
}

void StuckFreezeProcessor::setEnvPoints (int idx, std::vector<juce::Point<float>> pts)
{
    if (pts.size() < 2) pts = defaultEnv (idx);
    std::sort (pts.begin(), pts.end(), [] (const juce::Point<float>& a, const juce::Point<float>& b) { return a.x < b.x; });
    pts.front().x = 0.0f;
    pts.back().x = 1.0f;
    for (auto& p : pts) p.y = juce::jlimit (0.0f, 1.0f, p.y);
    envPoints[idx] = pts;

    for (int i = 0; i < envRes; ++i)
    {
        const float x = (float) i / (float) (envRes - 1);
        float y = pts.back().y;
        for (size_t j = 0; j + 1 < pts.size(); ++j)
        {
            if (x >= pts[j].x && x <= pts[j + 1].x)
            {
                const float span = pts[j + 1].x - pts[j].x;
                const float t = span > 1.0e-6f ? (x - pts[j].x) / span : 0.0f;
                y = pts[j].y + (pts[j + 1].y - pts[j].y) * t;
                break;
            }
        }
        envTable[(size_t) (idx * envRes + i)].store (y);
    }
}

float StuckFreezeProcessor::envValue (int idx, float phase) const
{
    const float x = juce::jlimit (0.0f, 1.0f, phase) * (float) (envRes - 1);
    const int i0 = (int) x;
    const int i1 = std::min (i0 + 1, envRes - 1);
    const float fr = x - (float) i0;
    const float a = envTable[(size_t) (idx * envRes + i0)].load();
    const float b = envTable[(size_t) (idx * envRes + i1)].load();
    return a + (b - a) * fr;
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

    // Stutter (stile Stucker)
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "stuck", 1 }, "Stuck",
           juce::NormalisableRange<float> (0.0f, 1.0f, 0.0f), 0.0f));
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "stSync", 1 }, "Stutter Sync", true));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "stLen", 1 }, "Grid",
           juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16" }, 2));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "stFreeMs", 1 }, "Free Time",
           juce::NormalisableRange<float> (20.0f, 2000.0f, 1.0f, 0.5f), 250.0f, unitAttr ("ms", 0)));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "stRange", 1 }, "Range",
           juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 8.0f, unitAttr ("oct", 1)));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "stSmooth", 1 }, "Smooth",
           juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f, percentAttr()));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "stLowCut", 1 }, "Low Cut",
           juce::NormalisableRange<float> (20.0f, 2000.0f, 1.0f, 0.4f), 20.0f, unitAttr ("Hz", 0)));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "stKey", 1 }, "Key",
           juce::StringArray { "Off", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "stDrop", 1 }, "On The Drop",
           juce::StringArray { "Cut", "Tail", "Reverse" }, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "stCurve", 1 }, "Curve",
           juce::StringArray { "Lin", "Exp", "S" }, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "stRide", 1 }, "Ride",
           juce::StringArray { "Off", "1 bar", "2 bars", "4 bars", "8 bars" }, 0));
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "stGo", 1 }, "Go", false));

    // Freeze
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "frSpeed", 1 }, "Freeze Speed",
           juce::NormalisableRange<float> (-200.0f, 200.0f, 1.0f), 0.0f, unitAttr ("%", 0)));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "frGrain", 1 }, "Freeze Grain Size",
           juce::NormalisableRange<float> (256.0f, 8192.0f, 1.0f, 0.5f), 2048.0f, unitAttr ("smp", 0)));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "frEngine", 1 }, "Grain / FFT",
           juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, percentAttr()));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "frLen", 1 }, "Freeze Length",
           juce::StringArray { "1/4 bar", "1/2 bar", "1 bar", "2 bars", "4 bars", "8 bars" }, 2));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "frPlay", 1 }, "Freeze Playback",
           juce::StringArray { "Normal", "Loop", "Ping-Pong" }, 1));
    l.add (std::make_unique<juce::AudioParameterChoice> (PID { "envTarget", 1 }, "Envelope Target",
           juce::StringArray { "Off", "Speed", "Pitch", "Smear" }, 0));
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "frOn", 1 }, "Freeze", false));
    l.add (std::make_unique<juce::AudioParameterBool> (PID { "frLive", 1 }, "Live FX", true));
    l.add (std::make_unique<juce::AudioParameterFloat> (PID { "frSmear", 1 }, "Freeze Smear",
           juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, percentAttr()));
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
    for (auto& v : envTable) v.store (0.5f);
    for (int i = 0; i < 3; ++i) setEnvPoints (i, defaultEnv (i));
}

void StuckFreezeProcessor::prepareToPlay (double sampleRate, int)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        stutter[ch].prepare (sampleRate);
        freeze[ch].prepare (sampleRate);
    }
    rideActive = false; rideDone = false; prevGo = false; prevFreeze = false; envPos = 0.0;
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

    const bool  stSync    = get ("stSync") > 0.5f;
    const int   stLenIdx  = juce::jlimit (0, 4, (int) get ("stLen"));
    const float stFreeMs  = get ("stFreeMs");
    const float stRange   = get ("stRange");
    const float stSmooth  = get ("stSmooth");
    const float stLowCut  = get ("stLowCut");
    const int   keyIdx    = (int) get ("stKey");
    const int   dropMode  = juce::jlimit (0, 2, (int) get ("stDrop"));
    const int   curve     = juce::jlimit (0, 2, (int) get ("stCurve"));

    const bool  frOn      = get ("frOn") > 0.5f;
    const float frSmear   = get ("frSmear");
    const float frPitch   = get ("frPitch");
    const float frBits    = get ("frReducer");
    const float frMix     = get ("frMix");

    // ---- Freeze: velocita', motore grani/FFT, inviluppo ----
    static const double lenBeatsTable[6] = { 1.0, 2.0, 4.0, 8.0, 16.0, 32.0 };
    const int frLenIdx  = juce::jlimit (0, 5, (int) get ("frLen"));
    const int frPlay    = juce::jlimit (0, 2, (int) get ("frPlay"));
    const int envTarget = juce::jlimit (0, 3, (int) get ("envTarget"));
    const double envPeriod = lenBeatsTable[frLenIdx] * 60.0 / bpm * sr;

    // l'inviluppo scorre sempre (anche con Freeze spento: effetto live); in modo "Once" riparte a ogni Freeze
    if (frOn && ! prevFreeze && frPlay == 0) envPos = 0.0;
    prevFreeze = frOn;

    float envPh = -1.0f;
    float speedPct = get ("frSpeed"), pitchSt = frPitch, smearAmt = frSmear;
    {
        const double t = envPos / envPeriod;
        double ph;
        if (frPlay == 0)      ph = std::min (1.0, t);
        else if (frPlay == 1) ph = t - std::floor (t);
        else { const double m = std::fmod (t, 2.0); ph = m < 1.0 ? m : 2.0 - m; }

        if (envTarget > 0)
        {
            const float v = envValue (envTarget - 1, (float) ph);
            if (envTarget == 1)      speedPct = (v * 2.0f - 1.0f) * 200.0f;
            else if (envTarget == 2) pitchSt  = (v * 2.0f - 1.0f) * 12.0f;
            else                     smearAmt = v;
            envPh = (float) ph;
        }
        envPos += (double) n;
    }
    envPhase.store (envPh);

    FreezeParams fp;
    fp.freeze = frOn;
    fp.live = get ("frLive") > 0.5f;
    fp.smear = smearAmt;
    fp.pitchSt = pitchSt;
    fp.bits = frBits;
    fp.mix = frMix;
    fp.speed = speedPct / 100.0f;
    fp.grain = get ("frGrain");
    fp.engine = get ("frEngine");
    fp.windowLen = (int) juce::jmin (envPeriod, sr * (double) FreezeEngine::bufSeconds - 1.0);
    fp.playMode = frPlay;

    // ---- Ride: la manopola sale da sola in 1/2/4/8 battute, poi scatta il "drop" ----
    static const int rideBarsTable[5] = { 0, 1, 2, 4, 8 };
    const int rideIdx = juce::jlimit (0, 4, (int) get ("stRide"));
    const bool go = get ("stGo") > 0.5f;
    if (go && ! prevGo && rideIdx > 0)
    {
        rideElapsed = 0.0;
        rideTotal = (double) rideBarsTable[rideIdx] * 4.0 * 60.0 / bpm * sr;
        rideActive = true;
        rideDone = false;
    }
    if (! go && prevGo) { rideActive = false; rideDone = false; }
    prevGo = go;

    float rideProg = 0.0f;
    if (rideActive.load())
    {
        rideElapsed += (double) n;
        if (rideElapsed >= rideTotal) { rideActive = false; rideDone = true; }
        else rideProg = (float) (rideElapsed / rideTotal);
    }
    rideProgress.store (rideProg);

    // ---- manopola STUCK -> lunghezza della fetta ----
    const float k = std::max (get ("stuck"), rideProg);
    const bool active = k > 0.002f;

    static const double beats[5] = { 4.0, 2.0, 1.0, 0.5, 0.25 };
    const double gridBeats = beats[stLenIdx];
    const double gridSamples = stSync ? gridBeats * 60.0 / bpm * sr
                                      : (double) stFreeMs * 0.001 * sr;
    const int gridLen = (int) gridSamples;

    const float c = (curve == 0) ? k : (curve == 1 ? k * k : k * k * (3.0f - 2.0f * k));
    const double halvings = (double) stRange * (double) c;
    double sliceTarget = gridSamples * std::pow (2.0, -halvings);
    sliceTarget = juce::jmax (32.0, sliceTarget);

    if (active && keyIdx > 0)
    {
        const double f = sr / sliceTarget;
        if (f >= 20.0)
            sliceTarget = juce::jmax (32.0, sr / snapToKey (f, keyIdx - 1));
    }

    loopMs.store (active ? (float) (sliceTarget / sr * 1000.0) : 0.0f);
    sliceDenom.store ((active && stSync) ? (float) (4.0 / gridBeats * std::pow (2.0, halvings)) : 0.0f);

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
                const float s = st.process (in, active, gridLen, (float) sliceTarget, stSmooth, stLowCut, dropMode);
                wet = fr.process (s, fp);
            }
            else if (mode == 1)  // Freeze > Stutter
            {
                const float f = fr.process (in, fp);
                wet = st.process (f, active, gridLen, (float) sliceTarget, stSmooth, stLowCut, dropMode);
            }
            else                 // Parallelo
            {
                const float a = st.process (in, active, gridLen, (float) sliceTarget, stSmooth, stLowCut, dropMode);
                const float b = fr.process (in, fp);
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
    for (int i = 0; i < 3; ++i)
        apvts.state.setProperty ("env" + juce::String (i), envToString (envPoints[i]), nullptr);

    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void StuckFreezeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            for (int i = 0; i < 3; ++i)
            {
                const juce::String key = "env" + juce::String (i);
                if (apvts.state.hasProperty (key))
                    setEnvPoints (i, envFromString (apvts.state[key].toString()));
            }
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StuckFreezeProcessor();
}
