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
BigKnob::BigKnob()
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    setMouseDragSensitivity (260);
}

void BigKnob::setCenterText (const juce::String& t)
{
    if (t != centerText) { centerText = t; repaint(); }
}

void BigKnob::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    const float d = std::min (b.getWidth(), b.getHeight());
    const float cx = b.getCentreX(), cy = b.getCentreY();
    const float outerR = d * 0.5f;
    const float arcR = outerR - d * 0.035f;
    const float th = d * 0.035f;
    const float start = juce::MathConstants<float>::pi * 1.25f;
    const float end   = juce::MathConstants<float>::pi * 2.75f;
    const float pos = (float) valueToProportionOfLength (getValue());
    const float ang = start + pos * (end - start);

    juce::Path track;
    track.addCentredArc (cx, cy, arcR, arcR, 0.0f, start, end, true);
    g.setColour (panelEdge);
    g.strokePath (track, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (pos > 0.001f)
    {
        juce::Path val;
        val.addCentredArc (cx, cy, arcR, arcR, 0.0f, start, ang, true);
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float bodyR = outerR - d * 0.10f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34343f), cx - bodyR * 0.5f, cy - bodyR * 0.7f,
                                             juce::Colour (0xff15151c), cx + bodyR * 0.6f, cy + bodyR * 0.8f, false));
    g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.5f);

    const float dotR = d * 0.028f, dr = bodyR * 0.80f;
    g.setColour (juce::Colours::white);
    g.fillEllipse (cx + dr * std::sin (ang) - dotR, cy - dr * std::cos (ang) - dotR, dotR * 2.0f, dotR * 2.0f);

    g.setColour (juce::Colours::white);
    g.setFont (makeFont (d * 0.20f, true));
    g.drawText (centerText, juce::Rectangle<float> (cx - bodyR, cy - bodyR * 0.45f, bodyR * 2.0f, bodyR * 0.6f),
                juce::Justification::centred, false);
    g.setColour (dim);
    g.setFont (makeFont (d * 0.055f, true));
    g.drawText ("STUCK", juce::Rectangle<float> (cx - bodyR, cy + bodyR * 0.2f, bodyR * 2.0f, bodyR * 0.25f),
                juce::Justification::centred, false);
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
int EnvelopeEditor::target() const
{
    return (int) proc.apvts.getRawParameterValue ("envTarget")->load();
}

juce::Point<float> EnvelopeEditor::toPix (juce::Point<float> p) const
{
    auto a = plotArea();
    return { a.getX() + p.x * a.getWidth(), a.getBottom() - p.y * a.getHeight() };
}

juce::Point<float> EnvelopeEditor::fromPix (juce::Point<float> px) const
{
    auto a = plotArea();
    return { juce::jlimit (0.0f, 1.0f, (px.x - a.getX()) / a.getWidth()),
             juce::jlimit (0.0f, 1.0f, 1.0f - (px.y - a.getY()) / a.getHeight()) };
}

void EnvelopeEditor::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0d0d12));
    g.fillRoundedRectangle (b, 6.0f);

    const int tgt = target();
    const int idx = juce::jmax (0, tgt - 1);
    const bool on = tgt > 0;
    auto a = plotArea();

    g.setColour (panelEdge.withAlpha (0.7f));
    for (int i = 1; i < 4; ++i)
    {
        g.drawVerticalLine ((int) (a.getX() + a.getWidth() * (float) i / 4.0f), a.getY(), a.getBottom());
        g.drawHorizontalLine ((int) (a.getY() + a.getHeight() * (float) i / 4.0f), a.getX(), a.getRight());
    }

    const auto& pts = proc.envPoints[idx];
    if (pts.size() >= 2)
    {
        juce::Path line, fill;
        for (size_t i = 0; i < pts.size(); ++i)
        {
            auto px = toPix (pts[i]);
            if (i == 0) { line.startNewSubPath (px); fill.startNewSubPath (px.x, a.getBottom()); fill.lineTo (px); }
            else        { line.lineTo (px); fill.lineTo (px); }
        }
        fill.lineTo (toPix (pts.back()).x, a.getBottom());
        fill.closeSubPath();

        g.setColour (accent.withAlpha (on ? 0.22f : 0.06f));
        g.fillPath (fill);
        g.setColour (on ? accent : dim.withAlpha (0.4f));
        g.strokePath (line, juce::PathStrokeType (2.0f));

        if (on)
            for (auto& p : pts)
            {
                auto px = toPix (p);
                g.setColour (juce::Colours::white);
                g.fillEllipse (px.x - 4.0f, px.y - 4.0f, 8.0f, 8.0f);
            }
    }

    if (on && phase >= 0.0f)
    {
        const float x = a.getX() + phase * a.getWidth();
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
    }

    g.setColour (dim);
    g.setFont (makeFont (9.0f, true));
    const char* top[]    = { "+200%", "+12 st", "100%" };
    const char* bottom[] = { "-200%", "-12 st", "0%" };
    if (on)
    {
        g.drawText (top[idx],    juce::Rectangle<float> (a.getX() + 2, a.getY(), 50, 12), juce::Justification::left, false);
        g.drawText (bottom[idx], juce::Rectangle<float> (a.getX() + 2, a.getBottom() - 12, 50, 12), juce::Justification::left, false);
    }
    else
    {
        g.setFont (makeFont (12.0f, true));
        g.drawText ("ENVELOPE OFF  -  scegli SPEED, PITCH o SMEAR", b, juce::Justification::centred, false);
    }
}

