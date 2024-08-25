/*
  ==============================================================================

    Este arquivo contém o código básico para um processador de plugin JUCE.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EqualizadorAudioProcessor::EqualizadorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{

}

EqualizadorAudioProcessor::~EqualizadorAudioProcessor()
{
}

//==============================================================================
const juce::String EqualizadorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EqualizadorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EqualizadorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EqualizadorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EqualizadorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EqualizadorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: alguns hosts não lidam bem se você disser que há 0 programas,
                // então isso deve ser pelo menos 1, mesmo que você não esteja realmente implementando programas.
}

int EqualizadorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EqualizadorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EqualizadorAudioProcessor::getProgramName (int index)
{
    return {};
}

void EqualizadorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void /*EqualizadorAudioProcessor::*/updateCoefficients(juce::dsp::IIR::Filter<float>::CoefficientsPtr& old, const juce::dsp::IIR::Filter<float>::CoefficientsPtr& replacements)
{
    *old = *replacements;
}

void EqualizadorAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings) 
{
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());

    auto& leftLowCut = leftChannelChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChannelChain.get<ChainPositions::LowCut>();

    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void EqualizadorAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());;

    auto& leftHighCut = leftChannelChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChannelChain.get<ChainPositions::HighCut>();

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void EqualizadorAudioProcessor::updateFilters() 
{
    auto chainSettings = getChainSettings(apvts);

    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
}

juce::dsp::IIR::Filter<float>::CoefficientsPtr makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGain));
}

void EqualizadorAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{

    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());
    updateCoefficients(leftChannelChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChannelChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

//==============================================================================
void EqualizadorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChannelChain.prepare(spec);
    rightChannelChain.prepare(spec);

    updateFilters();
}

void EqualizadorAudioProcessor::releaseResources()
{
    // Quando a reprodução para, você pode usar isso como uma oportunidade para liberar qualquer
    // memória extra, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EqualizadorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // Este é o lugar onde você verifica se o layout é suportado.
    // Neste código de modelo, só suportamos layouts mono ou estéreo.
    // Alguns hosts de plugin, como certas versões do GarageBand, só carregam
    // plugins que suportam layouts de barramento estéreo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Verifica se o layout de entrada corresponde ao layout de saída
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EqualizadorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Limpa canais de saída que não têm dados de entrada
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateFilters();

    // Cria um AudioBlock a partir do buffer
    juce::dsp::AudioBlock<float> audioBlock(buffer);
    auto leftAudioBlock = audioBlock.getSingleChannelBlock(0);
    auto rightAudioBlock = audioBlock.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftAudioBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightAudioBlock);

    leftChannelChain.process(leftContext);
    rightChannelChain.process(rightContext);

}

//==============================================================================
bool EqualizadorAudioProcessor::hasEditor() const
{
    return true; // (altere isso para false se você optar por não fornecer um editor)
}

juce::AudioProcessorEditor* EqualizadorAudioProcessor::createEditor()
{

    return new EqualizadorAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void EqualizadorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Você deve usar este método para armazenar seus parâmetros no bloco de memória.
    // Você pode fazer isso como dados brutos ou usar as classes XML ou ValueTree
    // como intermediários para facilitar o salvamento e carregamento de dados complexos.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void EqualizadorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Você deve usar este método para restaurar seus parâmetros deste bloco de memória,
    // cujo conteúdo será criado pela chamada getStateInformation().
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
        updateFilters();
    }
}

//==============================================================================
// Isso cria novas instâncias do plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EqualizadorAudioProcessor();
}

// Define o Layout dos 3 parâmetros: Low Band, High Band e Parametric/Peak Band
juce::AudioProcessorValueTreeState::ParameterLayout EqualizadorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut", "LowCut", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut", "HighCut", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak", "Peak", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 750.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain", "Peak Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality", "Peak Quality", juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));

    // Inclinação mínima de 12 dB/oitava para LowCut e HighCut
    juce::StringArray strArr;
    for (int i = 0; i < 4; ++i) 
    {
        juce::String str;
        str << (12 + 12 * i);
        str << " dB/Oct";
        strArr.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", strArr, 0)); // Inicializa com 12 dB/oitava
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", strArr, 0)); // Inicializa com 12 dB/oitava
    return layout;
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    // Recupera valores do ValueTreeState e atribui à estrutura ChainSettings
    settings.peakFreq = apvts.getRawParameterValue("Peak")->load();
    settings.peakGain = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut")->load();

    settings.lowCutSlope = static_cast<Slope>(static_cast<int>(apvts.getRawParameterValue("LowCut Slope")->load()));
    settings.highCutSlope = static_cast<Slope>(static_cast<int>(apvts.getRawParameterValue("HighCut Slope")->load()));

    return settings;
}