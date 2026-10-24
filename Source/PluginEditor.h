#pragma once
#include "PluginProcessor.h"

namespace ui
{
    inline const juce::Colour bg        { 0xff14141b };
    inline const juce::Colour panel     { 0xff1c1c26 };
    inline const juce::Colour panelEdge { 0xff2a2a38 };
    inline const juce::Colour accent    { 0xff9b6cff };
    inline const juce::Colour accentDim { 0xff5b3fa0 };
    inline const juce::Colour text      { 0xffe8e8f0 };
    inline const juce::Colour dim       { 0xff8a8a9a };
    inline const juce::Colour ledRed    { 0xffff3b30 };

    inline juce::Font makeFont (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
    }
}

// ---- Look and feel: manopole stile Stucker ----
class StuckLnF : public juce::LookAndFeel_V4
{
public:
    StuckLnF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Font getLabelFont (juce::Label& l) override;
};

// ---- Grande pulsante rotondo: STUTTER (scuro) o FREEZE (metallico, stile TimeFreeze) ----
class RoundButton : public juce::Button
{
public:
    enum class Kind { stutter, freeze };
    explicit RoundButton (Kind k);
    void paintButton (juce::Graphics&, bool over, bool down) override;
    bool hitTest (int x, int y) override;
private:
    Kind kind;
};

// ---- Selettore a segmenti collegato a un parametro "choice" ----
class ChoiceStrip : public juce::Component
{
public:
    ChoiceStrip (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, juce::StringArray items);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::StringArray items;
    int selected = 0;
    juce::ParameterAttachment attachment;
};

// ---- Meter verticale con scala dB ----
class LevelMeter : public juce::Component
{
public:
    explicit LevelMeter (bool labelsOnRight) : right (labelsOnRight) {}
    void setLevel (float lin) { level = lin; repaint(); }
    void paint (juce::Graphics&) override;
private:
    float level = 0.0f;
    bool right;
};

// ---- Display LED rosso ----
class LedDisplay : public juce::Component
{
public:
    void setText (const juce::String& big, const juce::String& small);
    void paint (juce::Graphics&) override;
private:
    juce::String bigText, smallText;
};

// ---- Grande manopola STUCK (stile Stucker): OFF -> la giri e il loop si accorcia sempre di piu' ----
class BigKnob : public juce::Slider
{
public:
    BigKnob();
    void setCenterText (const juce::String& t);
    void paint (juce::Graphics&) override;
private:
    juce::String centerText { "OFF" };
};

// ---- Editor dell'inviluppo (click = aggiungi/trascina punto, Shift o tasto destro = elimina) ----
class EnvelopeEditor : public juce::Component
{
public:
    explicit EnvelopeEditor (StuckFreezeProcessor& p) : proc (p) {}
    void setPhase (float ph) { if (ph != phase) { phase = ph; repaint(); } }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragIdx = -1; }
private:
    juce::Rectangle<float> plotArea() const { return getLocalBounds().toFloat().reduced (8.0f, 8.0f); }
    juce::Point<float> toPix (juce::Point<float> p) const;
    juce::Point<float> fromPix (juce::Point<float> px) const;
    int target() const;

    StuckFreezeProcessor& proc;
    int dragIdx = -1;
    float phase = -1.0f;
};

class StuckFreezeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit StuckFreezeEditor (StuckFreezeProcessor&);
    ~StuckFreezeEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    void setupKnob (Knob&, const juce::String& id, const juce::String& label);
    void placeKnob (Knob&, juce::Rectangle<int> cell, float s);
    void timerCallback() override;

    StuckFreezeProcessor& proc;
    StuckLnF lnf;

    LevelMeter inMeter { false }, outMeter { true };
    BigKnob stuckKnob;
    RoundButton freezeBtn { RoundButton::Kind::freeze };
    juce::TextButton syncBtn { "SYNC" }, goBtn { "GO" }, liveBtn { "LIVE FX" };
    ChoiceStrip lenStrip, keyStrip, routeStrip, dropStrip, curveStrip, rideStrip;
    ChoiceStrip envTargetStrip, playStrip, frLenStrip;
    EnvelopeEditor envEditor;
    LedDisplay led;

    Knob kFree, kSmooth, kRange, kLowCut;
    Knob kSpeed, kSmear, kFrPitch, kReducer, kGrain, kEngine, kFrMix, kMix, kOut;

    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BA> frAtt, syncAtt, goAtt, liveAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stuckAtt;

    juce::Rectangle<int> stutterPanel, freezePanel, envPanel, masterPanel;
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> smallLabels;
    bool wasRiding = false;
    float meterIn = 0.0f, meterOut = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StuckFreezeEditor)
};
