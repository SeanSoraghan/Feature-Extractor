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
    RealTimeAnalyser (AudioDataCollector& adc, double sampleRate = 48000.0)
    :   Thread ("Audio analysis thread"),
        audioDataCollector (adc),
        overlapper       (1, 1024, audioDataCollector),
        fft              (1024, sampleRate),
        spectralAnalyser (1024),
        pitchEstimator   (fft)
    {}

    void run() override
    {
        while (!threadShouldExit())
        {
            const int numSamplesInAnalysisWindow = fft.getFFTExpectedSamples();
            AudioSampleBuffer audioWindow = overlapper.getNextBuffer();
            features.updateFeature (AudioFeatures::eAudioFeature::enRMS, audioWindow.getRMSLevel (0, 0, audioWindow.getNumSamples()));
            /* Low-pass filter the audio */
            AudioSampleBuffer filteredAudio (audioWindow);
            filteredAudio.clear();
            filter.filterAudio (audioWindow, filteredAudio);

            /* Apply windowing function */
            windower.scaleBufferWithBartlettWindowing (filteredAudio);
            windower.scaleBufferWithBartlettWindowing (audioWindow);

            /* Compute FFT */
            AudioSampleBuffer filteredFrequencyBuffer = fft.getFrequencyData (filteredAudio);
            AudioSampleBuffer frequencyBuffer = fft.getFrequencyData (audioWindow);

            /* Get spectral features */
            SpectralCharacteristics spectralFeatures = spectralAnalyser.calculateSpectralCharacteristics (frequencyBuffer, 0, fft.getNyquist());
            features.updateFeature (AudioFeatures::eAudioFeature::enCentroid, spectralFeatures.centroid);
            features.updateFeature (AudioFeatures::eAudioFeature::enFlatness, spectralFeatures.flatness);
            features.updateFeature (AudioFeatures::eAudioFeature::enSpread,   spectralFeatures.spread);
            features.updateFeature (AudioFeatures::eAudioFeature::enFlux,     spectralFeatures.flux);
            features.updateFeature (AudioFeatures::eAudioFeature::enSlope,    spectralAnalyser.calculateNormalisedSpectralSlope (frequencyBuffer, 0));

            features.updateFeature (AudioFeatures::eAudioFeature::enOnset, detectOnset());
            
            if (features.getValue (AudioFeatures::eAudioFeature::enOnset) > 0.0f && onsetDetectedCallback != nullptr)
                onsetDetectedCallback();
            
            /* Estimate Pitch */
            double f0Estimate = pitchEstimator.estimatePitch (filteredFrequencyBuffer);
            const double f0NormalisationFactor = 5000.0;
            features.updateFeature (AudioFeatures::eAudioFeature::enF0, (float) (f0Estimate / f0NormalisationFactor));

            /* Calculate harmonic features based on pitch estimation */
            HarmonicCharacteristics harmonicFeatures = harmonicAnalyser.calculateHarmonicCharacteristics (frequencyBuffer, f0Estimate, fft.getNyquist(), 0);
            features.updateFeature (AudioFeatures::eAudioFeature::enHarmonicEnergyRatio, harmonicFeatures.harmonicEnergyRatio);
            features.updateFeature (AudioFeatures::eAudioFeature::enInharmonicity,       harmonicFeatures.inharmonicity);

            /* Wait for notification that new audio data has come in from audio thread */
            wait (-1);
        }
    }

    float detectOnset ()
    {
        float currentSpectralFluxValue = features.getValue (AudioFeatures::eAudioFeature::enFlux);
        float currentAmp               = features.getValue (AudioFeatures::eAudioFeature::enRMS);
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
    void sampleRateChanged     (double newSampleRate)                           { fft.setNyquistValue (newSampleRate / 2.0); }
    float getAudioFeature      (AudioFeatures::eAudioFeature featureType) const { return features.getValue (featureType); }

    RealTimeAudioDataOverlapper&     getOverlapper()       { return overlapper; }
    FFTAnalyser&                     getFFTAnalyser()      { return fft; }
    PitchAnalyser&                   getPitchAnalyser()    { return pitchEstimator; }
    OnsetDetector&                   getOnsetDetector()    { return onsetDetector; }
    HarmonicCharacteristicsAnalyser& getHarmonicAnalyser() { return harmonicAnalyser; }
    SpectralCharacteristicsAnalyser& getSpectralAnalyser() { return spectralAnalyser; }
private:
    AudioDataCollector&             audioDataCollector;
    AudioFilter                     filter;
    RealTimeWindower                windower;
    RealTimeAudioDataOverlapper     overlapper;
    FFTAnalyser                     fft;
    SpectralCharacteristicsAnalyser spectralAnalyser;
    HarmonicCharacteristicsAnalyser harmonicAnalyser;
    PitchAnalyser                   pitchEstimator;
    OnsetDetector                   onsetDetector;
    AudioFeatures                   features;
    std::function<void()>           onsetDetectedCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeAnalyser)
};




#endif  // REALTIMEANALYSER_H_INCLUDED
