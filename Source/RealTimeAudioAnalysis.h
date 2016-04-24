/*
  ==============================================================================

    AudioFilter.h
    Created: 17 Apr 2016 11:23:31am
    Author:  Sean

  ==============================================================================
*/

#ifndef AUDIOFILTER_H_INCLUDED
#define AUDIOFILTER_H_INCLUDED

//const float pi = 3.14159265359f; USE float_Pi

static void printBuffer (AudioSampleBuffer& b)
{
    String dataString = String::empty;
    for (int i = 0; i < b.getNumSamples(); i++)
    {
        dataString = dataString + " | " + String (b.getSample (0, i)); 
    }
    DBG (dataString);
}

static void printBuffer (const float* b, int size)
{
    String dataString = String::empty;
    for (int i = 0; i < size; i++)
    {
        dataString = dataString + " | " + String (b[i]); 
    }
    DBG (dataString);
}

//============================================================================================================================================================
//============================================================================================================================================================

struct ValueHistory 
{
public:
    ValueHistory (int historyLength)
    :   recordedHistory (0)
    {
        setHistoryLength (historyLength);
    }

    float getTotal()
    {
        float total = 0.0f;
        for (int i = 0; i < (int)history.size(); i++)
        {
            total += history[i];
        }
        return total;
    }

    void insertNewValueAndupdateHistory (float newValue)
    {
        for (int i = 0; i < (int) history.size() - 1; i++)
        {
            history[i] = history[i + 1];
        }
        history[history.size() - 1] = newValue;
        
        //updateHistoryIndex();
        
        if (recordedHistory < (int) history.size())
            recordedHistory ++;
    }

    void setHistoryLength (int historyLength)
    {
        recordedHistory = 0;
        history.clear();
        for (int i = 0; i < historyLength; i++)
        {
            history.push_back (0.0f);
        }
    }

    void printHistory()
    {
        String h (String::empty);
        for (int i = 0; i < (int)history.size(); i++)
        {
            h<<String (history[i]);
            h<<" ";
        }
        DBG(h);
    }

    std::vector<float> history;
    int recordedHistory;
};

//============================================================================================================================================================
//============================================================================================================================================================

class AudioFilter
{
public:
    AudioFilter() {}

    void filterAudio (AudioSampleBuffer& bufferIn, AudioSampleBuffer& bufferOut)
    {
        jassert (bufferIn.getNumChannels() == bufferOut.getNumChannels() && bufferIn.getNumSamples() == bufferOut.getNumSamples());
        const int numSamples  = bufferIn.getNumSamples();
        const int numChannels = bufferIn.getNumChannels();

        for (int c = 0; c < numChannels; c++)
        {
            const float* inData  = bufferIn. getReadPointer  (c);
            float* outData       = bufferOut.getWritePointer (c);
            
            if (numSamples > 0)
                bufferOut.setSample (c, 0, bufferIn.getSample (c, 0));

            for (int sample = 1; sample < numSamples; sample++)
            {
                outData[sample] =  ((float_Pi / m ) * inData[sample]) + (exp (-float_Pi / m) * outData[sample - 1]);
            }
        }
    }

    const float m { 2.0f };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFilter)
};

//============================================================================================================================================================
//============================================================================================================================================================

class RealTimeWindower 
{
public:
    RealTimeWindower() {}

    static void scaleBufferWithBartlettWindowing (AudioSampleBuffer& source) // TODO: change to DSPMultiBlock
    {
        int numSamples = source.getNumSamples();
        
        /* /\  <- very simple window shape */
        for (int channel = 0; channel < source.getNumChannels(); ++channel)
        {
            source.applyGainRamp (channel, 0, numSamples / 2, 0.0f, 1.0f);
            source.applyGainRamp (channel, numSamples / 2, numSamples / 2, 1.0f, 0.0f);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeWindower)
};

//============================================================================================================================================================
//============================================================================================================================================================

class RealTimeFFT
{
public:
    RealTimeFFT (int samplesPerWindow)
    :   forwardFFT (int (log (samplesPerWindow) / log (2) + 1e-6), false),
        inverseFFT (int (log (samplesPerWindow) / log (2) + 1e-6), true)
    {}

