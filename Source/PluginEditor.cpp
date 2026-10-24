#include "PluginEditor.h"

using namespace ui;

//==============================================================================
StuckLnF::StuckLnF()
{
    setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff2a2a38));
    setColour (juce::TextButton::buttonOnColourId, accent);
    setColour (juce::TextButton::textColourOffId,  dim);
    setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId,    accent);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

juce::Font StuckLnF::getLabelFont (juce::Label& l)
{
    return juce::Font (juce::FontOptions ((float) l.getHeight() * 0.72f));
}

void StuckLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                 float startAngle, float endAngle, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float d = std::min (b.getWidth(), b.getHeight());
    const float cx = b.getCentreX(), cy = b.getCentreY();
    const float outerR = d * 0.5f;
    const float arcR = outerR - d * 0.05f;
    const float th = std::max (2.0f, d * 0.07f);
    const float ang = startAngle + pos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (panelEdge);
    g.strokePath (track, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, ang, true);
    g.setColour (accent);
    g.strokePath (val, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float bodyR = outerR - d * 0.17f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3d4c), cx - bodyR * 0.5f, cy - bodyR * 0.7f,
                                             juce::Colour (0xff14141b), cx + bodyR * 0.6f, cy + bodyR * 0.8f, false));
    g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    const float dotR = std::max (2.0f, d * 0.04f);
    const float dr = bodyR * 0.68f;
    g.setColour (juce::Colours::white);
    g.fillEllipse (cx + dr * std::sin (ang) - dotR, cy - dr * std::cos (ang) - dotR, dotR * 2.0f, dotR * 2.0f);
}

//==============================================================================
RoundButton::RoundButton (Kind k)
    : juce::Button (k == Kind::stutter ? "stutter" : "freeze"), kind (k)
{
    setClickingTogglesState (true);
}

bool RoundButton::hitTest (int x, int y)
{
    const float r = (float) std::min (getWidth(), getHeight()) * 0.5f;
    return getLocalBounds().toFloat().getCentre().getDistanceFrom (juce::Point<float> ((float) x, (float) y)) <= r;
}

void RoundButton::paintButton (juce::Graphics& g, bool, bool down)
{
    auto b = getLocalBounds().toFloat().reduced (4.0f);
    const float d = std::min (b.getWidth(), b.getHeight());
    auto r = juce::Rectangle<float> (d, d).withCentre (b.getCentre());
    const bool on = getToggleState();

    if (kind == Kind::freeze)
    {
        // corpo metallico stile TimeFreeze
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f4f8), r.getX() + d * 0.3f, r.getY() + d * 0.1f,
                                                 juce::Colour (0xff6e6e7a), r.getRight(), r.getBottom(), false));
        g.fillEllipse (r);
        auto in = r.reduced (d * 0.07f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffdcdce2), in.getX(), in.getY(),
                                                 juce::Colour (0xff9a9aa6), in.getRight(), in.getBottom(), false));
        g.fillEllipse (in);
        if (on)
        {
            g.setColour (accent);
            g.drawEllipse (r.reduced (d * 0.01f), d * 0.035f);
        }
        g.setColour (on ? accentDim.darker (0.3f) : juce::Colour (0xff2a2a33));
        g.setFont (makeFont (d * 0.15f, true));
        g.drawText ("FREEZE", in, juce::Justification::centred, false);
    }
    else
    {
        // corpo scuro stile Stucker
        g.setColour (on ? accent : panelEdge);
        g.drawEllipse (r.reduced (d * 0.015f), d * 0.035f);
        auto in = r.reduced (d * 0.07f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34343f), in.getX(), in.getY(),
                                                 juce::Colour (0xff17171e), in.getRight(), in.getBottom(), false));
        g.fillEllipse (in);
        g.setColour (juce::Colours::white);
        g.setFont (makeFont (d * 0.21f, true));
        g.drawText (on ? "STUCK" : "OFF", in.withTrimmedBottom (d * 0.06f), juce::Justification::centred, false);
        g.setColour (dim);
        g.setFont (makeFont (d * 0.065f, true));
        g.drawText ("STUTTER", in.withTrimmedTop (d * 0.5f).withTrimmedBottom (d * 0.2f), juce::Justification::centred, false);
    }

    if (down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.15f));
        g.fillEllipse (r);
    }
}

