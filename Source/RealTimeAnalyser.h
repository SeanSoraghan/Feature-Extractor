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
        audioAnalyserSpec (/*state.currentSamplesPerBlock*/128, 
                           1, 
                           sampleRate / 2.0,
                           true,
                           false),
        audioAnalyserHarm (128 * 8,
                           1,
                           sampleRate / 2.0,
                           false,
                           true),
        spectralFeatures (1,
                          0,
                          1,
                          audioAnalyserSpec.fftOut.getNumSamples() - 1,
                          0.0,
                          sampleRate),
        harmonicFeatures (1,
                          0,
                          1,
                          audioAnalyserHarm.fftOut.getNumSamples() - 1,
                          0.0,
                          sampleRate)
    {}

    void timerCallback() override
    {
        //setSpectralFeatureAudioBuffer();
        setFeatureAnalysisAudioBuffer (spectralFeatures, audioAnalyserSpec);
        extractFeatures               (spectralFeatures, audioAnalyserSpec);

        setFeatureAnalysisAudioBuffer (harmonicFeatures, audioAnalyserHarm);
        extractFeatures               (harmonicFeatures, audioAnalyserHarm);
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

    //void setSpectralFeatureAudioBuffer()
    //{
    //    int numSamplesInAnalysisWindow   = audioAnalyserSpec.fftIn.getNumSamples();
    //    AudioSampleBuffer audioWindow = audioDataCollector.getAnalysisBuffer (numSamplesInAnalysisWindow);
    //    
    //    int numChannels = audioWindow.getNumChannels();

    //    int numSamplesInCollectedBuffer  = audioWindow.getNumSamples();

    //    spectralFeatures.audioOutput.setSize (numChannels, 
    //                                          numSamplesInCollectedBuffer, false, true);        
    //    spectralFeatures.audioOutput.clear();
    //    for (int c = 0; c < numChannels; c++)
    //    {
    //        spectralFeatures.audioOutput.copyFrom (c, 0, audioWindow, c, 0, numSamplesInCollectedBuffer);
    //    }
    //}

    //void setHarmonicFeatureAudioBuffer()
    //{
    //    int numSamplesInAnalysisWindow   = audioAnalyserHarm.fftIn.getNumSamples();
    //    AudioSampleBuffer audioWindow = audioDataCollector.getAnalysisBuffer (numSamplesInAnalysisWindow);
    //    
    //    int numChannels = audioWindow.getNumChannels();

    //    int numSamplesInCollectedBuffer  = audioWindow.getNumSamples();

    //    harmonicFeatures.audioOutput.setSize (numChannels, 
    //                                          numSamplesInCollectedBuffer, false, true);        
    //    harmonicFeatures.audioOutput.clear();
    //    for (int c = 0; c < numChannels; c++)
    //    {
    //        harmonicFeatures.audioOutput.copyFrom (c, 0, audioWindow, c, 0, numSamplesInCollectedBuffer);
    //    }
    //}

    void setFeatureAnalysisAudioBuffer (ConcatenatedFeatureBuffer& featureBuffer, 
                                        AudioAnalyser& audioAnalyser)
    {
        int numSamplesInAnalysisWindow   = audioAnalyser.fftIn.getNumSamples();
        AudioSampleBuffer audioWindow    = audioDataCollector.getAnalysisBuffer (numSamplesInAnalysisWindow);
        
        int numChannels = audioWindow.getNumChannels();

        int numSamplesInCollectedBuffer  = audioWindow.getNumSamples();

        featureBuffer.audioOutput.setSize (numChannels, 
                                              numSamplesInCollectedBuffer, false, true);        
        featureBuffer.audioOutput.clear();
        for (int c = 0; c < numChannels; c++)
        {
            featureBuffer.audioOutput.copyFrom (c, 0, audioWindow, c, 0, numSamplesInCollectedBuffer);
        }
    }

    void extractFeatures (ConcatenatedFeatureBuffer& features, AudioAnalyser& analyser)
    {
        if (features.audioOutput.getNumSamples() > 0)
            analyser.performSpectralAnalysis (features);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeAnalyser)
};




#endif  // REALTIMEANALYSER_H_INCLUDED
