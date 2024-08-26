/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Enumeração que define diferentes ordens para a Transformada Rápida de Fourier (FFT).
enum FFTOrder
{
    order2048 = 11,  // FFT com 2048 pontos (2^11)
    order4096 = 12,  // FFT com 4096 pontos (2^12)
    order8192 = 13   // FFT com 8192 pontos (2^13)
};

// Estrutura template para gerar dados de FFT a partir de blocos de áudio.
template<typename BlockType>
struct FFTDataGenerator
{
    /**
     * Produz dados de FFT a partir de um buffer de áudio.
     * @param audioData Buffer de áudio a ser processado.
     * @param negativeInfinity Valor usado para representar -(infinito) em decibéis.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        // Obtém o tamanho da FFT, que é 2^order.
        const auto fftSize = getFFTSize();

        // Inicializa o vetor `fftData` com zeros, garantindo que esteja limpo antes de ser preenchido.
        fftData.assign(fftData.size(), 0);

        // Obtém o ponteiro para os dados de áudio do primeiro canal.
        auto* readIndex = audioData.getReadPointer(0);

        // Copia os dados de áudio para o vetor `fftData` até o tamanho da FFT.
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        // Aplica uma função de janela aos dados para suavizá-los e reduzir efeitos indesejados.
        window->multiplyWithWindowingTable(fftData.data(), fftSize);  // [1]

        // Realiza a Transformada Rápida de Fourier (FFT) nos dados.
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());  // [2]

        // Calcula o número de bins (frequências) que serão processados.
        int numBins = fftSize / 2;

        // Normaliza os valores da FFT dividindo-os pelo número de bins.
        for (int i = 0; i < numBins; ++i)
        {
            auto v = fftData[i];
            if (!std::isinf(v) && !std::isnan(v))
            {
                v /= float(numBins);  // Normalização para cada valor de frequência.
            }
            else
            {
                v = 0.f;  // Se o valor for infinito ou NaN, define como 0.
            }
            fftData[i] = v;
        }

        // Converte os valores normalizados para decibéis.
        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        // Insere os dados de FFT processados na fila FIFO para uso posterior.
        fftDataFifo.push(fftData);
    }

    /**
     * Muda a ordem da FFT e reconfigura os objetos associados.
     * @param newOrder A nova ordem da FFT.
     */
    void changeOrder(FFTOrder newOrder)
    {
        // Atualiza a ordem da FFT.
        order = newOrder;

        // Calcula o novo tamanho da FFT com base na nova ordem.
        auto fftSize = getFFTSize();

        // Recria o objeto FFT com a nova ordem.
        forwardFFT = std::make_unique<juce::dsp::FFT>(order);

        // Recria a função de janela com o novo tamanho da FFT.
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        // Limpa e redimensiona o vetor `fftData` para acomodar o novo tamanho da FFT.
        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        // Prepara a fila FIFO para armazenar dados FFT processados com o novo tamanho.
        fftDataFifo.prepare(fftData.size());
    }

    //==============================================================================
    /**
     * Obtém o tamanho da FFT atual.
     * @return O tamanho da FFT (número de pontos).
     */
    int getFFTSize() const { return 1 << order; }  // Calcula 2^order para obter o tamanho da FFT.

    /**
     * Obtém o número de blocos de dados FFT disponíveis na fila FIFO.
     * @return O número de blocos disponíveis.
     */
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }

    //==============================================================================
    /**
     * Extrai um bloco de dados FFT da fila FIFO.
     * @param fftData O bloco de dados FFT que será preenchido.
     * @return Verdadeiro se a operação for bem-sucedida, falso caso contrário.
     */
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }

private:
    FFTOrder order;  // A ordem atual da FFT, que determina o tamanho da FFT.
    BlockType fftData;  // Vetor para armazenar os dados de FFT processados.
    std::unique_ptr<juce::dsp::FFT> forwardFFT;  // Objeto para realizar a FFT.
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;  // Função de janela para suavizar os dados antes da FFT.

    // Fila FIFO que armazena blocos de dados FFT prontos para uso posterior.
    Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
        juce::Rectangle<float> fftBounds,
        int fftSize,
        float binWidth,
        float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
            {
                return juce::jmap(v,
                    negativeInfinity, 0.f,
                    float(bottom + 10), top);
            };

        auto y = map(renderData[0]);

        //        jassert( !std::isnan(y) && !std::isinf(y) );
        if (std::isnan(y) || std::isinf(y))
            y = bottom;

        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

        for (int binNum = 1; binNum < numBins; binNum += pathResolution)
        {
            y = map(renderData[binNum]);

            //            jassert( !std::isnan(y) && !std::isinf(y) );

            if (!std::isnan(y) && !std::isinf(y))
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

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
    struct LabelPos
    {
        float pos;
        juce::String label;
    };
    juce::Array<LabelPos> labels;
};

struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<juce::AudioBuffer<float>>& scsf) :
        leftChannelFifo(&scsf)
    {
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    }
    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() { return leftChannelFFTPath; }
private:
    SingleChannelSampleFifo<juce::AudioBuffer<float>>* leftChannelFifo;
    juce::AudioBuffer<float> monoBuffer;

    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;

    AnalyzerPathGenerator<juce::Path> pathProducer;

    juce::Path leftChannelFFTPath;
};

struct ResponseCurveComponent : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer
{
    ResponseCurveComponent(EqualizadorAudioProcessor&);
    ~ResponseCurveComponent();
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}

    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
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

    void updateChain();

    juce::Image background;

    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysisArea();

    PathProducer leftPathProducer, rightPathProducer;
};

//==============================================================================
/**
*/
class EqualizadorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    EqualizadorAudioProcessorEditor(EqualizadorAudioProcessor&);
    ~EqualizadorAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizadorAudioProcessorEditor)
};