void EnvelopeEditor::mouseDown (const juce::MouseEvent& e)
{
    const int tgt = target();
    if (tgt == 0) return;
    const int idx = tgt - 1;

    auto pts = proc.envPoints[idx];
    int nearest = -1;
    float best = 12.0f;
    for (size_t i = 0; i < pts.size(); ++i)
    {
        const float d = toPix (pts[i]).getDistanceFrom (e.position);
        if (d < best) { best = d; nearest = (int) i; }
    }

    dragIdx = -1;
    if (e.mods.isRightButtonDown() || e.mods.isShiftDown())
    {
        if (nearest > 0 && nearest < (int) pts.size() - 1)
        {
            pts.erase (pts.begin() + nearest);
            proc.setEnvPoints (idx, pts);
        }
    }
    else if (nearest >= 0)
        dragIdx = nearest;
    else
    {
        const auto np = fromPix (e.position);
        size_t ins = 1;
        while (ins < pts.size() - 1 && pts[ins].x < np.x) ++ins;
        pts.insert (pts.begin() + (long) ins, np);
        proc.setEnvPoints (idx, pts);
        dragIdx = (int) ins;
    }
    repaint();
}

void EnvelopeEditor::mouseDrag (const juce::MouseEvent& e)
{
    const int tgt = target();
    if (tgt == 0 || dragIdx < 0) return;
    const int idx = tgt - 1;

    auto pts = proc.envPoints[idx];
    if (dragIdx >= (int) pts.size()) return;

    auto np = fromPix (e.position);
    if (dragIdx == 0)                           np.x = 0.0f;
    else if (dragIdx == (int) pts.size() - 1)   np.x = 1.0f;
    else np.x = juce::jlimit (pts[(size_t) dragIdx - 1].x + 0.002f, pts[(size_t) dragIdx + 1].x - 0.002f, np.x);

    pts[(size_t) dragIdx] = np;
    proc.setEnvPoints (idx, pts);
    repaint();
}

