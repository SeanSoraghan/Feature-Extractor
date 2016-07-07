/*
  ==============================================================================

    RealTimeAnalyser.h
    Created: 23 Oct 2015 10:02:59am
    Author:  Sean

  ==============================================================================
*/

#ifndef REALTIMEANALYSER_H_INCLUDED
#define REALTIMEANALYSER_H_INCLUDED

struct AudioFeatures
{
public:
    enum eAudioFeature
    {
        enOnset = 0,
        enRMS,
        enF0,
        enCentroid,
        enSpread,
        enFlatness,
        enFlux,
        enSlope,
        enHarmonicEnergyRatio,
        enOddEvenHarmonicRatio,
        enInharmonicity,
        numFeatures
    };

    static String getFeatureName (eAudioFeature featureType)
    {
        switch (featureType)
        {
            case enOnset:
                return String ("Onset");
            case enRMS:
                return String ("Amp.");
            case enF0:
                return String ("Pitch");
            case enCentroid:
                return String ("Centroid");
            case enSpread:
                return String ("Spread");
            case enFlatness:
                return String ("Flatness");
            case enFlux:
                return String ("Flux");
            case enSlope:
                return String ("Slope");
            case enHarmonicEnergyRatio:
                return String ("H.E.R");
            case enOddEvenHarmonicRatio:
                return String ("O.E.R");
            case enInharmonicity:
                return String ("Inharm.");
        }
    }

    static float getMaxValueForFeature (eAudioFeature f) 
    {
        return 1.0f;
    }

    AudioFeatures() 
    {
        for (int feature = 0; feature < eAudioFeature::numFeatures; feature++)
            smoothedFeatures.push_back (ValueHistory (feature == AudioFeatures::eAudioFeature::enOnset || feature == eAudioFeature::enFlux ? 1 : 10));
    }

    void updateFeature (eAudioFeature featureType, float newValue)
    {
        jassert (featureType < eAudioFeature::numFeatures && featureType >= enOnset);
        jassert ((int) featureType <= (int) smoothedFeatures.size());

        smoothedFeatures[(int) featureType].insertNewValueAndupdateHistory (newValue);
    }

    float getValue (eAudioFeature featureType) const
    {
        const ValueHistory feature = smoothedFeatures[(int) featureType];
        return feature.getTotal() / feature.recordedHistory;
    }

private:
    std::vector<ValueHistory> smoothedFeatures; 
};

//============================================================================================================================================================
//============================================================================================================================================================

class RealTimeAnalyser : public Thread
{
public:
    RealTimeAnalyser (AudioDataCollector& adc, AudioFeatures& featuresRef, int windowSize, double sampleRate = 48000.0)
    :   Thread ("Audio analysis thread"),
        audioDataCollector (adc),
        overlapper         (1, windowSize, audioDataCollector),
        fft                (windowSize, sampleRate),
        features           (featuresRef)
    {}

    virtual void run() override
    {}

    void sampleRateChanged (double newSampleRate)                           
    { 
        fft.setNyquistValue (newSampleRate / 2.0); 
    }

    RealTimeAudioDataOverlapper&    getOverlapper()       { return overlapper; }
    FFTAnalyser&                    getFFTAnalyser()      { return fft; }

    AudioFeatures&                  getFeatures()         { return features; }
private:
    AudioDataCollector&             audioDataCollector;
    RealTimeAudioDataOverlapper     overlapper;
    FFTAnalyser                     fft;
    AudioFeatures&                  features;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeAnalyser)
};


//============================================================================================================================================================
//============================================================================================================================================================

class RealTimeHarmonicAnalyser : public RealTimeAnalyser
{
public:
    RealTimeHarmonicAnalyser (AudioDataCollector& adc, AudioFeatures& featuresRef, int windowSize, double sampleRate = 48000.0)
    :   RealTimeAnalyser   (adc, featuresRef, windowSize, sampleRate),
        pitchEstimator     (getFFTAnalyser())
    {}

    void run() override
    {
        while (!threadShouldExit())
        {
            FFTAnalyser& fftAnalyser = getFFTAnalyser();
            const int numSamplesInHarmAnalysisWindow = fftAnalyser.getFFTExpectedSamples();
            AudioSampleBuffer audioWindow = getOverlapper().getNextBuffer();
            float rms = audioWindow.getRMSLevel (0, 0, audioWindow.getNumSamples());
            float logRMS = log10 (rms * 9.0f + 1.0f);
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enRMS, logRMS);
            /* Low-pass filter the audio */
            AudioSampleBuffer filteredAudio (audioWindow);
            filteredAudio.clear();
            filter.filterAudio (audioWindow, filteredAudio);

            /* Apply windowing function */
            windower.scaleBufferWithBartlettWindowing (filteredAudio);

            /* Compute FFT */
            AudioSampleBuffer filteredFrequencyBuffer = fftAnalyser.getFrequencyData (filteredAudio);
            AudioSampleBuffer frequencyBuffer = fftAnalyser.getFrequencyData (audioWindow);
            
            /* Estimate Pitch */
            double f0Estimate = pitchEstimator.estimatePitch (filteredFrequencyBuffer);
            const double f0NormalisationFactor = 5000.0;
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enF0, (float) (f0Estimate / f0NormalisationFactor));

