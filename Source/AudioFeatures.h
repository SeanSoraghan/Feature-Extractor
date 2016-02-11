/*
  ==============================================================================

    AudioFeatures.h
    Created: 6 Aug 2015 10:17:42am
    Author:  Sean Soraghan

  ==============================================================================
*/

#ifndef AUDIOFEATURES_H_INCLUDED
#define AUDIOFEATURES_H_INCLUDED

class ConcatenatedFeatureBuffer : public ReferenceCountedObject
{
public:
    
    typedef ReferenceCountedObjectPtr<ConcatenatedFeatureBuffer> Ptr;
    
    enum Feature
    {
        Centroid = 0,
        Slope,
        Spread,
        Flatness,
        ZeroCrosses,
        F0,
        HER,
        Inharmonicity,
        Audio,
        FFT,
        NumFeatures
    };
    
    static bool isFeatureSpectral (Feature f)
    {
        if ((int) f <= (int)Feature::Flatness)
            return true;
        return false;
    }
    static String getFeatureName (Feature f)
    {
        switch (f)
        {
            case Centroid:    return "Centroid";
            case Slope:       return "Slope";
            case Spread:      return "Spread";
            case Flatness:    return "Flatness";
            case ZeroCrosses: return "ZeroCrosses";
            case F0:          return "F0";
            case Audio:       return "Audio";
            case FFT:         return "FFT";
            case NumFeatures: return "NumFeatures";
            default: jassert(false); return "UNKNOWN";
        }
    }
    /* ----------------------------------------------------------------- */
    
    ConcatenatedFeatureBuffer ()
    :   sampleRate             (0),
        numDownsamples         (0),
        numFeatureChannels     (0),
        numRealFFTBins         (0),
        estimatedLogAttackTime (0.0f),
        totalSequenceTimeMs    (0.0),
        nyquistFrequency       (0.0),
        audioOutput            ()
    {}
    
    ConcatenatedFeatureBuffer (int nChannels, int numberOfSamples, int numberOfDownsamples, int numberOfRealFFTBins, double seqTime, double sampleR)
    :   sampleRate             ((int) sampleR),
        numDownsamples         (numberOfDownsamples),
        numFeatureChannels     (nChannels),
        numRealFFTBins         (numberOfRealFFTBins),
        estimatedLogAttackTime (0.0f),
        totalSequenceTimeMs    (seqTime),
        nyquistFrequency       (sampleR / 2.0),
        audioOutput            (nChannels, numberOfSamples)
    {
        if (numDownsamples > 0)
        {
            int size = numDownsamples * (Feature::NumFeatures - 1) * nChannels + numRealFFTBins * numDownsamples * nChannels;
            featureBuffer = AudioSampleBuffer (1, size);
        }
        else
        {
            featureBuffer = AudioSampleBuffer();
        }
    }
    
    ConcatenatedFeatureBuffer (ConcatenatedFeatureBuffer& other)
    :   sampleRate             (other.sampleRate),
        numDownsamples         (other.numDownsamples),
        numFeatureChannels     (other.numFeatureChannels),
        numRealFFTBins         (other.numRealFFTBins),
        estimatedLogAttackTime (other.estimatedLogAttackTime),
        totalSequenceTimeMs    (other.totalSequenceTimeMs),
        nyquistFrequency       (other.nyquistFrequency),
        audioOutput            (other.audioOutput),
        featureBuffer          (other.featureBuffer),
        energyEnvelope         (other.energyEnvelope),
        modulationOutput       (other.modulationOutput)
    {}
    
    ConcatenatedFeatureBuffer& operator= (const ConcatenatedFeatureBuffer& other) noexcept
    {
        sampleRate             = other.sampleRate;
        numDownsamples         = other.numDownsamples;
        numFeatureChannels     = other.numFeatureChannels;
        numRealFFTBins         = other.numRealFFTBins;
        estimatedLogAttackTime = other.estimatedLogAttackTime;
        totalSequenceTimeMs    = other.totalSequenceTimeMs;
        nyquistFrequency       = other.nyquistFrequency;
        audioOutput            = other.audioOutput;
        featureBuffer          = other.featureBuffer;
        energyEnvelope         = other.energyEnvelope;
        modulationOutput       = other.modulationOutput;
        return *this;
    }
    
    /* ----------------------------------------------------------------- */
    
    void setFeatureArray (Feature f, const float* data, int channel)
    {
        /* For FFT use set FFTArray */
        jassert (f != Feature::FFT);
        
        int arrayStartIndex = f * numDownsamples * numFeatureChannels + channel * numDownsamples;
        featureBuffer.copyFrom (0, arrayStartIndex, data, numDownsamples);
    }
    
