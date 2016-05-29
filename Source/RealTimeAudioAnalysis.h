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
    DBG("Buffer");
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

    float getTotal() const
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

    static void scaleBufferWithBartlettWindowing (AudioSampleBuffer& source) // in place
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

    /* Be sure to filter / window the audio data as you require before hand */
    AudioSampleBuffer getFrequencyData (AudioSampleBuffer& timeBuffer) 
    {
        const int numChannels = timeBuffer.getNumChannels();
        const int numSamples  = timeBuffer.getNumSamples();

        AudioSampleBuffer frequencyBuffer (timeBuffer);

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
    void setNyquistValue (double newValue)        { nyquist = newValue; }

    const AudioSampleBuffer getFFTBufferToDraw() const { return AudioSampleBuffer (fftBufferToDraw); }
    int getFFTExpectedSamples()                  const { return fft.getFFTExpectedSamples(); }
    const RealTimeFFT& getFFTObject()            const { return fft; }
    double getNyquist()                          const { return nyquist; }

private:
    RealTimeFFT       fft; 
    AudioSampleBuffer fftBufferToDraw;
    double            nyquist;
    Atomic<int>       fftDisplayBufferNeedsUpdating;
};


#endif  // AUDIOFILTER_H_INCLUDED
