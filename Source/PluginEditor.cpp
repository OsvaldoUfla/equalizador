/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPosProportional, float rotaryStartAngle,
    float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height);
    g.setColour(juce::Colour(15, 15, 15));
    g.fillEllipse(bounds);

    g.setColour(juce::Colours::white);
    g.drawEllipse(bounds, 1.f);

    if (auto* sliderWithLabels = dynamic_cast<SliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        juce::Path p;

        juce::Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - sliderWithLabels->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = juce::jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(juce::AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
        g.fillPath(p);

        g.setFont(sliderWithLabels->getTextHeight());
        auto text = sliderWithLabels->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, sliderWithLabels->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());

        g.setColour(juce::Colours::black);
        g.fillRect(r);

        g.setColour(juce::Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float minSliderPos, float maxSliderPos,
    const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    auto bounds = juce::Rectangle<int>(x, y, width, height);
    auto sliderBounds = bounds.reduced(10); // Margem para a aparência do slider

    // Desenhe o fundo do slider
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(bounds);

    // Desenhe o trilho do slider
    g.setColour(juce::Colours::lightgrey);
    g.fillRect(sliderBounds);

    // Desenhe o controle do slider
    g.setColour(juce::Colours::blue);
    auto controlWidth = 10; // Ajuste o tamanho do controle do slider
    auto controlBounds = sliderBounds;
    controlBounds.setX(sliderPos - controlWidth / 2);
    controlBounds.setWidth(controlWidth);

    g.fillRect(controlBounds);

    // Desenhe o texto do label, se houver
    if (auto* verticalSlider = dynamic_cast<VerticalSliderWithLabels*>(&slider))
    {
        g.setColour(juce::Colours::white);
        g.setFont(verticalSlider->getTextHeight());

        auto labelText = verticalSlider->getDisplayString();
        auto textBounds = bounds;
        textBounds.setX(bounds.getX());
        textBounds.setWidth(bounds.getWidth());
        textBounds.setBottom(bounds.getBottom() - verticalSlider->getTextHeight() - 5);

        g.drawFittedText(labelText, textBounds, juce::Justification::centredBottom, 1);
    }
}

//==============================================================================
// Implementação de SliderWithLabels

SliderWithLabels::SliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix, juce::Slider::SliderStyle style)
    : juce::Slider(style, juce::Slider::TextEntryBoxPosition::NoTextBox),
    param(&rap),
    suffix(unitSuffix)
{
    setLookAndFeel(&lnf);
}

SliderWithLabels::~SliderWithLabels()
{
    setLookAndFeel(nullptr);
}

juce::Rectangle<int> SliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), bounds.getCentreY());

    return r;
}

juce::String SliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    bool addK = false;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();
        if (val > 999.f)
        {
            val /= 1000.f;
            addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse;
    }

    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
            str << "k";
        str << suffix;
    }
    return str;
}

//==============================================================================
// Implementação de VerticalSliderWithLabels

VerticalSliderWithLabels::VerticalSliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix)
    : SliderWithLabels(rap, unitSuffix, juce::Slider::SliderStyle::LinearVertical)
{
}

void VerticalSliderWithLabels::paint(juce::Graphics& g)
{
    auto range = getRange();
    auto sliderBounds = getSliderBounds();
    auto thumbPos = juce::jmap<float>(getValue(), range.getStart(), range.getEnd(), 0.0f, static_cast<float>(sliderBounds.getHeight()));

    // Desenhar o fundo do slider
    g.setColour(juce::Colours::black); // Cor de fundo do slider
    g.fillRect(sliderBounds);

    // Desenhar o trilho do slider
    g.setColour(juce::Colour(51, 153, 255)); // Cor do trilho do slider
    g.fillRect(sliderBounds
        .withTop(sliderBounds.getBottom() - thumbPos)
        .withHeight(thumbPos));

    // Desenhar o controle (thumb) como um círculo
    g.setColour(juce::Colours::white); // Cor do controle (thumb)
    auto thumbWidth = sliderBounds.getWidth();
    auto thumbHeight = 10; // Ajuste para afinar o thumb
    auto thumbBounds = juce::Rectangle<int>(
        sliderBounds.getX() + (sliderBounds.getWidth() - thumbWidth) / 2,
        sliderBounds.getBottom() - thumbPos - thumbHeight / 2,
        thumbWidth,
        thumbHeight
    );

    g.fillEllipse(thumbBounds.toFloat());

    // Ajustar a label abaixo do slider
    g.setColour(juce::Colours::white);
    g.setFont(getTextHeight());
    auto textBounds = getLocalBounds().removeFromBottom(getTextHeight() + 5); // Ajustar a posição da label

    g.drawFittedText(getDisplayString(), textBounds, juce::Justification::centredBottom, 1);
}

