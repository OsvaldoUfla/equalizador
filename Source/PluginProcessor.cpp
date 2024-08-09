/*
  ==============================================================================

    Este arquivo contém o código básico para um processador de plugin JUCE.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MyAudioProcessorAudioProcessor::MyAudioProcessorAudioProcessor()
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
    // Inicialize os coeficientes dos filtros aqui (para 3 bandas como exemplo)
    // Alterado: O método setCoefficients foi substituído por atribuição direta aos coeficientes.
    filterBand1.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(getSampleRate(), 500.0f);
    filterBand2.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(getSampleRate(), 1000.0f, 1.0f);
    filterBand3.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), 3000.0f);
}

MyAudioProcessorAudioProcessor::~MyAudioProcessorAudioProcessor()
{
}

//==============================================================================
const juce::String MyAudioProcessorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MyAudioProcessorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MyAudioProcessorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MyAudioProcessorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MyAudioProcessorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MyAudioProcessorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: alguns hosts não lidam bem se você disser que há 0 programas,
                // então isso deve ser pelo menos 1, mesmo que você não esteja realmente implementando programas.
}

int MyAudioProcessorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MyAudioProcessorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MyAudioProcessorAudioProcessor::getProgramName (int index)
{
    return {};
}

void MyAudioProcessorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MyAudioProcessorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Inicialize os filtros com a taxa de amostragem atual
    // Alterado: Atualiza os coeficientes dos filtros com a taxa de amostragem atual.
    filterBand1.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 500.0f);
    filterBand2.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 1000.0f, 1.0f);
    filterBand3.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 3000.0f);

    // Resete os filtros
    // Alterado: Adiciona o reset dos filtros para garantir que estejam prontos para o processamento.
    filterBand1.reset();
    filterBand2.reset();
    filterBand3.reset();
}

void MyAudioProcessorAudioProcessor::releaseResources()
{
    // Quando a reprodução para, você pode usar isso como uma oportunidade para liberar qualquer
    // memória extra, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MyAudioProcessorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void MyAudioProcessorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Limpa canais de saída que não têm dados de entrada
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Processa áudio através dos filtros
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // Cria um AudioBlock a partir do buffer
        // Alterado: Adiciona a criação de um AudioBlock a partir do buffer.
        juce::dsp::AudioBlock<float> audioBlock(buffer);
        // Alterado: Cria o contexto de processamento a partir do AudioBlock.
        juce::dsp::ProcessContextReplacing<float> context(audioBlock);

        // Aplica filtros ao bloco de áudio
        // Alterado: Usa o método process do filtro com o contexto criado.
        filterBand1.process(context);
        filterBand2.process(context);
        filterBand3.process(context);
    }
}

//==============================================================================
bool MyAudioProcessorAudioProcessor::hasEditor() const
{
    return true; // (altere isso para false se você optar por não fornecer um editor)
}

juce::AudioProcessorEditor* MyAudioProcessorAudioProcessor::createEditor()
{
    return new MyAudioProcessorAudioProcessorEditor (*this);
}

//==============================================================================
void MyAudioProcessorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Você deve usar este método para armazenar seus parâmetros no bloco de memória.
    // Você pode fazer isso como dados brutos ou usar as classes XML ou ValueTree
    // como intermediários para facilitar o salvamento e carregamento de dados complexos.
}

void MyAudioProcessorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Você deve usar este método para restaurar seus parâmetros deste bloco de memória,
    // cujo conteúdo será criado pela chamada getStateInformation().
}

//==============================================================================
// Isso cria novas instâncias do plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyAudioProcessorAudioProcessor();
}
