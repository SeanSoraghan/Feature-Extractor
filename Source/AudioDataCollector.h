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
    AudioDataCollector() {}

    void audioDeviceAboutToStart (AudioIODevice*) override
    {}

    void audioDeviceStopped() override
    {}

    void audioDeviceIOCallback (const float** inputChannelData, int /*numInputChannels*/,
                                float** outputChannelData, int /*numOutputChannels*/,
                                int numberOfSamples) override
    {
        const float** channelData = collectInput ? inputChannelData : outputChannelData;
        if (spectralAnalysisBufferNeedsUpdating.get() == 1)
        {
            spectralAnalysisBuffer.setSize (2, numberOfSamples);
            spectralAnalysisBuffer.clear();
            spectralAnalysisBuffer.addFrom (0, 0, inputChannelData[0], numberOfSamples);
            spectralAnalysisBuffer.addFrom (1, 0, inputChannelData[1], numberOfSamples);

            spectralAnalysisBufferNeedsUpdating.set (0);
        }

        if (harmonicAnalysisBufferNeedsUpdating.get() == 1)
        {
            if (harmonicAnalysisBufferPosition == 0)
                harmonicAnalysisBuffer.setSize (2, 0);

            harmonicAnalysisBuffer.setSize (2, harmonicAnalysisBuffer.getNumSamples() + numberOfSamples, true/*keep existing content*/, true/*clear exrtra space*/);
            harmonicAnalysisBuffer.addFrom (0, harmonicAnalysisBuffer.getNumSamples() - numberOfSamples, inputChannelData[0], numberOfSamples);
            harmonicAnalysisBuffer.addFrom (1, harmonicAnalysisBuffer.getNumSamples() - numberOfSamples, inputChannelData[1], numberOfSamples);

            harmonicAnalysisBufferPosition ++;

            if (harmonicAnalysisBufferPosition >= 8)
            {
                harmonicAnalysisBufferNeedsUpdating.set (0);
                harmonicAnalysisBufferPosition = 0;
            }
        }
    }

    AudioSampleBuffer getHarmonicAnalysisBuffer()
    {
        if (harmonicAnalysisBufferNeedsUpdating.get() == 0)
            return harmonicAnalysisBuffer;

        return AudioSampleBuffer();
    }

    AudioSampleBuffer getSpectralAnalysisBuffer()
    {
        if (spectralAnalysisBufferNeedsUpdating.get() == 0)
            return spectralAnalysisBuffer;

        return AudioSampleBuffer();
    }

    void toggleCollectInput (bool shouldCollectInput) { collectInput = shouldCollectInput; }

    Atomic<int> harmonicAnalysisBufferNeedsUpdating { 0 };
    Atomic<int> spectralAnalysisBufferNeedsUpdating { 0 };

private:
    bool collectInput { true };
    AudioSampleBuffer spectralAnalysisBuffer;
    AudioSampleBuffer harmonicAnalysisBuffer;
    int harmonicAnalysisBufferPosition { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDataCollector)
};



#endif  // AUDIODATACOLLECTOR_H_INCLUDED
