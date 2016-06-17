/*
  ==============================================================================

    PitchAnalyser.h
    Created: 24 Apr 2016 3:55:24pm
    Author:  Sean

  ==============================================================================
*/

#ifndef PITCHANALYSER_H_INCLUDED
#define PITCHANALYSER_H_INCLUDED

//============================================================================================================================================================
//============================================================================================================================================================

class PitchAnalyser
{
public:
    PitchAnalyser (FFTAnalyser& ffT) 
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
        const double pitchEstimate = (fft.getNyquist() * 2.0f) / lagEstimate;
        return pitchEstimate;
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
                const float value = autoCorrelationData[sample];
                sum += value;
                if (sum != 0.0f)
                {
                    const auto norm = (1.0f / sample) * sum;
                    cumulativeDiffData[sample] = value / sum;
                }
                else
                {
                    cumulativeDiffData[sample] = 0.0f;
                }
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
            return cndData[lagEstimate] <= cndData[rightNeighbour] ? Point<float> ((float) lagEstimate, cndData[lagEstimate]) 
                                                                   : Point<float> ((float) rightNeighbour, cndData[rightNeighbour]);
        
        if (rightNeighbour == lagEstimate)/* '|| */
            return cndData[lagEstimate] <= cndData[leftNeighbour] ? Point<float> ((float) lagEstimate, cndData[lagEstimate])
                                                                  : Point<float> ((float) leftNeighbour, cndData[leftNeighbour]);

        float leftSample = cndData[leftNeighbour];
        float sample = cndData[lagEstimate];
        float rightSample = cndData[rightNeighbour];

        const float interpolatedLagDifference = (rightSample - leftSample) / (2.0f * (2.0f * sample - leftSample - rightSample));
        const float interpolatedSample = cndData[lagEstimate] - 0.25f * (cndData[leftNeighbour] - cndData[rightNeighbour]) * interpolatedLagDifference;

        return Point<float> (interpolatedLagDifference, interpolatedSample);
    }
    
};



#endif  // PITCHANALYSER_H_INCLUDED
