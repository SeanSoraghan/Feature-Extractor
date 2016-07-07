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
        bufferIsBipolar ? FeatureExtractorLookAndFeel::drawBipolarBuffer (g, getLocalBounds().toFloat(), bufferCopy)
                        : FeatureExtractorLookAndFeel::drawBuffer        (g, getLocalBounds().toFloat(), bufferCopy);
        
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
    bool                               bufferIsBipolar   { true };
    bool                               bufferHasPeak     { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BufferVisualiser)
};


//======================================================================================================================================
//======================================================================================================================================

class PitchEstimationVisualiser : public Component
{
public:
    PitchEstimationVisualiser() 
    :   autoCorrelationBufferVisualiser      (false),
        cumulativeDifferenceBufferVisualiser (false, true),
        fftMagnitudesVisualiser              (false)
    {
        addAndMakeVisible (overlappedBufferVisualiser);
        addAndMakeVisible (fftBufferVisualiser);
        addAndMakeVisible (autoCorrelationBufferVisualiser);
        addAndMakeVisible (cumulativeDifferenceBufferVisualiser);
        addAndMakeVisible (fftMagnitudesVisualiser);
    }

    void paintOverChildren (Graphics& g)
    {
        if (getPitchEstimate != nullptr)
        {
            const auto visBounds = getLocalBounds().withSizeKeepingCentre (2, (int) (getHeight() * 0.8f));
            FeatureExtractorLookAndFeel::paintFeatureVisualiser (g, getPitchEstimate(), visBounds);
        }
    }

    void resized() override
    {
        auto localBounds = getLocalBounds().toFloat();

        const auto bufferVisualiserHeight            = localBounds.getHeight() / (float) numVisualisers;
        const auto halfBufferWidth                   = localBounds.getWidth() / 2.0f;
        overlappedBufferVisualiser.setBounds           (localBounds.removeFromTop (bufferVisualiserHeight).getSmallestIntegerContainer());
        fftBufferVisualiser.setBounds                  (localBounds.removeFromTop (bufferVisualiserHeight).getSmallestIntegerContainer());
        auto autoCorrelationAndFFTMagBounds          = localBounds.removeFromTop (bufferVisualiserHeight);
        autoCorrelationBufferVisualiser.setBounds      (autoCorrelationAndFFTMagBounds.removeFromLeft (halfBufferWidth).getSmallestIntegerContainer());
        fftMagnitudesVisualiser.setBounds              (autoCorrelationAndFFTMagBounds.removeFromLeft (halfBufferWidth).getSmallestIntegerContainer());
        cumulativeDifferenceBufferVisualiser.setBounds (localBounds.removeFromTop (bufferVisualiserHeight).removeFromLeft (halfBufferWidth).getSmallestIntegerContainer());
    }

    void clearCallbacks()
    {
        overlappedBufferVisualiser.setGetBufferCallback                    (nullptr);
        overlappedBufferVisualiser.setTagBufferForUpdateCallback           (nullptr);
        overlappedBufferVisualiser.setGetPeakPositionCallback              (nullptr);
        fftBufferVisualiser.setGetBufferCallback                           (nullptr);
        fftBufferVisualiser.setTagBufferForUpdateCallback                  (nullptr);
        fftBufferVisualiser.setGetPeakPositionCallback                     (nullptr);
        autoCorrelationBufferVisualiser.setGetBufferCallback               (nullptr);
        autoCorrelationBufferVisualiser.setTagBufferForUpdateCallback      (nullptr);
        autoCorrelationBufferVisualiser.setGetPeakPositionCallback         (nullptr);
        cumulativeDifferenceBufferVisualiser.setGetBufferCallback          (nullptr);
        cumulativeDifferenceBufferVisualiser.setTagBufferForUpdateCallback (nullptr);
        cumulativeDifferenceBufferVisualiser.setGetPeakPositionCallback    (nullptr);
        fftMagnitudesVisualiser.setGetBufferCallback                       (nullptr);
        fftMagnitudesVisualiser.setTagBufferForUpdateCallback              (nullptr);
        fftMagnitudesVisualiser.setGetPeakPositionCallback                 (nullptr);
    }

    BufferVisualiser& getOverlappedBufferVisualiser()           { return overlappedBufferVisualiser; }
    BufferVisualiser& getFFTBufferVisualiser()                  { return fftBufferVisualiser; }
    BufferVisualiser& getAutoCorrelationBufferVisualiser()      { return autoCorrelationBufferVisualiser; }
    BufferVisualiser& getCumulativeDifferenceBufferVisualiser() { return cumulativeDifferenceBufferVisualiser; }
    BufferVisualiser& getFFTMagnitudesVisualiser()              { return fftMagnitudesVisualiser; }
    void setGetPitchEstimateCallback (std::function<float()> f) { getPitchEstimate = f; }

private:
    BufferVisualiser overlappedBufferVisualiser;
    BufferVisualiser fftBufferVisualiser;
    BufferVisualiser autoCorrelationBufferVisualiser;
    BufferVisualiser cumulativeDifferenceBufferVisualiser;
    BufferVisualiser fftMagnitudesVisualiser;
    std::function<float()> getPitchEstimate;
    const int numVisualisers { 4 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchEstimationVisualiser)
};



#endif  // PITCHESTIMATIONVISUALISER_H_INCLUDED
