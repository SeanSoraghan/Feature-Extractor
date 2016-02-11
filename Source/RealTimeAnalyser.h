/*
  ==============================================================================

    RealTimeAnalyser.h
    Created: 23 Oct 2015 10:02:59am
    Author:  Sean

  ==============================================================================
*/

#ifndef REALTIMEANALYSER_H_INCLUDED
#define REALTIMEANALYSER_H_INCLUDED

typedef ConcatenatedFeatureBuffer::Feature Feature;

class RealTimeAnalyser : private Timer
{
public:
    RealTimeAnalyser (AudioDataCollector& adc, double sampleRate = 48000.0)
    :   audioDataCollector (adc),
        audioAnalyserSpec (/*state.currentSamplesPerBlock*/256, 
                           2, 
                           sampleRate / 2.0,
                           true,
                           false),
        audioAnalyserHarm (256 * 8,
                           2,
                           sampleRate / 2.0,
                           false,
                           true),
        spectralFeatures (2,
                          0,
                          1,
                          audioAnalyserSpec.fftOut.getNumSamples() - 1,
                          0.0,
                          sampleRate),
        harmonicFeatures (2,
                          0,
                          1,
                          audioAnalyserHarm.fftOut.getNumSamples() - 1,
                          0.0,
                          sampleRate)
    {}

    void timerCallback() override
    {
        setSpectralFeatureAudioBuffer();
        setHarmonicFeatureAudioBuffer();
        extractFeatures();
        
        //owner.paintingBufferNeedsUpdating = 1;
        //owner.pitchAnalysisBufferNeedsUpdating = 1;
    }

    void sampleRateChanged (double newSampleRate)
    {
        spectralFeatures.nyquistFrequency = newSampleRate / 2.0;
        spectralFeatures.nyquistFrequency = newSampleRate / 2.0;

        stopTimer();
        startTimerHz (FeatureExtractorLookAndFeel::getAnimationRateHz());
    }

    ConcatenatedFeatureBuffer& getSpectralFeatures()
    {
        return spectralFeatures;
    }

    ConcatenatedFeatureBuffer& getHarmonicFeatures()
    {
        return harmonicFeatures;
    }

private:
    AudioDataCollector&       audioDataCollector;
    AudioAnalyser             audioAnalyserSpec;
    AudioAnalyser             audioAnalyserHarm;
    ConcatenatedFeatureBuffer spectralFeatures;
    ConcatenatedFeatureBuffer harmonicFeatures;

    void setSpectralFeatureAudioBuffer()
    {
        AudioSampleBuffer audioWindow = audioDataCollector.getSpectralAnalysisBuffer ();
        int numChannels = audioWindow.getNumChannels();

        int numSamplesInCollectedBuffer  = audioWindow.getNumSamples();
        int numSamplesInAnalysisWindow   = audioAnalyserSpec.fftIn.getNumSamples();
        int numSamplesLeftOver           = numSamplesInCollectedBuffer - numSamplesInAnalysisWindow;
        spectralFeatures.audioOutput.setSize (numChannels, 
                                              numSamplesInCollectedBuffer);        
        spectralFeatures.audioOutput.clear();
        for (int c = 0; c < numChannels; c++)
        {
            spectralFeatures.audioOutput.copyFrom (c, 0, audioWindow, c, 0, numSamplesInAnalysisWindow);
        }

        if (numSamplesLeftOver < numSamplesInAnalysisWindow)
            audioDataCollector.spectralAnalysisBufferNeedsUpdating.set (1);
    }

    void setHarmonicFeatureAudioBuffer()
    {
        AudioSampleBuffer audioWindow = audioDataCollector.getHarmonicAnalysisBuffer();
        int numChannels = audioWindow.getNumChannels();
        int numSamples  = audioWindow.getNumSamples();
        harmonicFeatures.audioOutput.setSize (numChannels, 
                                              numSamples);        
        harmonicFeatures.audioOutput.clear();
        for (int c = 0; c < numChannels; c++)
        {
            harmonicFeatures.audioOutput.copyFrom (c, 0, audioWindow, c, 0, numSamples);
        }
    }

    void extractFeatures()
    {
        if (spectralFeatures.audioOutput.getNumSamples() > 0)
        {
            audioAnalyserSpec.performSpectralAnalysis (spectralFeatures);
            //audioAnalyser.analyseNormalisedZeroCrosses (features);
            spectralFeatures.normaliseAudioChannels();
        }

        if (harmonicFeatures.audioOutput.getNumSamples() > 0)
        {
            audioAnalyserHarm.performSpectralAnalysis (harmonicFeatures);
            harmonicFeatures.normaliseAudioChannels();
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeAnalyser)
};




#endif  // REALTIMEANALYSER_H_INCLUDED
