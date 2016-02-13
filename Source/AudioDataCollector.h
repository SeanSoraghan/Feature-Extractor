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
        const float** channelData = collectInput ? inputChannelData : outputChannelData;
        if (writeIndex + numberOfSamples <= circleBuffer.getNumSamples())
        {
            circleBuffer.addFrom (0, writeIndex, channelData[0], numberOfSamples);
            circleBuffer.addFrom (1, writeIndex, channelData[1], numberOfSamples);
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
    }

    AudioSampleBuffer getSpectralAnalysisBuffer (int numSamplesRequired)
    {
        while (indexesOverlap (numSamplesRequired))
        {}

        AudioSampleBuffer buffer = AudioSampleBuffer();
        buffer.clear();
        buffer.setSize (2, numSamplesRequired);
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
        spectralReadPosition = (spectralReadPosition + numSamplesRequired) % circleBuffer.getNumSamples();
        return buffer;
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
    void toggleCollectInput         (bool shouldCollectInput) noexcept { clearBuffer(); collectInput = shouldCollectInput; }
    void setExpectedSamplesPerBlock (int spb)                 noexcept { expectedSamplesPerBlock = spb; }
    void clearBuffer() { circleBuffer.clear(); }
private:
    AudioSampleBuffer circleBuffer;
    int writeIndex              { 0 };
    int spectralReadPosition    { 0 };
    int expectedSamplesPerBlock { 512 };
    bool collectInput           { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDataCollector)
};



#endif  // AUDIODATACOLLECTOR_H_INCLUDED