//==============================================================================
StuckFreezeEditor::StuckFreezeEditor (StuckFreezeProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      lenStrip   (p.apvts, "stLen",   { "1/1", "1/2", "1/4", "1/8", "1/16" }),
      keyStrip   (p.apvts, "stKey",   { "OFF", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }),
      routeStrip (p.apvts, "mode",    { "S>F", "F>S", "PAR" }),
      dropStrip  (p.apvts, "stDrop",  { "CUT", "TAIL", "REVERSE" }),
      curveStrip (p.apvts, "stCurve", { "LIN", "EXP", "S" }),
      rideStrip  (p.apvts, "stRide",  { "OFF", "1", "2", "4", "8" }),
      envTargetStrip (p.apvts, "envTarget", { "OFF", "SPEED", "PITCH", "SMEAR" }),
      playStrip  (p.apvts, "frPlay",  { "ONCE", "LOOP", "PING" }),
      frLenStrip (p.apvts, "frLen",   { "1/4", "1/2", "1", "2", "4", "8" }),
      envEditor  (p)
{
    setLookAndFeel (&lnf);

    stuckAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "stuck", stuckKnob);
    frAtt    = std::make_unique<BA> (proc.apvts, "frOn",   freezeBtn);
    syncAtt  = std::make_unique<BA> (proc.apvts, "stSync", syncBtn);
    goAtt    = std::make_unique<BA> (proc.apvts, "stGo",   goBtn);
    liveAtt  = std::make_unique<BA> (proc.apvts, "frLive", liveBtn);
    liveBtn.setClickingTogglesState (true);
    syncBtn.setClickingTogglesState (true);
    goBtn.setClickingTogglesState (true);

    setupKnob (kFree,    "stFreeMs",  "FREE TIME");
    setupKnob (kSmooth,  "stSmooth",  "SMOOTH");
    setupKnob (kRange,   "stRange",   "RANGE");
    setupKnob (kLowCut,  "stLowCut",  "LOW CUT");
    setupKnob (kSpeed,   "frSpeed",   "SPEED");
    setupKnob (kFrPitch, "frPitch",   "PITCH");
    setupKnob (kSmear,   "frSmear",   "SMEAR");
    setupKnob (kReducer, "frReducer", "REDUCER");
    setupKnob (kGrain,   "frGrain",   "GRAIN SIZE");
    setupKnob (kEngine,  "frEngine",  "GRAIN<>FFT");
    setupKnob (kFrMix,   "frMix",     "FREEZE MIX");
    setupKnob (kMix,     "mix",       "MIX");
    setupKnob (kOut,     "outGain",   "OUTPUT");

    for (juce::Component* c : { (juce::Component*) &inMeter, (juce::Component*) &outMeter,
                                (juce::Component*) &stuckKnob, (juce::Component*) &freezeBtn,
                                (juce::Component*) &syncBtn, (juce::Component*) &goBtn, (juce::Component*) &liveBtn,
                                (juce::Component*) &lenStrip, (juce::Component*) &keyStrip,
                                (juce::Component*) &routeStrip, (juce::Component*) &dropStrip,
                                (juce::Component*) &curveStrip, (juce::Component*) &rideStrip,
                                (juce::Component*) &envTargetStrip, (juce::Component*) &playStrip,
                                (juce::Component*) &frLenStrip, (juce::Component*) &envEditor,
                                (juce::Component*) &led })
        addAndMakeVisible (*c);

    setResizable (true, true);
    setResizeLimits (700, 476, 2000, 1360);
    getConstrainer()->setFixedAspectRatio (1000.0 / 680.0);
    setSize (1000, 680);
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

    // Ride: la manopola si muove da sola; a fine ride il tasto GO si rilascia
    const float kParam = proc.apvts.getRawParameterValue ("stuck")->load();
    const bool riding = proc.rideActive.load();
    if (riding)
    {
        stuckKnob.setValue (std::max ((double) kParam, (double) proc.rideProgress.load()), juce::dontSendNotification);
        wasRiding = true;
    }
    else if (wasRiding)
    {
        stuckKnob.setValue ((double) kParam, juce::dontSendNotification);
        wasRiding = false;
    }
    if (proc.rideDone.exchange (false))
        if (auto* goParam = proc.apvts.getParameter ("stGo"))
            goParam->setValueNotifyingHost (0.0f);

    static const char* notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* lens[]  = { "1/1", "1/2", "1/4", "1/8", "1/16" };

    const float ms    = proc.loopMs.load();
    const float denom = proc.sliceDenom.load();
    const int key  = (int) proc.apvts.getRawParameterValue ("stKey")->load();
    const int len  = juce::jlimit (0, 4, (int) proc.apvts.getRawParameterValue ("stLen")->load());
    const bool sync = proc.apvts.getRawParameterValue ("stSync")->load() > 0.5f;
    const bool active = ms > 0.0f;

    // testo al centro della manopola
    if (! active)            stuckKnob.setCenterText ("OFF");
    else if (denom > 0.0f)   stuckKnob.setCenterText ("1/" + juce::String (juce::roundToInt (denom)));
    else                     stuckKnob.setCenterText (juce::String (ms, ms < 100.0f ? 1 : 0) + " ms");

    juce::String small = key > 0 ? "KEY " + juce::String (notes[key - 1]) + " MIN"
                                 : (sync ? "GRID " + juce::String (lens[len]) : "FREE");
    led.setText (active ? juce::String (ms, ms < 100.0f ? 1 : 0) + " MS" : "---", small);

    syncBtn.setButtonText (sync ? "SYNC" : "FREE");
    envEditor.setPhase (proc.envPhase.load());
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
    for (auto* r : { &stutterPanel, &freezePanel, &envPanel, &masterPanel })
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
    g.drawText ("FREEZE", freezePanel.getX() + juce::roundToInt (14 * s), freezePanel.getY() + juce::roundToInt (6 * s),
                juce::roundToInt (200 * s), juce::roundToInt (20 * s), juce::Justification::centredLeft, false);
    g.drawText ("FREEZE ENVELOPE   -   TARGET  |  MODE  |  LENGTH (BARS)", envPanel.getX() + juce::roundToInt (14 * s),
                envPanel.getY() + juce::roundToInt (6 * s), juce::roundToInt (480 * s), juce::roundToInt (20 * s),
                juce::Justification::centredLeft, false);
    g.drawText ("MASTER", masterPanel.getX() + juce::roundToInt (14 * s), masterPanel.getY() + juce::roundToInt (6 * s),
                juce::roundToInt (200 * s), juce::roundToInt (20 * s), juce::Justification::centredLeft, false);
    g.setColour (dim);
    g.setFont (makeFont (10.0f * s, true));
    for (auto& l : smallLabels)
        g.drawText (l.first, l.second, juce::Justification::centredLeft, false);
}