    void performForward (float* timeData, const int size)
    {
        jassert (size == forwardFFT.getSize() * 2);
        forwardFFT.performRealOnlyForwardTransform (timeData);
    }

    void performInverse (float* frequencyData, const int size) const
    {
        jassert (size == inverseFFT.getSize() * 2);
        inverseFFT.performRealOnlyInverseTransform (frequencyData);
    }

    int getFFTExpectedSamples() const
    {
        return forwardFFT.getSize();
    }

private:
    FFT forwardFFT;
    FFT inverseFFT;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeFFT)

};

//============================================================================================================================================================
//============================================================================================================================================================

class RealTimeAudioDataOverlapper
{
public:
    RealTimeAudioDataOverlapper (int numChannels, int windowSize, AudioDataCollector& collector) 
    :   circleBuffer        (collector),
        overlappedAudio     (numChannels, windowSize),
        numSamplesPerWindow (windowSize)
    {
        overlappedAudio.clear();
    }

    AudioSampleBuffer& getNextBuffer()
    {
        const int halfWindowSize = numSamplesPerWindow / 2;
        AudioSampleBuffer newDataBuffer = circleBuffer.getAnalysisBuffer (halfWindowSize);
        jassert (numSamplesPerWindow % 2 == 0);
        for (int c = 0; c < overlappedAudio.getNumChannels(); c++)
        {
            float* existingData = overlappedAudio.getWritePointer (c);            
            //shift existing samples to the left
            for (int sampleToShift = numSamplesPerWindow - 1; sampleToShift >= halfWindowSize; sampleToShift--)
                existingData[sampleToShift - halfWindowSize] = existingData[sampleToShift];

            //copy new samples into the right
            overlappedAudio.copyFrom (c, halfWindowSize, newDataBuffer, c, 0, halfWindowSize);
        }
        
        if (displayBufferNeedsUpdating.get() == 1)
        {
            bufferToDraw = AudioSampleBuffer (overlappedAudio);
            displayBufferNeedsUpdating.set (0);
        }

        return overlappedAudio;
    }

    void enableBufferToDrawNeedsUpdating()     { displayBufferNeedsUpdating.set (1); }

    const AudioSampleBuffer getBufferToDraw() { return AudioSampleBuffer (bufferToDraw); }

private:
    AudioDataCollector& circleBuffer;
    AudioSampleBuffer overlappedAudio;
    AudioSampleBuffer bufferToDraw;
    Atomic<int>       displayBufferNeedsUpdating;
    int               numSamplesPerWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RealTimeAudioDataOverlapper)
};
//============================================================================================================================================================
//============================================================================================================================================================

class FFTAnalyser
{
public:
    FFTAnalyser (int numSamplesPerWindow, double sampleRate) 
    :   fft     (numSamplesPerWindow),
        nyquist (sampleRate / 2.0)
    {}

    AudioSampleBuffer getFrequencyData (AudioSampleBuffer& timeBuffer)
    {
        const int numChannels = timeBuffer.getNumChannels();
        const int numSamples  = timeBuffer.getNumSamples();

        AudioSampleBuffer frequencyBuffer (timeBuffer);
        frequencyBuffer.clear();

        filter.filterAudio (timeBuffer, frequencyBuffer);
        windower.scaleBufferWithBartlettWindowing (frequencyBuffer);

        const int numFFTElements = numSamples * 2;
        frequencyBuffer.setSize (numChannels, numFFTElements, true, true, false);

        for (int c = 0; c < numChannels; c++)
        {
            float* inOutData = frequencyBuffer.getWritePointer (c);
            fft.performForward (inOutData, numFFTElements);
        }

        if (fftDisplayBufferNeedsUpdating.get() == 1)
        {
            fftBufferToDraw = AudioSampleBuffer (frequencyBuffer);
            fftDisplayBufferNeedsUpdating.set (0);
        }

        return frequencyBuffer;
    }