    void setFeatureSample (Feature f, int index, float sample, int channel)
    {
        /* For FFT use set FFTSample */
        jassert (f != Feature::FFT);
        
        int arrayStartIndex = f * numDownsamples * numFeatureChannels + channel * numDownsamples;
        featureBuffer.setSample (0, arrayStartIndex + index, sample);
    }
    
    void setFFTBinsForSample (const float* binsData, int channel, int samplePosition)
    {
        jassert (samplePosition >= 0 && samplePosition < numDownsamples);
        
        int fftArrayStartIndex = (Feature::NumFeatures - 1) * numDownsamples * numFeatureChannels;
        int fftChannelStartIndex = channel * numDownsamples * numRealFFTBins;
        
        int sampleIndex = samplePosition;
        
        for (int bin = 0; bin < numRealFFTBins; bin++)
        {
            featureBuffer.setSample (0, fftArrayStartIndex + fftChannelStartIndex + sampleIndex, binsData [bin]);
            sampleIndex += numDownsamples;
        }
    }
    
    void fillDownsampledRMSAudioChannels ()
    {
        int numRawSamples = audioOutput.getNumSamples();
        int samplesPerStep = numRawSamples / numDownsamples;
        
        jassert (samplesPerStep > 0);
        
        if (samplesPerStep == 1)
        {
            for (int channel = 0; channel < numFeatureChannels; channel++)
            {
                setFeatureArray (Feature::Audio, audioOutput.getReadPointer (channel), channel);
            }
            return;
        }
        
        jassert(numFeatureChannels == audioOutput.getNumChannels());
        
        for (int channel = 0; channel < numFeatureChannels; ++channel)
        {
            for (int i = 0; i < numDownsamples; i++)
            {
                float sample = audioOutput.getRMSLevel (channel, i * samplesPerStep, samplesPerStep);
                setFeatureSample (Feature::Audio, i, sample, channel);
            }
        }
    }
    
    void normaliseAudioChannels()
    {
        int arrayStartIndex = Feature::Audio * numDownsamples * numFeatureChannels;
        Range<float> range = featureBuffer.findMinMax (0, arrayStartIndex, numDownsamples * numFeatureChannels);
        
        float absoluteMax = jmax (abs (range.getStart()), abs (range.getEnd()));
        
        float normScale = 1.0f / absoluteMax;
        featureBuffer.applyGain (arrayStartIndex, numDownsamples * numFeatureChannels, normScale);
    }
    
    const float* getFullFeatureBufferReadPointer()
    {
        return featureBuffer.getReadPointer (0);
    }
    
    const float* getFeatureBufferReadPointer (Feature featureType)
    {
        int featureStartIndex = featureType * numDownsamples * numFeatureChannels;
        return &getFullFeatureBufferReadPointer() [featureStartIndex];
    }
    
    Range<float> getFeatureRange (Feature featureType)
    {
        jassert (featureType != Feature::FFT);
        int arrayStartIndex = featureType * numDownsamples * numFeatureChannels;
        return featureBuffer.findMinMax (0, arrayStartIndex, numDownsamples * numFeatureChannels);
    }
    
    std::vector<float> getDiscreteFeatureDistribution (Feature featureType, int numBrackets)
    {
        std::vector<float> distribution;
        std::vector<float> sortedArray;
        
        for (int i = 0; i < numDownsamples; ++i)
        {
            sortedArray.push_back (getFeatureBufferReadPointer (featureType) [i]);
        }
        
        std::sort (std::begin (sortedArray), std::end (sortedArray));
        
        int step = numDownsamples / numBrackets;
        
        for (int bracket = 0; bracket < numBrackets; ++bracket)
        {
            float average = 0.0f;
            for (int featureSample = bracket * step; featureSample < (bracket + 1) * step; ++featureSample)
            {
                jassert (featureSample < numDownsamples);
                average += sortedArray[(size_t)featureSample];
            }
            distribution.push_back (average / (float)step);
        }
        return distribution;
    }
    
    AudioSampleBuffer getFeatureBuffer (Feature feature)
    {
        jassert (feature >= 0 && feature < Feature::NumFeatures && feature != Feature::FFT);
        AudioSampleBuffer buffer;
        buffer.setSize (numFeatureChannels, numDownsamples);
        buffer.clear();
        int featureStartIndex = feature * numDownsamples * numFeatureChannels;
        for (int channel = 0; channel < numFeatureChannels; channel++)
        {
            buffer.copyFrom (channel, 0, featureBuffer, 0, channel * numDownsamples + featureStartIndex , numDownsamples);
        }
        return buffer;
    }
    
