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

class RealTimeAnalyser : public Thread
{
public:
    RealTimeAnalyser (AudioDataCollector& adc, double sampleRate = 48000.0)
    :   Thread ("Audio analysis thread"),
        audioDataCollector (adc),
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
                          sampleRate),
        pitchEstimator   (1024, sampleRate),
        overlapper       (1, 1024, audioDataCollector),
        f0Estimation     (5)
    {}

    void run() override
    {
        while (!threadShouldExit())
        {
            ///setFeatureAnalysisAudioBuffer (spectralFeatures, audioAnalyserSpec);
            //extractFeatures               (spectralFeatures, audioAnalyserSpec);
            estimateF0();
            wait (-1);
        }
    }

    void sampleRateChanged (double newSampleRate)
    {
        //stopThread();
        spectralFeatures.nyquistFrequency = newSampleRate / 2.0;
        spectralFeatures.nyquistFrequency = newSampleRate / 2.0;

        
        //startThread (FeatureExtractorLookAndFeel::getAnimationRateHz());
    }

    ConcatenatedFeatureBuffer& getSpectralFeatures()
    {
        return spectralFeatures;
    }

    ConcatenatedFeatureBuffer& getHarmonicFeatures()
    {
        return harmonicFeatures;
    }

    float getPitchEstimate() { return (float) f0Estimation.getTotal() / f0Estimation.recordedHistory; }

    PitchAnalyser&               getPitchAnalyser() { return pitchEstimator; }
    RealTimeAudioDataOverlapper& getOverlapper()    { return overlapper; }

private:
    AudioDataCollector&         audioDataCollector;
    AudioAnalyser               audioAnalyserSpec;
    AudioAnalyser               audioAnalyserHarm;
    ConcatenatedFeatureBuffer   spectralFeatures;
    ConcatenatedFeatureBuffer   harmonicFeatures;
    PitchAnalyser               pitchEstimator;
    RealTimeAudioDataOverlapper overlapper;
    ValueHistory                f0Estimation;

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

    void estimateF0 ()
    {
        const int numSamplesInAnalysisWindow = pitchEstimator.getFFTExpectedSamples();
        AudioSampleBuffer audioWindow = overlapper.getNextBuffer();
        f0Estimation.insertNewValueAndupdateHistory (pitchEstimator.estimatePitch (audioWindow));
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeAnalyser)
};




#endif  // REALTIMEANALYSER_H_INCLUDED