    void enableFFTBufferToDrawNeedsUpdating()     { fftDisplayBufferNeedsUpdating.set (1); }

    const AudioSampleBuffer getFFTBufferToDraw() const { return AudioSampleBuffer (fftBufferToDraw); }
    int getFFTExpectedSamples()                  const { return fft.getFFTExpectedSamples(); }
    const RealTimeFFT& getFFTObject()            const { return fft; }
    double getNyquist()                          const { return nyquist; }
private:
    AudioFilter       filter;
    RealTimeWindower  windower;
    RealTimeFFT       fft; 
    AudioSampleBuffer fftBufferToDraw;
    double            nyquist;
    Atomic<int>       fftDisplayBufferNeedsUpdating;
};

//============================================================================================================================================================
//============================================================================================================================================================

class PitchAnalyser
{
public:
    PitchAnalyser (int numSamplesPerWindow, double sampleRate, FFTAnalyser& ffT) 
    :   fft (ffT)
    {}

    double estimatePitch (AudioSampleBuffer& frequencyData)
    {
        AudioSampleBuffer conjugateMultiplication = getComplexConjugateMultiplication (frequencyData);
        
        AudioSampleBuffer autoCorrelationBuffer   = getAutoCorrelationFromConjugateMultiplication (conjugateMultiplication);

        if (autoCorrelationDisplayBufferNeedsUpdating.get() == 1)
        {
            autoCorrelationBufferToDraw = AudioSampleBuffer (autoCorrelationBuffer);
            autoCorrelationDisplayBufferNeedsUpdating.set (0);
        }

        AudioSampleBuffer cumulativeNormalisedDifferenceBuffer = getCumulativeNormalisedDifferenceFromAutoCorrelationBuffer (autoCorrelationBuffer);

        if (cumulativeDifferenceBufferNeedsUpdating.get() == 1)
        {
            cumulativeDifferenceBufferToDraw = AudioSampleBuffer (cumulativeNormalisedDifferenceBuffer);
            cumulativeDifferenceBufferNeedsUpdating.set (0);
        }

        const float lagEstimate = getLagEstimateFromCumulativeDifference (cumulativeNormalisedDifferenceBuffer, 0);
        DBG ("Lag: " << lagEstimate);
        return (fft.getNyquist() * 2.0f) / lagEstimate; //getPitchFromAutoCorrelationBuffer (autoCorrelationBuffer);
        //return estimatePitchFromFrequencyData (frequencyData);
    }



    AudioSampleBuffer getComplexConjugateMultiplication (AudioSampleBuffer& frequencyData)
    {
        const int numChannels = frequencyData.getNumChannels();
        const int numSamples  = frequencyData.getNumSamples();
        AudioSampleBuffer conjugateMultiplicationBuffer (frequencyData);
        conjugateMultiplicationBuffer.clear();

        jassert (numSamples % 2 == 0);
        
        for (int c = 0; c < numChannels; c++)
        {
            const float* complexData           = frequencyData.getReadPointer (c);
            float* conjugateMultiplicationData = conjugateMultiplicationBuffer.getWritePointer (c);

            for (int magnitudeIndex = 0; magnitudeIndex < numSamples; magnitudeIndex += 2)
            {
                const int phaseIndex  = magnitudeIndex + 1;
                const float magnitude = complexData[magnitudeIndex];
                const float phase     = complexData[phaseIndex];
                //const float conjIm = -complexData[imagineryIndex];
                //float newReal      = real * real - im * conjIm;
                //float newImag      = real * conjIm + im * real;
                conjugateMultiplicationData[magnitudeIndex] = magnitude * magnitude;
                conjugateMultiplicationData[phaseIndex]     = 0.0f;
            }
        }

        return conjugateMultiplicationBuffer;
    }

