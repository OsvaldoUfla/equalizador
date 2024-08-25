/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle,
        float rotaryEndAngle, juce::Slider&) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override;
};

class SliderWithLabels : public juce::Slider
{
public:
    SliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix, juce::Slider::SliderStyle style);
    ~SliderWithLabels() override;

    //virtual void paint(juce::Graphics& g) override = 0;
    virtual juce::Rectangle<int> getSliderBounds() const = 0;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;

protected:
    LookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
};


// Slider vertical com labels
class VerticalSliderWithLabels : public SliderWithLabels
{
public:
    VerticalSliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix);
    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const override;
};

// Slider rotativo com labels
class RotarySliderWithLabels : public SliderWithLabels
{
public:
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix);
    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const override;
};


/*
struct CustomVerticalSlider : juce::Slider
{
    CustomVerticalSlider() : juce::Slider(juce::Slider::SliderStyle::LinearVertical, juce::Slider::TextEntryBoxPosition::NoTextBox)
    {

    }
};
*/

struct ResponseCurveComponent : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer
{
    ResponseCurveComponent(EqualizadorAudioProcessor&);
    ~ResponseCurveComponent();
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}

    void timerCallback() override;
    void paint(juce::Graphics& g) override;
private:
    EqualizadorAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged{ false };
    juce::dsp::ProcessorChain<
        juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Filter<float>
        >,
        juce::dsp::IIR::Filter<float>,
        juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Filter<float>
        >
    > monoChain;
};

//==============================================================================
/**
*/
class EqualizadorAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    EqualizadorAudioProcessorEditor (EqualizadorAudioProcessor&);
    ~EqualizadorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EqualizadorAudioProcessor& audioProcessor;

    RotarySliderWithLabels peakFreqSlider,
        peakGainSlider,
        peakQualitySlider,
        lowCutFreqSlider,
        highCutFreqSlider;
        //lowCutSlopeSlider,
        //highCutSlopeSlider;

    VerticalSliderWithLabels lowCutSlopeSlider,
        highCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    juce::AudioProcessorValueTreeState::SliderAttachment peakFreqSliderAttachment,
        peakGainSliderAttachment,
        peakQualitySliderAttachment,
        lowCutFreqSliderAttachment,
        highCutFreqSliderAttachment,
        lowCutSlopeSliderAttachment,
        highCutSlopeSliderAttachment;

    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqualizadorAudioProcessorEditor)
};