    float getFeatureSample (Feature feature, int channel, int index)
    {
        jassert (feature >= 0 && feature < Feature::NumFeatures && feature != Feature::FFT);
        jassert (index >= 0 && index < numDownsamples);
        int featureStart = feature * numDownsamples * numFeatureChannels;
        int channelStart = channel * numDownsamples;
        int sampleIndex = featureStart + channelStart + index;
        return featureBuffer.getSample (0, sampleIndex);
    }
    
    float getAverageFeatureSample (Feature feature, int index)
    {
        float av = 0.0f;
        for (int c = 0; c < numFeatureChannels; c++)
        {
            av += getFeatureSample (feature, c, index);
        }
        av /= numFeatureChannels;
        return av;
    }

    float getAverageRMSLevel()
    {
        float av = 0.0f;
        for (int c = 0; c < numFeatureChannels; c++)
        {
            av += audioOutput.getRMSLevel (c, 0, audioOutput.getNumSamples());
        }
        av /= numFeatureChannels;
        return av;
    }

    /* ----------------------------------------------------------------- */
    
    int                     sampleRate;
    int                     numDownsamples;
    int                     numFeatureChannels;
    int                     numRealFFTBins;
    float                   estimatedLogAttackTime;
    double                  totalSequenceTimeMs;
    double                  nyquistFrequency;
    AudioSampleBuffer       audioOutput;
    AudioSampleBuffer       featureBuffer;
    AudioSampleBuffer       energyEnvelope;

   std:: vector<std::vector<float>> modulationOutput;

};

//==========================================================================================
//==========================================================================================

struct FeatureSpace
{
    FeatureSpace() :    attackRange(),
    spectralCentroidRange(),
    spectralSlopeRange(),
    zeroCrossesRange()
    {}
    
    FeatureSpace (float attack,
                  Range<float> centroidRange,
                  Range<float> slopeRange,
                  Range<float> crossesRange)
    :   attackRange           (Range<float> (attack, 0.0f)),
        spectralCentroidRange (centroidRange),
        spectralSlopeRange    (slopeRange),
        zeroCrossesRange      (crossesRange)
    {}
    
    static void compareAndUpdateRanges (Range<float>& existingRange, Range<float>& newRange)
    {
        if (newRange.getStart() < existingRange.getStart())
            existingRange.setStart (newRange.getStart());
        if (newRange.getEnd() < existingRange.getEnd())
            existingRange.setEnd (newRange.getEnd());
    }
    
    Range<float> attackRange;
    Range<float> spectralCentroidRange;
    Range<float> spectralSlopeRange;
    Range<float> zeroCrossesRange;
};

//==========================================================================================
//==========================================================================================

typedef ConcatenatedFeatureBuffer::Feature AudioFeature;

struct SmoothedFeatures
{
    SmoothedFeatures (int averagingDepth) : historyLength ((size_t) averagingDepth)
    {
        for (size_t i = 0; i < AudioFeature::NumFeatures - 2; i++) //only 1-D features.
        {
            currentFeatureValues.push_back (0.0f);
            featureHistory.push_back (std::vector<float>());
            for (int a = 0; a < averagingDepth; a++)
            {
                featureHistory[i].push_back (0.0f);
            }
        }
    }
    
    void updateValues (ConcatenatedFeatureBuffer& features)
    {
        updateHistory();
        for (int i = 0; i < AudioFeature::NumFeatures - 2; i++) //only 1-D features.
        {
            float newValue = features.getFeatureSample (AudioFeature (i), 0, 0);
            updateFeatureWithIncomingValue (AudioFeature (i), newValue);
        }
    }
    
    size_t historyLength;
    std::vector<float> currentFeatureValues;
    std::vector<std::vector<float>> featureHistory;
    
private:
    
    void updateFeatureWithIncomingValue (AudioFeature feature, float newValue)
    {
        float weight = JUCE_LIVE_CONSTANT(0.5f);
        
        // calculate new average and augment by new incoming value
        float runningAverage = calculateRunningAverage (feature);
        currentFeatureValues[feature] = runningAverage + (newValue - runningAverage) * weight;
    }
    
    /*
     shift every entry in each values vector down by 1 (add the most recent value to the history, and discard the oldest)
     */
    void updateHistory ()
    {
        for (size_t f = 0; f < featureHistory.size(); f++)
        {
            for (size_t i = 0; i < historyLength - 1; i++)//purposefully miss the last index, as it will be shifted down and replaced with the new incoming running average
            {
                featureHistory[f][i] = featureHistory[f][i + 1];
            }
            featureHistory[f][historyLength - 1] = currentFeatureValues[f];
        }
    }
    
    float calculateRunningAverage (AudioFeature feature)
    {
        float av = 0.0f;
        for (size_t i = 0; i < historyLength; i++)
        {
            av += featureHistory[feature][i];
        }
        av /= (float) historyLength + 1;
        return av;
    }
};


#endif  // AUDIOFEATURES_H_INCLUDED