    AudioSampleBuffer getAutoCorrelationFromConjugateMultiplication (AudioSampleBuffer& conjugateMultiplicationBuffer)
    {
        const int numChannels = conjugateMultiplicationBuffer.getNumChannels();
        const int numSamples  = conjugateMultiplicationBuffer.getNumSamples();

        AudioSampleBuffer autoCorrelationBuffer (conjugateMultiplicationBuffer);

        for (int c = 0; c < numChannels; c++)
        {
            float* inOutData = autoCorrelationBuffer.getWritePointer (c);
            fft.getFFTObject().performInverse (inOutData, numSamples);

            for (int sample = 0; sample < numSamples; sample++)
                inOutData[sample] = inOutData[sample] * inOutData[sample] * sample;
        }

        return autoCorrelationBuffer;
    }
    
    AudioSampleBuffer getCumulativeNormalisedDifferenceFromAutoCorrelationBuffer (AudioSampleBuffer& autoCorrelationBuffer)
    {
        const int numChannels = autoCorrelationBuffer.getNumChannels();
        const int numSamples  = autoCorrelationBuffer.getNumSamples();
        AudioSampleBuffer cumulativeNormalisedDifferenceBuffer (autoCorrelationBuffer);
        cumulativeNormalisedDifferenceBuffer.clear();

        for (int channel = 0; channel < numChannels; channel++)
        {
            float sum = 0.0f;
            const float* autoCorrelationData = autoCorrelationBuffer.getReadPointer (channel);
            float* cumulativeDiffData = cumulativeNormalisedDifferenceBuffer.getWritePointer (channel);
            cumulativeDiffData[0] = 1.0f;
            for (int sample = 1; sample < numSamples; sample++)
            {
                const float value = autoCorrelationData[sample];// * autoCorrelationData[sample];
                sum += value;
                const auto norm = (1.0f / sample) * sum;
                cumulativeDiffData[sample] = value / sum;
            }
        }

        return cumulativeNormalisedDifferenceBuffer;
    }

    float getLagEstimateFromCumulativeDifference (AudioSampleBuffer& cndBuffer, int channel, float threshold = 0.01f)
    {
        const int numSamples           = cndBuffer.getNumSamples() / 2;
        const float* cndData           = cndBuffer.getReadPointer (channel);
        float globalMinIndex           = -1.0f;
        float globalMin                = 100.0f;

        Point<float> interpolatedLagWithMinimumValue (-1.0f, 100.0f);
        for (int sample = 2; sample < numSamples; sample++)
        {
            if (cndData[sample] < globalMin)
            {
                globalMinIndex = (float) sample;
                globalMin = cndData[sample];
            }
            if (cndData[sample] < threshold)
            {
                while (sample + 1 < numSamples && cndData[sample + 1] < cndData[sample])
                {
                    sample ++;
                }
                interpolatedLagWithMinimumValue = getInterpolatedValleyFromCumulativeDifferenceLagEstimate (sample, cndBuffer, channel);
                break;
            }
        }

        normalisedLagPosition = Point<float> (interpolatedLagWithMinimumValue.getX() / cndBuffer.getNumSamples(), interpolatedLagWithMinimumValue.getY());
        return interpolatedLagWithMinimumValue.getX() == -1.0f ? globalMinIndex
                                                               : interpolatedLagWithMinimumValue.getX();
    }