//==============================================================================
ChoiceStrip::ChoiceStrip (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, juce::StringArray it)
    : items (std::move (it)),
      attachment (*apvts.getParameter (id), [this] (float v) { selected = (int) std::round (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

void ChoiceStrip::mouseDown (const juce::MouseEvent& e)
{
    const int n = items.size();
    if (n <= 0 || getWidth() <= 0) return;
    const int idx = juce::jlimit (0, n - 1, e.x * n / getWidth());
    attachment.setValueAsCompleteGesture ((float) idx);
}

void ChoiceStrip::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float rad = b.getHeight() * 0.3f;
    g.setColour (juce::Colour (0xff101016));
    g.fillRoundedRectangle (b, rad);

    const int n = items.size();
    if (n <= 0) return;
    const float w = b.getWidth() / (float) n;
    g.setFont (makeFont (b.getHeight() * 0.42f, true));
    for (int i = 0; i < n; ++i)
    {
        auto seg = juce::Rectangle<float> (b.getX() + w * (float) i, b.getY(), w, b.getHeight()).reduced (2.0f);
        if (i == selected)
        {
            g.setColour (accent);
            g.fillRoundedRectangle (seg, rad * 0.8f);
            g.setColour (juce::Colours::white);
        }
        else g.setColour (dim);
        g.drawText (items[i], seg, juce::Justification::centred, false);
    }
}

//==============================================================================
void LevelMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float barW = b.getWidth() * 0.30f;
    auto bar = juce::Rectangle<float> (right ? b.getX() + 2.0f : b.getRight() - barW - 2.0f,
                                       b.getY() + 6.0f, barW, b.getHeight() - 12.0f);

    g.setColour (juce::Colour (0xff0d0d12));
    g.fillRoundedRectangle (bar, 3.0f);

    auto dbToY = [&] (float db) { return bar.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 54.0f) * bar.getHeight(); };

    const float db = juce::Decibels::gainToDecibels (level, -60.0f);
    const float top = dbToY (db);
    if (top < bar.getBottom())
    {
        juce::ColourGradient grad (accentDim, 0.0f, bar.getBottom(), juce::Colour (0xffff5a7a), 0.0f, bar.getY(), false);
        grad.addColour (0.7, accent);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bar.getX(), top, bar.getWidth(), bar.getBottom() - top, 3.0f);
    }

    g.setColour (dim);
    g.setFont (makeFont (std::max (8.0f, b.getWidth() * 0.22f)));
    const float ticks[] = { 6, 0, -6, -12, -18, -24, -36, -48 };
    const float tw = b.getWidth() - barW - 6.0f;
    for (float t : ticks)
    {
        const float y = dbToY (t);
        auto r = juce::Rectangle<float> (right ? bar.getRight() + 3.0f : b.getX(), y - 6.0f, tw, 12.0f);
        g.drawText (t > 0 ? "+" + juce::String ((int) t) : juce::String ((int) t), r,
                    right ? juce::Justification::centredLeft : juce::Justification::centredRight, false);
    }
}

//==============================================================================
void LedDisplay::setText (const juce::String& big, const juce::String& small)
{
    if (big != bigText || small != smallText)
    {
        bigText = big; smallText = small;
        repaint();
    }
}

void LedDisplay::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff09090c));
    g.fillRoundedRectangle (b, b.getHeight() * 0.12f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (b.reduced (1.0f), b.getHeight() * 0.12f, 1.5f);

    auto inner = b.reduced (b.getWidth() * 0.04f, b.getHeight() * 0.08f);
    auto bigR = inner.removeFromTop (inner.getHeight() * 0.68f);

    g.setFont (makeFont (bigR.getHeight() * 0.85f, true));
    g.setColour (ledRed.withAlpha (0.22f));
    g.drawText (bigText, bigR.expanded (1.5f), juce::Justification::centred, false);
    g.setColour (ledRed);
    g.drawText (bigText, bigR, juce::Justification::centred, false);

    g.setFont (makeFont (inner.getHeight() * 0.7f, true));
    g.setColour (ledRed.withAlpha (0.6f));
    g.drawText (smallText, inner, juce::Justification::centred, false);
}

