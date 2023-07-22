#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics &g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider & slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);
    g.setColour(Colour(97u, 18u, 167u));
    g.fillEllipse(bounds);

    g.setColour(Colour(255u, 154u, 1u));
    g.drawEllipse(bounds, 1.f);

    // If we can cast from a slider to RotarySliderWithLabels then we can
    // call the RotarySliderWithLabels methods.
    if( auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;
        // Plot rotary slider in square
        Rectangle<float> r;

        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);
        // Add and rotate indicator line
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);
        // Create bounding box for label
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(center);
        g.setColour(Colours::black);
        // g.fillRect(r);
        // Display slider value
        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}
//======================================================================
void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    // g.setColour(Colours::red);
    // g.drawRect(getLocalBounds());
    // g.setColour(Colours::yellow);
    // g.drawRect(sliderBounds);

    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng,
                                      *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.toFloat().getHeight() / 2.f;
    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());
    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; i++)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5 + 1,
                                       ang);

    Rectangle<float> r;
    auto str = labels[i].label;
    r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
    r.setCentre(c);
    r.setY(r.getY() + getTextHeight());
    g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);
    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    // Return choice name for choice sliders
    if( auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param) )
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    // Whether to add K for kHz
    bool addK = false;
    // Return string for float parameters
    if( auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param) )
    {
        float val = getValue();
        if ( val > 999.f )
        {
            val /= 1000.f;
            addK = true;
        }
        // 2 decimal places for kHz otherwise default formwatting
        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse; // Param not of type AudioParameterChoice or AudioParameterFloat
    }
    if ( suffix.isNotEmpty() )
    {
        str << " ";
        if ( addK )
        {
            str << "k";
        }
        str << suffix;
    }
    return str;
}

//======================================================================
ResponseCurveComponent::ResponseCurveComponent(AudioPluginAudioProcessor& p) : processorRef(p)
{
    const auto& params = processorRef.getParameters();
    for ( auto param : params )
    {
        param->addListener(this);
    }

    startTimerHz(60);

    setSize (600, 400);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = processorRef.getParameters();
    for ( auto param : params )
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
    if( parametersChanged.compareAndSetBool(false, true) )
    {
        // Update the monochain coefficients to match apvts
        auto chainSettings = getChainSettings(processorRef.apvts);

        auto peakCoefficients = makePeakFilter(chainSettings, processorRef.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

        auto lowCutCoefficients = makeLowCutFilter(chainSettings, processorRef.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(),
                        lowCutCoefficients, chainSettings.lowCutSlope);

        auto highCutCoefficients = makeHighCutFilter(chainSettings, processorRef.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(),
                        highCutCoefficients, chainSettings.highCutSlope);
        // Signal a repaint
        repaint();
    }

}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

    auto responseArea = getLocalBounds();
    // auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

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


//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      peakFreqSlider(*p.apvts.getParameter("Peak Freq"), "Hz"),
      peakGainSlider(*p.apvts.getParameter("Peak Gain"), "dB"),
      peakQualitySlider(*p.apvts.getParameter("Peak Quality"), ""),
      lowCutFreqSlider(*p.apvts.getParameter("LowCut Freq"), "Hz"),
      highCutFreqSlider(*p.apvts.getParameter("HighCut Freq"), "Hz"),
      lowCutSlopeSlider(*p.apvts.getParameter("LowCut Slope"), "dB/Oct"),
      highCutSlopeSlider(*p.apvts.getParameter("HighCut Slope"), "dB/Oct"),
      responseCurveComponent(p),
      peakFreqSliderAttachment(p.apvts, "Peak Freq", peakFreqSlider),
      peakGainSliderAttachment(p.apvts, "Peak Gain", peakGainSlider),
      peakQualitySliderAttachment(p.apvts, "Peak Quality", peakQualitySlider),
      lowCutFreqSliderAttachment(p.apvts, "LowCut Freq", lowCutFreqSlider),
      highCutFreqSliderAttachment(p.apvts, "HighCut Freq", highCutFreqSlider),
      lowCutSlopeSliderAttachment(p.apvts, "LowCut Slope", lowCutSlopeSlider),
      highCutSlopeSliderAttachment(p.apvts, "HighCut Slope", highCutSlopeSlider)
{
      // Add labels for max and min values
      peakFreqSlider.labels.add({0.f, "20Hz"});
      peakFreqSlider.labels.add({1.f, "20kHz"});

      peakGainSlider.labels.add({0.f, "-24dB"});
      peakGainSlider.labels.add({1.f, "24dB"});

      peakQualitySlider.labels.add({0.f, "0.1"});
      peakQualitySlider.labels.add({1.f, "10.0"});

      lowCutFreqSlider.labels.add({0.f, "20Hz"});
      lowCutFreqSlider.labels.add({1.f, "20kHz"});

      highCutFreqSlider.labels.add({0.f, "20Hz"});
      highCutFreqSlider.labels.add({1.f, "20kHz"});

      lowCutSlopeSlider.labels.add({0.f, "12"});
      lowCutSlopeSlider.labels.add({1.f, "48"});

      highCutSlopeSlider.labels.add({0.f, "12"});
      highCutSlopeSlider.labels.add({1.f, "48"});

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
}

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    responseCurveComponent.setBounds(responseArea);

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

std::vector<juce::Component*> AudioPluginAudioProcessorEditor::getComps()
{
    return
    {
        &responseCurveComponent,
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider
    };
}