    Point<float> getInterpolatedValleyFromCumulativeDifferenceLagEstimate (int lagEstimate, AudioSampleBuffer& cndBuffer, int channel)
    {
        const int numSamples = cndBuffer.getNumSamples() / 2;
        //I write out the priority using brackets for readability.
        int leftNeighbour  = (lagEstimate < 1) ? 0 : lagEstimate; 
        int rightNeighbour = lagEstimate + ((lagEstimate < numSamples + 1) ? 1 : 0);

        const float* cndData = cndBuffer.getReadPointer (channel);
        
        if (leftNeighbour == lagEstimate)/* ||' */
            return cndData[lagEstimate] <= cndData[rightNeighbour] ? Point<float> (lagEstimate, cndData[lagEstimate]) 
                                                                   : Point<float> (rightNeighbour, cndData[rightNeighbour]);
        
        if (rightNeighbour == lagEstimate)/* '|| */
            return cndData[lagEstimate] <= cndData[leftNeighbour] ? Point<float> (lagEstimate, cndData[lagEstimate])
                                                                  : Point<float> (leftNeighbour, cndData[leftNeighbour]);

        float leftSample = cndData[leftNeighbour];
        float sample = cndData[lagEstimate];
        float rightSample = cndData[rightNeighbour];

        const float interpolatedLagDifference = (rightSample - leftSample) / (2.0f * (2.0f * sample - leftSample - rightSample));
        const float interpolatedSample = cndData[lagEstimate] - 0.25f * (cndData[leftNeighbour] - cndData[rightNeighbour]) * interpolatedLagDifference;

        return Point<float> (interpolatedLagDifference, interpolatedSample);
    }

    double getPitchFromAutoCorrelationBuffer (AudioSampleBuffer& autoCorrelationBuffer)
    {
        const int numChannels  = autoCorrelationBuffer.getNumChannels();
        const int numLagValues = autoCorrelationBuffer.getNumSamples() / 4;

        int peakLag = getMaxIndex (autoCorrelationBuffer.getReadPointer (0), 1, numLagValues - 1);
        int lag = peakLag;
        jassert(lag > 0);

        double sampleRate = fft.getNyquist() * 2.0;

        //double cutoff = nyquist / filter.m;
        double f0Estimate = sampleRate / lag;

        DBG ("Peaklag: " << peakLag << " | Frequency estimation: "<<f0Estimate);

        return f0Estimate;
    }

    double estimatePitchFromFrequencyData (AudioSampleBuffer& frequencyData)
    {
        const int numChannels          = frequencyData.getNumChannels();
        const int numRealFrequencyBins = frequencyData.getNumSamples() / 2;

        int peakBin = getMaxIndex (frequencyData.getReadPointer (0), 1, numRealFrequencyBins);

        double frequencyRangePerBin = fft.getNyquist() / (double) numRealFrequencyBins;

        double midFreqOfMaxBin = (peakBin * frequencyRangePerBin) + (frequencyRangePerBin / 2.0);
        DBG ("Frequency estimation: "<<midFreqOfMaxBin);

        return midFreqOfMaxBin;
    }

    static int getMaxIndex (const float* data, int start, int numItems)
    {
        int indexOfMax = start;
        float currentMax = data[start];

        for (int i = start + 1; i < numItems; i++)
        {
            if (data[i] > currentMax)
            {
                indexOfMax = i;
                currentMax = data[i];
            }
        }
        return indexOfMax;
    }

    int getFFTExpectedSamples() const 
    {
        return fft.getFFTExpectedSamples();
    }

    

    void enableAutoCorrelationBufferToDrawNeedsUpdating()     { autoCorrelationDisplayBufferNeedsUpdating.set (1); }
    const AudioSampleBuffer getAutoCorrelationBufferToDraw()  { return AudioSampleBuffer (autoCorrelationBufferToDraw); }

    void enableCumulativeDifferenceBufferNeedsUpdating() { cumulativeDifferenceBufferNeedsUpdating.set (1); }
    const AudioSampleBuffer getCumulativeDifferenceBufferToDraw() { return AudioSampleBuffer (cumulativeDifferenceBufferToDraw); }

    Point<float> getNormalisedLagPosition() { return normalisedLagPosition; }

private:
    AudioSampleBuffer autoCorrelationBufferToDraw;
    AudioSampleBuffer cumulativeDifferenceBufferToDraw;
    FFTAnalyser&      fft;       
    Point<float>      normalisedLagPosition { 0.0f, 0.0f };
    
    Atomic<int>       autoCorrelationDisplayBufferNeedsUpdating;
    Atomic<int>       cumulativeDifferenceBufferNeedsUpdating;
    
};
#endif  // AUDIOFILTER_H_INCLUDED
