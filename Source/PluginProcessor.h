/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// Configura��o dos filtros
struct ChainSettings
{
    float peakFreq{ 0 }, peakGain{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

/**
*/
class EqualizadorAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    EqualizadorAudioProcessor();
    ~EqualizadorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout()};

private:
    // Cadeia de processamento para o canal esquerdo e direito
    juce::dsp::ProcessorChain<
        juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>, // Primeiro filtro na cadeia LowCut
        juce::dsp::IIR::Filter<float>, // Segundo filtro na cadeia LowCut
        juce::dsp::IIR::Filter<float>, // Terciero filtro na cadeia LowCut
        juce::dsp::IIR::Filter<float> // Quarto filtro na cadeia LowCut
        >, 
        juce::dsp::IIR::Filter<float>, // Filtro intermedi�rio
        juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>, // Primeiro filtro na cadeia HighCut
        juce::dsp::IIR::Filter<float>, // Segundo filtro na cadeia HighCut
        juce::dsp::IIR::Filter<float>, // Terciero filtro na cadeia HighCut
        juce::dsp::IIR::Filter<float> // Quarto filtro na cadeia HighCut
        >
    > leftChannelChain, rightChannelChain;

    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };

    void updatePeakFilter(const ChainSettings& chainSettings);
    static void updateCoefficients(juce::dsp::IIR::Filter<float>::CoefficientsPtr& old, const juce::dsp::IIR::Filter<float>::CoefficientsPtr& replacements);

    template<int index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients)
    {
        updateCoefficients(chain.template get<index>().coefficients, coefficients[index]);
        chain.template setBypassed<index>(false);
    }

    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& cut, const CoefficientType& cutCoefficients, const Slope& slope)
    {
        // Inicializa todos os filtros como bypassed.
        cut.template setBypassed<0>(true);
        cut.template setBypassed<1>(true);
        cut.template setBypassed<2>(true);
        cut.template setBypassed<3>(true);

        // Define os coeficientes e desativa o bypass de acordo com a inclinação.
        switch (slope)
        {
        case Slope_48:
            update<3>(cut, cutCoefficients);

        case Slope_36:
            update<2>(cut, cutCoefficients);

        case Slope_24:
            update<1>(cut, cutCoefficients);

        case Slope_12:
            update<0>(cut, cutCoefficients);

        }
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqualizadorAudioProcessor)
};