void StuckFreezeEditor::resized()
{
    const float s = (float) getWidth() / 1000.0f;
    auto R = [s] (float x, float y, float w, float h)
    {
        return juce::Rectangle<int> (juce::roundToInt (x * s), juce::roundToInt (y * s),
                                     juce::roundToInt (w * s), juce::roundToInt (h * s));
    };

    inMeter.setBounds  (R (6, 56, 50, 614));
    outMeter.setBounds (R (944, 56, 50, 614));

    // zona centrale: manopola STUCK, display LED, pulsante FREEZE
    stuckKnob.setBounds (R (155, 46, 200, 206));
    freezeBtn.setBounds (R (645, 46, 200, 206));
    led.setBounds       (R (402, 100, 196, 96));

    smallLabels.clear();
    smallLabels.push_back ({ "RIDE", R (112, 262, 36, 24) });
    rideStrip.setBounds (R (150, 260, 170, 28));
    goBtn.setBounds     (R (326, 260, 44, 28));

    stutterPanel = R (62, 294, 520, 236);
    freezePanel  = R (594, 294, 344, 236);

    // Stutter
    smallLabels.push_back ({ "GRID",        R (76, 313, 200, 14) });
    smallLabels.push_back ({ "ON THE DROP", R (376, 313, 190, 14) });
    lenStrip.setBounds  (R (76, 328, 200, 26));
    syncBtn.setBounds   (R (284, 328, 84, 26));
    dropStrip.setBounds (R (376, 328, 192, 26));
    keyStrip.setBounds  (R (76, 360, 492, 26));

    const float kx = 66.0f, ky = 392.0f, kw = 102.0f, kh = 136.0f;
    placeKnob (kFree,   R (kx + 0 * kw, ky, kw, kh), s);
    placeKnob (kSmooth, R (kx + 1 * kw, ky, kw, kh), s);
    placeKnob (kRange,  R (kx + 2 * kw, ky, kw, kh), s);
    placeKnob (kLowCut, R (kx + 3 * kw, ky, kw, kh), s);

    smallLabels.push_back ({ "CURVE", R (486, 400, 80, 14) });
    curveStrip.setBounds (R (480, 420, 90, 26));

    // Freeze
    const float fx = 600.0f, fw = 83.0f;
    placeKnob (kSpeed,   R (fx + 0 * fw, 320, fw, 104), s);
    placeKnob (kFrPitch, R (fx + 1 * fw, 320, fw, 104), s);
    placeKnob (kSmear,   R (fx + 2 * fw, 320, fw, 104), s);
    placeKnob (kReducer, R (fx + 3 * fw, 320, fw, 104), s);
    placeKnob (kGrain,   R (fx + 0 * fw, 426, fw, 102), s);
    placeKnob (kEngine,  R (fx + 1 * fw, 426, fw, 102), s);
    placeKnob (kFrMix,   R (fx + 2 * fw, 426, fw, 102), s);
    liveBtn.setBounds (R (fx + 3 * fw + 2, 452, 80, 30));

    // Inviluppo
    envPanel = R (62, 538, 520, 134);
    envTargetStrip.setBounds (R (76, 562, 200, 24));
    playStrip.setBounds      (R (282, 562, 120, 24));
    frLenStrip.setBounds     (R (408, 562, 160, 24));
    envEditor.setBounds      (R (76, 592, 492, 74));

    // Master
    masterPanel = R (594, 538, 344, 134);
    placeKnob (kMix, R (fx + 0 * fw, 560, fw, 108), s);
    placeKnob (kOut, R (fx + 1 * fw, 560, fw, 108), s);
    smallLabels.push_back ({ "ROUTING", R (fx + 2 * fw + 6, 574, 152, 14) });
    routeStrip.setBounds (R (fx + 2 * fw + 6, 592, 152, 28));
}
