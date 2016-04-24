/*
  ==============================================================================

    PitchEstimationVisualiser.h
    Created: 18 Apr 2016 6:12:01pm
    Author:  Sean

  ==============================================================================
*/

#ifndef PITCHESTIMATIONVISUALISER_H_INCLUDED
#define PITCHESTIMATIONVISUALISER_H_INCLUDED

class BufferVisualiser : public  Component, 
                         private Timer
{
public:
    BufferVisualiser    (bool bipolar = true, bool containsPeak = false)
    :   bufferIsBipolar (bipolar),
        bufferHasPeak   (containsPeak)
    {
        bufferCopy.clear();
        startTimerHz (FeatureExtractorLookAndFeel::getAnimationRateHz());
    }

    void paint (Graphics& g) override
    {
        FeatureExtractorLookAndFeel::drawBipolarBuffer (g, getLocalBounds().toFloat(), bufferCopy, bufferCopy.getNumSamples());
        
        if (bufferHasPeak)
            FeatureExtractorLookAndFeel::drawPeakPoint (g, getLocalBounds().toFloat(), peakPosition);
    }
    
    void timerCallback() override
    {
        if (getBuffer != nullptr)
        {
            bufferCopy = AudioSampleBuffer (getBuffer());
            if (tagBufferForUpdate != nullptr)
                tagBufferForUpdate();
        }

        if (bufferHasPeak && getPeakPosition != nullptr)
            peakPosition = getPeakPosition();

        repaint();
    }

    void setGetBufferCallback          (std::function<AudioSampleBuffer()> f) { getBuffer = f; }
    void setTagBufferForUpdateCallback (std::function<void()> f)              { tagBufferForUpdate = f; }
    void setGetPeakPositionCallback    (std::function<Point<float>()> f)      { getPeakPosition = f; }
private:
    AudioSampleBuffer                  bufferCopy;
    std::function<AudioSampleBuffer()> getBuffer;
    std::function<void()>              tagBufferForUpdate;
    std::function<Point<float>()>      getPeakPosition;
    Point<float>                       peakPosition {0.0f, 0.0f};
    bool                               bufferIsBipolar;
    bool                               bufferHasPeak { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BufferVisualiser)
};


//======================================================================================================================================
//======================================================================================================================================

class PitchEstimationVisualiser : public Component
{
public:
    PitchEstimationVisualiser() 
    :   cumulativeDifferenceBufferVisualiser (true, true)
    {
        addAndMakeVisible (overlappedBufferVisualiser);
        addAndMakeVisible (fftBufferVisualiser);
        addAndMakeVisible (autoCorrelationBufferVisualiser);
        addAndMakeVisible (cumulativeDifferenceBufferVisualiser);
    }

    void paintOverChildren (Graphics& g)
    {
        if (getPitchEstimate != nullptr)
        {
            const auto visBounds = getLocalBounds().withSizeKeepingCentre (2, (int) (getHeight() * 0.8f));
            FeatureExtractorLookAndFeel::paintFeatureVisualiser (g, getPitchEstimate() / maxFrequency, visBounds);
        }
    }

    void resized() override
    {
        auto localBounds = getLocalBounds().toFloat();
        const auto bufferVisualiserHeight = localBounds.getHeight() / (float) numVisualisers;
        overlappedBufferVisualiser.setBounds (localBounds.removeFromTop (bufferVisualiserHeight).getSmallestIntegerContainer());
        fftBufferVisualiser.setBounds (localBounds.removeFromTop (bufferVisualiserHeight).getSmallestIntegerContainer());
        autoCorrelationBufferVisualiser.setBounds (localBounds.removeFromTop (bufferVisualiserHeight).getSmallestIntegerContainer());
        cumulativeDifferenceBufferVisualiser.setBounds (localBounds.removeFromTop (bufferVisualiserHeight).getSmallestIntegerContainer());
    }

    BufferVisualiser& getOverlappedBufferVisualiser()           { return overlappedBufferVisualiser; }
    BufferVisualiser& getFFTBufferVisualiser()                  { return fftBufferVisualiser; }
    BufferVisualiser& getAutoCorrelationBufferVisualiser()      { return autoCorrelationBufferVisualiser; }
    BufferVisualiser& getCumulativeDifferenceBufferVisualiser() { return cumulativeDifferenceBufferVisualiser; }
    void setGetPitchEstimateCallback (std::function<float()> f) { getPitchEstimate = f; }

private:
    BufferVisualiser overlappedBufferVisualiser;
    BufferVisualiser fftBufferVisualiser;
    BufferVisualiser autoCorrelationBufferVisualiser;
    BufferVisualiser cumulativeDifferenceBufferVisualiser;
    std::function<float()> getPitchEstimate;
    const float maxFrequency { 12000.0f };
    const int numVisualisers { 4 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchEstimationVisualiser)
};



#endif  // PITCHESTIMATIONVISUALISER_H_INCLUDED
