/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
#include <array>
template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert(std::is_same_v<T, juce::AudioBuffer<float>>,
            "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for (auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                numSamples,
                false,   //clear everything?
                true,    //including the extra space?
                true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }

    void prepare(size_t numElements)
    {
        static_assert(std::is_same_v<T, std::vector<float>>,
            "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for (auto& buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }

    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }

        return false;
    }

    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }

        return false;
    }

    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{ Capacity };
};

enum Channel
{
    Right, // Efetivamente 0
    Left // Efetivamente 1
};

template<typename BlockType>
struct SingleChannelSampleFifo
{
    // Construtor que inicializa a estrutura com o canal de áudio especificado
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        // Inicializa o estado de preparado como falso
        prepared.set(false);
    }

    // Função para atualizar o FIFO com novos dados de áudio
    void update(const BlockType& buffer)
    {
        // Verifica se a estrutura está preparada para uso
        jassert(prepared.get());

        // Verifica se o buffer de áudio tem canais suficientes
        jassert(buffer.getNumChannels() > channelToUse);

        // Obtém o ponteiro para o canal de áudio especificado
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        // Insere cada amostra do canal no FIFO
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    // Função para preparar a estrutura para o processamento
    void prepare(int bufferSize)
    {
        // Marca a estrutura como não preparada
        prepared.set(false);

        // Define o tamanho do buffer
        size.set(bufferSize);

        // Configura o buffer para preenchimento
        bufferToFill.setSize(1,             // 1 canal
            bufferSize,    // número de amostras
            false,         // não manter conteúdo existente
            true,          // limpar espaço extra
            true);         // evitar realocação de memória

        // Prepara o FIFO para armazenar buffers completos
        audioBufferFifo.prepare(1, bufferSize);

        // Reseta o índice do FIFO
        fifoIndex = 0;

        // Marca a estrutura como preparada
        prepared.set(true);
    }

    // Função que retorna o número de buffers completos disponíveis no FIFO
    int getNumCompleteBuffersAvailable() const
    {
        return audioBufferFifo.getNumAvailableForReading();
    }

    // Função que retorna se a estrutura está preparada para uso
    bool isPrepared() const
    {
        return prepared.get();
    }

    // Função que retorna o tamanho do buffer
    int getSize() const
    {
        return size.get();
    }

    // Função que puxa um buffer completo do FIFO para processamento
    bool getAudioBuffer(BlockType& buf)
    {
        return audioBufferFifo.pull(buf);
    }

private:
    // Canal de áudio que está sendo utilizado
    Channel channelToUse;

    // Índice que rastreia a posição atual no buffer
    int fifoIndex = 0;

    // FIFO que armazena os buffers completos
    Fifo<BlockType> audioBufferFifo;

    // Buffer que está sendo preenchido com amostras
    BlockType bufferToFill;

    // Flag atômico que indica se a estrutura está preparada
    juce::Atomic<bool> prepared = false;

    // Tamanho do buffer, armazenado de forma atômica
    juce::Atomic<int> size = 0;

    // Função privada para inserir a próxima amostra no FIFO
    void pushNextSampleIntoFifo(float sample)
    {
        // Verifica se o buffer está cheio
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            // Se o buffer estiver cheio, empurra-o para o FIFO
            auto ok = audioBufferFifo.push(bufferToFill);

            // Ignora a variável 'ok' se não for usada
            juce::ignoreUnused(ok);

            // Reseta o índice para começar a preencher o próximo buffer
            fifoIndex = 0;
        }

        // Insere a amostra atual no buffer
        bufferToFill.setSample(0, fifoIndex, sample);

        // Incrementa o índice para a próxima posição
        ++fifoIndex;
    }
};

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

enum ChainPositions
{
    LowCut,
    Peak,
    HighCut
};

void updateCoefficients(juce::dsp::IIR::Filter<float>::CoefficientsPtr& old, const juce::dsp::IIR::Filter<float>::CoefficientsPtr& replacements);

juce::dsp::IIR::Filter<float>::CoefficientsPtr makePeakFilter(const ChainSettings& chainSettings, double sampleRate);

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

inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    // Desenha os coeficientes de um filtro passa-altas de Butterworth de alta ordem
    // `chainSettings.lowCutSlope` representa a inclinação desejada do filtro:
    // Slope Choice 0: 12 dB/oct -> Filtro de 2 ordem
    // Slope Choice 1: 24 dB/oct -> Filtro de 4 ordem
    // Slope Choice 2: 36 dB/oct -> Filtro de 6 ordem
    // Slope Choice 3: 48 dB/oct -> Filtro de 8 ordem
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, 2 * (chainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, sampleRate, 2 * (chainSettings.highCutSlope + 1));
}
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

    SingleChannelSampleFifo <juce::AudioBuffer<float>> leftChannelFifo{ Channel::Left };
    SingleChannelSampleFifo <juce::AudioBuffer<float>> rightChannelFifo{ Channel::Right };
private:
    // Cadeia de processamento para o canal esquerdo e direito
    juce::dsp::ProcessorChain<
        juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>, // Primeiro filtro na cadeia LowCut
        juce::dsp::IIR::Filter<float>, // Segundo filtro na cadeia LowCut
        juce::dsp::IIR::Filter<float>, // Terciero filtro na cadeia LowCut
        juce::dsp::IIR::Filter<float> // Quarto filtro na cadeia LowCut
        >, 
        juce::dsp::IIR::Filter<float>, // Filtro intermediario
        juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>, // Primeiro filtro na cadeia HighCut
        juce::dsp::IIR::Filter<float>, // Segundo filtro na cadeia HighCut
        juce::dsp::IIR::Filter<float>, // Terciero filtro na cadeia HighCut
        juce::dsp::IIR::Filter<float> // Quarto filtro na cadeia HighCut
        >
    > leftChannelChain, rightChannelChain;

    void updatePeakFilter(const ChainSettings& chainSettings);

    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);

    void updateFilters();

    juce::dsp::Oscillator<float> osc;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqualizadorAudioProcessor)
};