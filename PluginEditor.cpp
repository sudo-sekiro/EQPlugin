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

void LookAndFeel::drawToggleButton(juce::Graphics& g,
                                   juce::ToggleButton& toggleButton,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
    using namespace juce;

    if (auto* pb = dynamic_cast<PowerButton*>(&toggleButton))
    {
        Path powerButton;
        auto bounds = toggleButton.getLocalBounds();
        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

        auto ang = 30.f;
        size -= 6;

        powerButton.addCentredArc(r.getCentreX(),
                                r.getCentreY(),
                                size * 0.5,
                                size * 0.5,
                                0.f,
                                degreesToRadians(ang),
                                degreesToRadians(360.f -  ang),
                                true);

        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());

        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
        auto colour = toggleButton.getToggleState() ?  Colours::dimgrey : Colour(0u, 172u, 1u);

        g.setColour(colour);
        g.strokePath(powerButton, pst);
        g.drawEllipse(r, 2.f);
    }
    else if (auto* analyserButton = dynamic_cast<AnalyserButton*>(&toggleButton))
    {
        auto colour = ! toggleButton.getToggleState() ?  Colours::dimgrey : Colour(0u, 172u, 1u);
        g.setColour(colour);

        auto bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);

        g.strokePath(analyserButton->randomPath, PathStrokeType(1.f));
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
ResponseCurveComponent::ResponseCurveComponent(AudioPluginAudioProcessor& p) :
    processorRef(p),
    leftPathProducer(p.leftChannelFifo),
    rightPathProducer(p.rightChannelFifo)
{
    const auto& params = processorRef.getParameters();
    for ( auto param : params )
    {
        param->addListener(this);
    }

    updateChain();

    startTimerHz(60);

    setSize (600, 480);
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

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    while( singleChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if( singleChannelFifo->getAudioBuffer(tempIncomingBuffer) )
        {
            auto size = tempIncomingBuffer.getNumSamples();
            // Shift data forward by buffer size.
            // Remove `size` number of samples from the left hand side
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, size),
                                              monoBuffer.getNumSamples() - size);

            // Copy size number of samples from the incoming buffer to the end (Right hand side) of the monoBuffer
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              size);

            singleChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }

    /* If there are fftData buffers to pull
            If we can pull a buffer
                Produce a path
     */
    // const auto fftBounds = getAnalysisArea().toFloat();
    const auto fftSize = singleChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / (double)fftSize;

    while( singleChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0 )
    {
        std::vector<float> fftData;
        if( singleChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }

    /* While there are paths to be pulled
        Pull as many as possible
            Only display most recent
     */
    while(pathProducer.getNumPathsAvailable() > 0)
    {
        pathProducer.getPath(singleChannelFFTPath);
    }
}

void ResponseCurveComponent::timerCallback()
{
    auto fftBounds = getAnalysisArea().toFloat();
    auto sampleRate = processorRef.getSampleRate();
    leftPathProducer.process(fftBounds, sampleRate);
    rightPathProducer.process(fftBounds, sampleRate);

    if( parametersChanged.compareAndSetBool(false, true) )
    {
        // Update the monochain coefficients
        updateChain();
    }
    // Signal a repaint
    repaint();

}

