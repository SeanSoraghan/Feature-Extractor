/*
  ==============================================================================

    AudioFilePlayer.h
    Created: 12 Feb 2016 3:30:43pm
    Author:  Sean

  ==============================================================================
*/

#ifndef AUDIOFILEPLAYER_H_INCLUDED
#define AUDIOFILEPLAYER_H_INCLUDED

class AudioFilePlayer
{
public:
    AudioFilePlayer ()
    :   thread ("audio preload thread")
    {
        formatManager.registerBasicFormats();
    }

    ~AudioFilePlayer()
    {
        audioTransportSource.setSource (nullptr);
        audioSourcePlayer.setSource (nullptr);
    }
    void setupAudioCallback (AudioDeviceManager& deviceManager)
    {
        deviceManager.addAudioCallback (&audioSourcePlayer);
        audioSourcePlayer.setSource    (&audioTransportSource);
    }

    void loadFileIntoTransport (const File& audioFile)
    {
        // unload the previous file source and delete it..
        audioTransportSource.stop();
        audioTransportSource.setSource (nullptr);
        currentAudioFileSource = nullptr;

        AudioFormatReader* reader = formatManager.createReaderFor (audioFile);

        if (reader != nullptr)
        {
            currentAudioFileSource = new AudioFormatReaderSource (reader, true);

            // ..and plug it into our transport source
            audioTransportSource.setSource (currentAudioFileSource,
                                            32768,                   // tells it to buffer this many samples ahead
                                            &thread,                 // this is the background thread to use for reading-ahead
                                            reader->sampleRate);     // allows for sample rate correction
        }
    }

private:
    AudioFormatManager                     formatManager;
    TimeSliceThread                        thread;
    AudioSourcePlayer                      audioSourcePlayer;
    AudioTransportSource                   audioTransportSource;
    ScopedPointer<AudioFormatReaderSource> currentAudioFileSource;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFilePlayer)
};



#endif  // AUDIOFILEPLAYER_H_INCLUDED