//==============================================================================
StuckFreezeEditor::StuckFreezeEditor (StuckFreezeProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      lenStrip   (p.apvts, "stLen",  { "1/1", "1/2", "1/4", "1/8", "1/16" }),
      keyStrip   (p.apvts, "stKey",  { "OFF", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }),
      routeStrip (p.apvts, "mode",   { "S>F", "F>S", "PAR" })
{
    setLookAndFeel (&lnf);

    stAtt   = std::make_unique<BA> (proc.apvts, "stOn",      stutterBtn);
    frAtt   = std::make_unique<BA> (proc.apvts, "frOn",      freezeBtn);
    syncAtt = std::make_unique<BA> (proc.apvts, "stSync",    syncBtn);
    revAtt  = std::make_unique<BA> (proc.apvts, "stReverse", reverseBtn);
    syncBtn.setClickingTogglesState (true);
    reverseBtn.setClickingTogglesState (true);

    setupKnob (kFree,    "stFreeMs",  "FREE TIME");
    setupKnob (kSmooth,  "stSmooth",  "SMOOTH");
    setupKnob (kPitch,   "stPitch",   "PITCH");
    setupKnob (kLowCut,  "stLowCut",  "LOW CUT");
    setupKnob (kOct,     "stKeyOct",  "KEY OCT");
    setupKnob (kSmear,   "frSmear",   "SMEAR");
    setupKnob (kFrPitch, "frPitch",   "PITCH");
    setupKnob (kReducer, "frReducer", "REDUCER");
    setupKnob (kFrMix,   "frMix",     "FREEZE MIX");
    setupKnob (kMix,     "mix",       "MIX");
    setupKnob (kOut,     "outGain",   "OUTPUT");

    for (juce::Component* c : { (juce::Component*) &inMeter, (juce::Component*) &outMeter,
                                (juce::Component*) &stutterBtn, (juce::Component*) &freezeBtn,
                                (juce::Component*) &syncBtn, (juce::Component*) &reverseBtn,
                                (juce::Component*) &lenStrip, (juce::Component*) &keyStrip,
                                (juce::Component*) &routeStrip, (juce::Component*) &led })
        addAndMakeVisible (*c);

    setResizable (true, true);
    setResizeLimits (700, 378, 2000, 1080);
    getConstrainer()->setFixedAspectRatio (1000.0 / 540.0);
    setSize (1000, 540);
    startTimerHz (30);
}

StuckFreezeEditor::~StuckFreezeEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void StuckFreezeEditor::setupKnob (Knob& k, const juce::String& id, const juce::String& label)
{
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                  juce::MathConstants<float>::pi * 2.75f, true);
    k.name.setText (label, juce::dontSendNotification);
    k.name.setJustificationType (juce::Justification::centred);
    k.name.setColour (juce::Label::textColourId, dim);
    k.name.setInterceptsMouseClicks (false, false);
    k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.name);
}

void StuckFreezeEditor::placeKnob (Knob& k, juce::Rectangle<int> cell, float s)
{
    k.name.setBounds (cell.removeFromTop (juce::roundToInt (16.0f * s)));
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                              juce::roundToInt ((float) cell.getWidth() * 0.95f), juce::roundToInt (18.0f * s));
    k.slider.setBounds (cell);
}

void StuckFreezeEditor::timerCallback()
{
    meterIn  = std::max (proc.inLevel.exchange (0.0f),  meterIn  * 0.88f);
    meterOut = std::max (proc.outLevel.exchange (0.0f), meterOut * 0.88f);
    inMeter.setLevel (meterIn);
    outMeter.setLevel (meterOut);

    static const char* notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* lens[]  = { "1/1", "1/2", "1/4", "1/8", "1/16" };

    const float ms = proc.loopMs.load();
    const int key  = (int) proc.apvts.getRawParameterValue ("stKey")->load();
    const int oct  = (int) proc.apvts.getRawParameterValue ("stKeyOct")->load();
    const int len  = juce::jlimit (0, 4, (int) proc.apvts.getRawParameterValue ("stLen")->load());
    const bool sync = proc.apvts.getRawParameterValue ("stSync")->load() > 0.5f;

    juce::String small = key > 0 ? "KEY " + juce::String (notes[key - 1]) + juce::String (oct)
                                 : (sync ? "SYNC " + juce::String (lens[len]) : "FREE");
    led.setText (juce::String (ms, ms < 100.0f ? 1 : 0) + " MS", small);

    syncBtn.setButtonText (sync ? "SYNC" : "FREE");
}