            /* Calculate harmonic features based on pitch estimation */
            HarmonicCharacteristics harmonicFeatures = harmonicAnalyser.calculateHarmonicCharacteristics (frequencyBuffer, f0Estimate, getFFTAnalyser().getNyquist(), 0);
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enHarmonicEnergyRatio, harmonicFeatures.harmonicEnergyRatio);
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enOddEvenHarmonicRatio, harmonicFeatures.harmonicEnergyRatio);
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enInharmonicity,       harmonicFeatures.inharmonicity);

            /* Wait for notification that new audio data has come in from audio thread */
            wait (-1);
        }
    }

    PitchAnalyser&                   getPitchAnalyser()    { return pitchEstimator; }
    HarmonicCharacteristicsAnalyser& getHarmonicAnalyser() { return harmonicAnalyser; }
private:
    AudioFilter                     filter;
    RealTimeWindower                windower;
    HarmonicCharacteristicsAnalyser harmonicAnalyser;
    PitchAnalyser                   pitchEstimator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeHarmonicAnalyser)
};

//============================================================================================================================================================
//============================================================================================================================================================

class RealTimeSpectralAnalyser : public RealTimeAnalyser
{
public:
    RealTimeSpectralAnalyser (AudioDataCollector& adc, AudioFeatures& featuresRef, int windowSize, double sampleRate = 48000.0)
    :   RealTimeAnalyser (adc, featuresRef, windowSize, sampleRate),
        spectralAnalyser (windowSize)
    {}

    void run() override
    {
        while (!threadShouldExit())
        {
            const int numSamplesInHarmAnalysisWindow = getFFTAnalyser().getFFTExpectedSamples();
            AudioSampleBuffer audioWindow = getOverlapper().getNextBuffer();
            float rms = audioWindow.getRMSLevel (0, 0, audioWindow.getNumSamples());
            float logRMS = log10 (rms * 9.0f + 1.0f);
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enRMS, logRMS);

            /* Apply windowing function */
            windower.scaleBufferWithBartlettWindowing (audioWindow);

            /* Compute FFT */
            AudioSampleBuffer frequencyBuffer = getFFTAnalyser().getFrequencyData (audioWindow);

            /* Get spectral features */
            SpectralCharacteristics spectralFeatures = spectralAnalyser.calculateSpectralCharacteristics (frequencyBuffer, 0, getFFTAnalyser().getNyquist());
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enCentroid, spectralFeatures.centroid);
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enFlatness, spectralFeatures.flatness);
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enSpread,   spectralFeatures.spread);
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enFlux,     spectralFeatures.flux);
            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enSlope,    spectralAnalyser.calculateNormalisedSpectralSlope (frequencyBuffer, 0));

            getFeatures().updateFeature (AudioFeatures::eAudioFeature::enOnset, detectOnset());
            
            if (getFeatures().getValue (AudioFeatures::eAudioFeature::enOnset) > 0.0f && onsetDetectedCallback != nullptr)
                onsetDetectedCallback();

            /* Wait for notification that new audio data has come in from audio thread */
            wait (-1);
        }
    }

    float detectOnset ()
    {
        float currentSpectralFluxValue = getFeatures().getValue (AudioFeatures::eAudioFeature::enFlux);
        float currentAmp               = getFeatures().getValue (AudioFeatures::eAudioFeature::enRMS);
        onsetDetector.addSpectralFluxAndAmpValue (currentSpectralFluxValue, currentAmp);
        return onsetDetector.detectOnset() ? 1.0f : 0.0f;
    }

    void setOnsetDetectionSensitivity (float s)
    {
        jassert (s >= 0.0f);
        onsetDetector.meanThresholdMultiplier = 1.0f + s;
    }

    void setOnsetWindowLength (int length)
    {
        onsetDetector.ampHistory.setHistoryLength          (length);
        onsetDetector.spectralFluxHistory.setHistoryLength (length);
    }

    void setOnsetDetectedCallback (std::function<void()> f) { onsetDetectedCallback = f; }

    void setOnsetDetectionType (OnsetDetector::eOnsetDetectionType t)           { onsetDetector.type = t; }

    OnsetDetector&                   getOnsetDetector()    { return onsetDetector; }
    SpectralCharacteristicsAnalyser& getSpectralAnalyser() { return spectralAnalyser; }
private:
    RealTimeWindower                windower;
    SpectralCharacteristicsAnalyser spectralAnalyser;
    OnsetDetector                   onsetDetector;
    std::function<void()>           onsetDetectedCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeSpectralAnalyser)
};

#endif  // REALTIMEANALYSER_H_INCLUDED
