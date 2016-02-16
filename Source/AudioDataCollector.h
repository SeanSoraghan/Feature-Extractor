/*
  ==============================================================================

    AudioDataCollector.h
    Created: 10 Feb 2016 8:02:00pm
    Author:  Sean

  ==============================================================================
*/

#ifndef AUDIODATACOLLECTOR_H_INCLUDED
#define AUDIODATACOLLECTOR_H_INCLUDED

//==============================================================================
/*
    Periodically collects the incoming audio into a buffer that is used for audio feature extraction
*/
class AudioDataCollector : public AudioIODeviceCallback
{
public:
    AudioDataCollector() 
    {
        circleBuffer.setSize (2, 4096);
        circleBuffer.clear();
    }

    void audioDeviceAboutToStart (AudioIODevice*) override
    {}

    void audioDeviceStopped() override
    {}

    void audioDeviceIOCallback (const float** inputChannelData, int /*numInputChannels*/,
                                float** outputChannelData, int /*numOutputChannels*/,
                                int numberOfSamples) override
    {
        if (analysisBufferNeedsUpdating.get() == 1)
        {
            const float** channelData = collectInput ? inputChannelData : outputChannelData;

            analysisBufferUpdating.set (1);

            if (writeIndex + numberOfSamples <= circleBuffer.getNumSamples())
            {
                circleBuffer.copyFrom (0, writeIndex, channelData[0], numberOfSamples);
                circleBuffer.copyFrom (1, writeIndex, channelData[1], numberOfSamples);
            }
            else
            {
                for (int index = 0; index < numberOfSamples; index++)
                {
                    const int modIndex = (index + writeIndex) % circleBuffer.getNumSamples();
                    circleBuffer.setSample (0, modIndex, channelData[0][index]);
                    circleBuffer.setSample (1, modIndex, channelData[1][index]);
                }
            }
        
            writeIndex = (writeIndex + numberOfSamples) % circleBuffer.getNumSamples();

            analysisBufferNeedsUpdating.set (0);
            analysisBufferUpdating.set (0);
        }
    }

    AudioSampleBuffer getSpectralAnalysisBuffer (int numSamplesRequired)
    {
        AudioSampleBuffer buffer = AudioSampleBuffer();
        buffer.clear();
        buffer.setSize (2, numSamplesRequired);

        while (analysisBufferUpdating.get() == 1) {}
        if (analysisBufferUpdating.get() != 1)
        {
            spectralReadPosition = (circleBuffer.getNumSamples() + (writeIndex - numSamplesRequired)) % circleBuffer.getNumSamples();
            
            if (spectralReadPosition + numSamplesRequired <= circleBuffer.getNumSamples())
            {
                buffer.copyFrom (0, 0, circleBuffer, 0, spectralReadPosition, numSamplesRequired);
                buffer.copyFrom (1, 0, circleBuffer, 1, spectralReadPosition, numSamplesRequired);
            }
            else
            {
                for (int index = 0; index < numSamplesRequired; index++)
                {
                    const int modIndex = (index + spectralReadPosition) % circleBuffer.getNumSamples();
                    buffer.setSample (0, index, circleBuffer.getReadPointer (0)[modIndex]);
                    buffer.setSample (1, index, circleBuffer.getReadPointer (1)[modIndex]);
                }
            }
            updateSpectralBufferVisualiser (buffer);
        }
        analysisBufferNeedsUpdating.set (1);
        return buffer;
    }
    
    void updateSpectralBufferVisualiser (AudioSampleBuffer& buffer)
    {
        if (spectralBufferUpdated != nullptr)
        {
            spectralBufferUpdated (buffer);
        }
    }

    bool indexesOverlap (int numSamplesToFill)
    {
        if (writeIndex < spectralReadPosition)
            if (spectralReadPosition < writeIndex + expectedSamplesPerBlock)
                return true;
        if (spectralReadPosition < writeIndex)
            if (writeIndex < spectralReadPosition + numSamplesToFill)
                return true;
        return false;
            
    }
    void setSpectralBufferUpdatedCallback (std::function<void (AudioSampleBuffer&)> f) { spectralBufferUpdated = f; }

    void toggleCollectInput         (bool shouldCollectInput) noexcept { clearBuffer(); collectInput = shouldCollectInput; }
    void setExpectedSamplesPerBlock (int spb)                 noexcept { expectedSamplesPerBlock = spb; }
    void clearBuffer() { circleBuffer.clear(); }
private:
    AudioSampleBuffer                        circleBuffer;
    std::function<void (AudioSampleBuffer&)> spectralBufferUpdated;
    int writeIndex                           { 0 };
    int spectralReadPosition                 { 0 };
    int expectedSamplesPerBlock              { 512 };
    Atomic<int> analysisBufferUpdating       { 0 };
    Atomic<int> analysisBufferNeedsUpdating  { 1 };
    bool collectInput                        { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDataCollector)
};



#endif  // AUDIODATACOLLECTOR_H_INCLUDED