void StuckFreezeEditor::paint (juce::Graphics& g)
{
    const float s = (float) getWidth() / 1000.0f;
    g.fillAll (bg);

    // titolo
    auto font = makeFont (26.0f * s, true);
    juce::GlyphArrangement ga;
    ga.addLineOfText (font, "STUCK", 0.0f, 0.0f);
    const float w1 = ga.getBoundingBox (0, -1, true).getWidth();
    g.setFont (font);
    g.setColour (text);
    g.drawText ("STUCK", juce::Rectangle<float> (64.0f * s, 8.0f * s, 200.0f * s, 34.0f * s), juce::Justification::centredLeft, false);
    g.setColour (accent);
    g.drawText ("FREEZE", juce::Rectangle<float> (64.0f * s + w1 + 2.0f * s, 8.0f * s, 200.0f * s, 34.0f * s), juce::Justification::centredLeft, false);

    // pannelli
    for (auto* r : { &stutterPanel, &freezePanel })
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r->toFloat(), 12.0f * s);
        g.setColour (panelEdge);
        g.drawRoundedRectangle (r->toFloat().reduced (0.5f), 12.0f * s, 1.0f);
    }
    g.setColour (accent);
    g.setFont (makeFont (12.0f * s, true));
    g.drawText ("STUTTER", stutterPanel.getX() + juce::roundToInt (14 * s), stutterPanel.getY() + juce::roundToInt (6 * s),
                juce::roundToInt (200 * s), juce::roundToInt (20 * s), juce::Justification::centredLeft, false);
    g.drawText ("FREEZE / MASTER", freezePanel.getX() + juce::roundToInt (14 * s), freezePanel.getY() + juce::roundToInt (6 * s),
                juce::roundToInt (200 * s), juce::roundToInt (20 * s), juce::Justification::centredLeft, false);
    g.setColour (dim);
    g.drawText ("ROUTING", routeLabelRect, juce::Justification::centred, false);
}

void StuckFreezeEditor::resized()
{
    const float s = (float) getWidth() / 1000.0f;
    auto R = [s] (float x, float y, float w, float h)
    {
        return juce::Rectangle<int> (juce::roundToInt (x * s), juce::roundToInt (y * s),
                                     juce::roundToInt (w * s), juce::roundToInt (h * s));
    };

    inMeter.setBounds  (R (6, 56, 50, 474));
    outMeter.setBounds (R (944, 56, 50, 474));

    stutterBtn.setBounds (R (147, 58, 216, 216));
    freezeBtn.setBounds  (R (637, 58, 216, 216));
    led.setBounds        (R (402, 112, 196, 96));

    stutterPanel = R (62, 290, 520, 240);
    freezePanel  = R (594, 290, 344, 240);

    // Stutter
    lenStrip.setBounds   (R (76, 320, 260, 28));
    syncBtn.setBounds    (R (348, 320, 100, 28));
    reverseBtn.setBounds (R (458, 320, 112, 28));
    keyStrip.setBounds   (R (76, 356, 492, 28));

    const float kx = 66.0f, ky = 390.0f, kw = 102.0f, kh = 136.0f;
    placeKnob (kFree,   R (kx + 0 * kw, ky, kw, kh), s);
    placeKnob (kSmooth, R (kx + 1 * kw, ky, kw, kh), s);
    placeKnob (kPitch,  R (kx + 2 * kw, ky, kw, kh), s);
    placeKnob (kLowCut, R (kx + 3 * kw, ky, kw, kh), s);
    placeKnob (kOct,    R (kx + 4 * kw, ky, kw, kh), s);

    // Freeze + master
    const float fx = 600.0f, fw = 83.0f;
    placeKnob (kSmear,   R (fx + 0 * fw, 316, fw, 104), s);
    placeKnob (kFrPitch, R (fx + 1 * fw, 316, fw, 104), s);
    placeKnob (kReducer, R (fx + 2 * fw, 316, fw, 104), s);
    placeKnob (kFrMix,   R (fx + 3 * fw, 316, fw, 104), s);
    placeKnob (kMix,     R (fx + 0 * fw, 424, fw, 104), s);
    placeKnob (kOut,     R (fx + 1 * fw, 424, fw, 104), s);

    routeLabelRect = R (fx + 2 * fw + 6, 444, 152, 16);
    routeStrip.setBounds (R (fx + 2 * fw + 6, 466, 152, 28));
}