juce::Rectangle<int> VerticalSliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    auto textHeight = getTextHeight();
    auto sliderWidth = 12; // Ajustar a largura do slider
    auto sliderArea = bounds.removeFromTop(bounds.getHeight() - textHeight - 15); // Ajuste para garantir que o slider ocupe o espaço disponível

    juce::Rectangle<int> r(sliderArea.getX() + (sliderArea.getWidth() - sliderWidth) / 2, sliderArea.getY() +8,
        sliderWidth, sliderArea.getHeight());

    return r;

}

//==============================================================================
// Implementação de RotarySliderWithLabels

RotarySliderWithLabels::RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix)
    : SliderWithLabels(rap, unitSuffix, juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag)
{
}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    auto startAng = juce::degreesToRadians(180.f + 45.f);
    auto endAng = juce::degreesToRadians(180.f + 45.f) + juce::MathConstants<float>::twoPi;

    auto range = getRange();
    auto sliderBounds = getSliderBounds();

    //g.setColour(juce::Colours::red);
    //g.drawRect(getLocalBounds());
    //g.setColour(juce::Colours::yellow);
    //g.drawRect(getLocalBounds());

    getLookAndFeel().drawRotarySlider(g, sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(),
        juce::jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), startAng, endAng, *this);
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    return SliderWithLabels::getSliderBounds();
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(EqualizadorAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        // Atualiza a corrente mono
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        auto sampleRate = audioProcessor.getSampleRate();
        auto peakCoefficients = makePeakFilter(chainSettings, sampleRate);
        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

        auto lowCutCoefficients = makeLowCutFilter(chainSettings, sampleRate);
        auto highCutCoefficients = makeHighCutFilter(chainSettings, sampleRate);

        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
        repaint();
    }
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::black);
    auto responseArea = getLocalBounds();

    auto w = responseArea.getWidth();
    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();
    std::vector<double> mags;

    mags.resize(w);
    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = juce::mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = juce::Decibels::gainToDecibels(mag);
    }

    juce::Path responseCurve;
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
        {
            return juce::jmap(input, -24.0, 24.0, outputMin, outputMax);
        };
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(juce::Colour(0, 128, 255));
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

    g.setColour(juce::Colours::white);
    g.strokePath(responseCurve, juce::PathStrokeType(2.f));
}

//==============================================================================
EqualizadorAudioProcessorEditor::EqualizadorAudioProcessorEditor (EqualizadorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak"), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),

    responseCurveComponent(audioProcessor),
    peakFreqSliderAttachment(audioProcessor.apvts, "Peak", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut", highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    setSize (750, 500);
}

EqualizadorAudioProcessorEditor::~EqualizadorAudioProcessorEditor()
{

}

//==============================================================================
void EqualizadorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colour(30, 30, 30));
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
}

void EqualizadorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    // Obtém a área disponível para os componentes
    auto bounds = getLocalBounds();

    // Adicionar espaço de sobra nos cantos
    int cornerMargin = 20;  // Define a margem em pixels
    bounds.reduce(cornerMargin, cornerMargin);  // Aplica a margem

    // Define a altura para a área de resposta, mantendo-a no topo
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    responseCurveComponent.setBounds(responseArea);

    // Área para sliders de frequência de corte e peak
    auto freqArea = bounds.removeFromTop(bounds.getHeight() * 0.5);

    // Dividir o espaço para os sliders de corte e peak
    auto lowCutArea = freqArea.removeFromLeft(freqArea.getWidth() / 3);
    auto highCutArea = freqArea.removeFromRight(freqArea.getWidth() / 2);

    // Sliders de frequência
    lowCutFreqSlider.setBounds(lowCutArea);
    peakFreqSlider.setBounds(freqArea);
    highCutFreqSlider.setBounds(highCutArea);

    // Ajuste do espaçamento entre sliders de slope e corte
    int slopeSpacing = 10; // Espaçamento extra

    // Sliders de slope ao lado oposto com espaçamento extra
    lowCutSlopeSlider.setBounds(lowCutArea.removeFromLeft(lowCutArea.getWidth() * 0.3).translated(-slopeSpacing, 0));  // À esquerda do LowCutFreq
    highCutSlopeSlider.setBounds(highCutArea.removeFromRight(highCutArea.getWidth() * 0.3).translated(slopeSpacing, 0));  // À direita do HighCutFreq

    // Sliders de ganho e qualidade abaixo do slider central, mais próximos do centro
    auto gainAndQualityArea = bounds;
    gainAndQualityArea.reduce(bounds.getWidth() / 6, 0);  // Reduz a largura, centralizando os sliders

    auto gainWidth = gainAndQualityArea.getWidth() / 2;

    peakGainSlider.setBounds(gainAndQualityArea.removeFromLeft(gainWidth));
    peakQualitySlider.setBounds(gainAndQualityArea);
}

std::vector<juce::Component*> EqualizadorAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent
    };
}