#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      peakFreqSliderAttachment(p.apvts, "Peak Freq", peakFreqSlider),
      peakGainSliderAttachment(p.apvts, "Peak Gain", peakGainSlider),
      peakQualitySliderAttachment(p.apvts, "Peak Quality", peakQualitySlider),
      lowCutFreqSliderAttachment(p.apvts, "LowCut Freq", lowCutFreqSlider),
      highCutFreqSliderAttachment(p.apvts, "HighCut Freq", highCutFreqSlider),
      lowCutSlopeSliderAttachment(p.apvts, "LowCut Slope", lowCutSlopeSlider),
      highCutSlopeSliderAttachment(p.apvts, "HighCut Slope", highCutSlopeSlider)
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    setSize (600, 400);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    auto w = responseArea.getWidth();

    auto& lowCut = monoChain.get<ChainPositions::LowCut>();
    auto& highCut = monoChain.get<ChainPositions::HighCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();

    auto sampleRate = processorRef.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);
    for (int i =0; i< w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak>() )
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!lowCut.isBypassed<0>() )
            mag *= lowCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowCut.isBypassed<1>() )
            mag *= lowCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowCut.isBypassed<2>() )
            mag *= lowCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowCut.isBypassed<3>() )
            mag *= lowCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highCut.isBypassed<0>() )
            mag *= highCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highCut.isBypassed<1>() )
            mag *= highCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highCut.isBypassed<2>() )
            mag *= highCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highCut.isBypassed<3>() )
            mag *= highCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); i++)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));

}

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto HighCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.66));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(HighCutArea.removeFromTop(HighCutArea.getHeight() * 0.66));
    highCutSlopeSlider.setBounds(HighCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33f));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5f));
    peakQualitySlider.setBounds(bounds);
}

void AudioPluginAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    if( parametersChanged.compareAndSetBool(false, true) )
    {
        // Update teh monochain from apvts
        // Signal a repaint
    }

}

std::vector<juce::Component*> AudioPluginAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider
    };
}