void ResponseCurveComponent::updateChain()
{
    // Update the monochain coefficients to match apvts
    auto chainSettings = getChainSettings(processorRef.apvts);

    // Do not draw components when they are bypassed
    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);

    auto peakCoefficients = makePeakFilter(chainSettings, processorRef.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, processorRef.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(),
                    lowCutCoefficients, chainSettings.lowCutSlope);

    auto highCutCoefficients = makeHighCutFilter(chainSettings, processorRef.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(),
                    highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

    auto responseArea = getAnalysisArea();
    // Draw background grid image for plotting frequencies
    g.drawImage(background, getLocalBounds().toFloat());

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

        if (! monoChain.isBypassed<ChainPositions::Peak>() )
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!monoChain.isBypassed<ChainPositions::LowCut>() )
        {
            if (!lowCut.isBypassed<0>() )
                mag *= lowCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowCut.isBypassed<1>() )
                mag *= lowCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowCut.isBypassed<2>() )
                mag *= lowCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowCut.isBypassed<3>() )
                mag *= lowCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!monoChain.isBypassed<ChainPositions::HighCut>() )
        {
            if (!highCut.isBypassed<0>() )
                mag *= highCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highCut.isBypassed<1>() )
                mag *= highCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highCut.isBypassed<2>() )
                mag *= highCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highCut.isBypassed<3>() )
                mag *= highCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

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
    auto leftChannelFFTPath = leftPathProducer.getPath();
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY() ));

    g.setColour(Colours::skyblue);
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY() ));

    g.setColour(Colours::lightyellow);
    g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));

}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();

    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);

    return bounds;
}
void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);

    Graphics g(background);

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    Array<float> freqs
    {
        20, 30, 50, 100,
        200, 300, 500, 1000,
        2000, 3000, 5000, 10000,
        20000
    };
    g.setColour(Colours::dimgrey);

    Array<float> xs;
    for (auto f : freqs )
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }

    for (auto x : xs )
    {
        g.drawVerticalLine(float(x), float(top), float(bottom));
    }


    Array<float> gain
    {
        -24, -12, 0, 12, 24
    };
    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::dimgrey );
        g.drawHorizontalLine(int(y), left, right);
    }

    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);

    // Draw frequency labels
    for (int i = 0; i< freqs.size(); i++)
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if ( f > 999.f )
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if (addK)
        {
            str << "k";
        }
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    // Draw gain labels
    for (auto gDb : gain)
    {
        // Plot frequency labels for filter chain
        String str;
        auto y  = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        str << gDb;
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth - 2);
        r.setCentre(r.getCentreX(), y);
        g.drawFittedText(str, r, juce::Justification::centred, 1);

        // Plot frequency labels for spectrum analyser
        str.clear();
        str << (gDb - 24.f);
        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
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
      highCutSlopeSliderAttachment(p.apvts, "HighCut Slope", highCutSlopeSlider),
      lowCutBypassButtonAttachment(p.apvts, "LowCut Bypassed", lowCutBypassButton),
      highCutBypassButtonAttachment(p.apvts, "HighCut Bypassed", highCutBypassButton),
      peakBypassButtonAttachment(p.apvts, "Peak Bypassed", peakBypassButton),
      analyserBypassButtonAttachment(p.apvts, "Analyser Bypassed", analyserBypassButton)
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

    peakBypassButton.setLookAndFeel(&lnf);
    lowCutBypassButton.setLookAndFeel(&lnf);
    highCutBypassButton.setLookAndFeel(&lnf);
    analyserBypassButton.setLookAndFeel(&lnf);

    setSize (600, 400);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    peakBypassButton.setLookAndFeel(nullptr);
    lowCutBypassButton.setLookAndFeel(nullptr);
    highCutBypassButton.setLookAndFeel(nullptr);
    analyserBypassButton.setLookAndFeel(nullptr);
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

    auto analyserEnabledArea = bounds.removeFromTop(25);
    analyserEnabledArea.setWidth(100);
    analyserEnabledArea.setX(5);
    analyserEnabledArea.removeFromTop(2);

    analyserBypassButton.setBounds(analyserEnabledArea);

    bounds.removeFromTop(5);

    float hRatio = 33 / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(5);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto HighCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

    lowCutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.66));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highCutBypassButton.setBounds(HighCutArea.removeFromTop(25));
    highCutFreqSlider.setBounds(HighCutArea.removeFromTop(HighCutArea.getHeight() * 0.66));
    highCutSlopeSlider.setBounds(HighCutArea);

    peakBypassButton.setBounds(bounds.removeFromTop(25));
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
        &highCutSlopeSlider,
        &lowCutBypassButton,
        &highCutBypassButton,
        &peakBypassButton,
        &analyserBypassButton
    };
}
