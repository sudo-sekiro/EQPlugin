#pragma once

#include "PluginProcessor.h"

#define JUCE_LIVE_CONSTANT

enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template <typename BlockType>
struct FFTDataGenerator
{
    // Produces FFT data from a sample buffer
    void produceFFTDataForRendering(const juce::AudioBuffer<float> &audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();

        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        // First apply a windowing function to our data
        window->multiplyWithWindowingTable(fftData.data(), fftSize);     // [1]

        // Render our FFT data
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data()); // [2]

        int numBins = (int)fftSize / 2;

        // Normalise the FFT values
        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] /= (float) numBins;
        }

        // Convert to decibels
        for(int i = 0; i < numBins; ++i)
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder)
    {
        // When you change order, recreate the window, forwardFFT, fifo, fftData
        // Also reset the fifo index
        // Things that need recreating should be created in the heap via std::make_unique<>
        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==================================================================
    int getFFTSize() const { return 1 << order; }
    // See how much FFT data is available
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==================================================================
    // Return FFT data to the fftData buffer
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

struct LookAndFeel : juce::LookAndFeel_V4
{
            virtual void drawRotarySlider (juce::Graphics& g,
                                       int x, int y, int width, int height,
                                       float sliderPosProportional,
                                       float rotaryStartAngle,
                                       float rotaryEndAngle,
                                       juce::Slider& slider) override;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                     juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap),
        suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }
    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }

    struct LabelPos
    {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const {return 14; }
    juce::String getDisplayString() const;
private:
    LookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct ResponseCurveComponent : juce::Component,
                       juce::AudioProcessorParameter::Listener,
                       juce::Timer
{
    ResponseCurveComponent(AudioPluginAudioProcessor&);
    ~ResponseCurveComponent();

    void parameterValueChanged (int parameterIndex, float newValue) override;

    /** Indicates that a parameter change gesture has started.

        E.g. if the user is dragging a slider, this would be called with gestureIsStarting
        being true when they first press the mouse button, and it will be called again with
        gestureIsStarting being false when they release it.

        IMPORTANT NOTE: This will be called synchronously, and many audio processors will
        call it during their audio callback. This means that not only has your handler code
        got to be completely thread-safe, but it's also got to be VERY fast, and avoid
        blocking. If you need to handle this event on your message thread, use this callback
        to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
        message thread.
    */
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {} ;

    void timerCallback() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    private:
        AudioPluginAudioProcessor& processorRef;
        juce::Atomic<bool> parametersChanged { false };
        MonoChain monoChain;

        void updateChain();

        juce::Image background;

        juce::Rectangle<int> getRenderArea();

        juce::Rectangle<int> getAnalysisArea();
};

//==============================================================================
class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    RotarySliderWithLabels peakFreqSlider,
                       peakGainSlider,
                       peakQualitySlider,
                       lowCutFreqSlider,
                       highCutFreqSlider,
                       lowCutSlopeSlider,
                       highCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment,
               peakGainSliderAttachment,
               peakQualitySliderAttachment,
               lowCutFreqSliderAttachment,
               highCutFreqSliderAttachment,
               lowCutSlopeSliderAttachment,
               highCutSlopeSliderAttachment;

    std::vector<juce::Component*> getComps();

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
