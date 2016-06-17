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
    AudioDataCollector (int audioChannelToCollect)
    :   channelToCollect (audioChannelToCollect)
    {
        circleBuffer.setSize (1, 4096);
        circleBuffer.clear();
    }

    void audioDeviceAboutToStart (AudioIODevice*) override
    {}

    void audioDeviceStopped() override
    {}

    void audioDeviceIOCallback (const float** inputChannelData, int numInputChannels,
                                float** outputChannelData, int numOutputChannels,
                                int numberOfSamples) override
    {
        ignoreUnused (numInputChannels);
        ignoreUnused (numOutputChannels);
        const float** channelData = collectInput ? inputChannelData : outputChannelData;

        if (collectInput)
            jassert (numInputChannels >= channelToCollect);
        if (!collectInput)
            jassert (numOutputChannels >= channelToCollect);
        if (channelToCollect > 0)
            jassert (numInputChannels >= channelToCollect);

        analysisBufferUpdating.set (1);

        if (writeIndex + numberOfSamples <= circleBuffer.getNumSamples())
        {
            circleBuffer.copyFrom (0, writeIndex, channelData[channelToCollect], numberOfSamples);
        }
        else
        {
            for (int index = 0; index < numberOfSamples; index++)
            {
                const int modIndex = (index + writeIndex) % circleBuffer.getNumSamples();
                circleBuffer.setSample (0, modIndex, channelData[channelToCollect][index]);
            }
        }
        
        writeIndex = (writeIndex + numberOfSamples) % circleBuffer.getNumSamples();

        analysisBufferUpdating.set (0);

        if (notifyAnalysisThread != nullptr)
            notifyAnalysisThread();
    }

    AudioSampleBuffer getAnalysisBuffer (int numSamplesRequired)
    {
        AudioSampleBuffer buffer = AudioSampleBuffer();
        buffer.clear();
        buffer.setSize (1, numSamplesRequired);

        while (analysisBufferUpdating.get() == 1) {}
        if (analysisBufferUpdating.get() != 1)
        {
            readIndex = (circleBuffer.getNumSamples() + (writeIndex - numSamplesRequired)) % circleBuffer.getNumSamples();
            
            if (readIndex + numSamplesRequired <= circleBuffer.getNumSamples())
            {
                for (int index = 0; index < numSamplesRequired; index++)
                {
                    int rIndex = index + readIndex;
                    buffer.setSample (0, index, circleBuffer.getReadPointer (0)[rIndex] * gain);
                }
                //buffer.copyFrom (0, 0, circleBuffer, 0, readIndex, numSamplesRequired);
                //buffer.copyFrom (1, 0, circleBuffer, 1, spectralReadPosition, numSamplesRequired);
            }
            else
            {
                for (int index = 0; index < numSamplesRequired; index++)
                {
                    const int modIndex = (index + readIndex) % circleBuffer.getNumSamples();
                    buffer.setSample (0, index, circleBuffer.getReadPointer (0)[modIndex] * gain);
                    //buffer.setSample (1, index, circleBuffer.getReadPointer (1)[modIndex]);
                }
            }
            updateBufferToDraw (buffer);
        }
        return buffer;
    }
    
    void updateBufferToDraw (AudioSampleBuffer& buffer)
    {
        if (bufferToDrawUpdated != nullptr)
        {
            bufferToDrawUpdated (buffer);
        }
    }

    bool indexesOverlap (int numSamplesToFill)
    {
        if (writeIndex < readIndex)
            if (readIndex < writeIndex + expectedSamplesPerBlock)
                return true;
        if (readIndex < writeIndex)
            if (writeIndex < readIndex + numSamplesToFill)
                return true;
        return false;
            
    }

    void setBufferToDrawUpdatedCallback  (std::function<void (AudioSampleBuffer&)> f) { bufferToDrawUpdated = f; }
    void setNotifyAnalysisThreadCallback (std::function<void()> f)                    { notifyAnalysisThread = f; }

    void toggleCollectInput         (bool shouldCollectInput) noexcept { clearBuffer(); collectInput = shouldCollectInput; }
    void setExpectedSamplesPerBlock (int spb)                 noexcept { expectedSamplesPerBlock = spb; }

    void clearBuffer() { circleBuffer.clear(); }
    void setChannelToCollect (int c) { channelToCollect = c; }
    void setGain (float g) { gain = g; }
private:
    AudioSampleBuffer                        circleBuffer;
    std::function<void (AudioSampleBuffer&)> bufferToDrawUpdated;
    std::function<void()>                    notifyAnalysisThread;
    float gain                               { 1.0f };
    Atomic<int> analysisBufferUpdating       { 0 };
    int writeIndex                           { 0 };
    int readIndex                            { 0 };
    int expectedSamplesPerBlock              { 512 };
    int channelToCollect                     { 0 };
    bool collectInput                        { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDataCollector)
};



#endif  // AUDIODATACOLLECTOR_H_INCLUDED
