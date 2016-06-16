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
        thread.startThread (3);
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

    AudioSourcePlayer* getAudioSourcePlayer()
    {
        return &audioSourcePlayer;
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
            currentAudioFileSource->setLooping (true);
            // ..and plug it into our transport source
            audioTransportSource.setSource (currentAudioFileSource,
                                            32768,                   // tells it to buffer this many samples ahead
                                            &thread,                 // this is the background thread to use for reading-ahead
                                            reader->sampleRate);     // allows for sample rate correction
        }
    }

    void play ()   { audioTransportSource.start(); }
    void pause()   { audioTransportSource.stop(); }
    void stop()    { pause(); audioTransportSource.setPosition (0.0); }
    void restart() { audioTransportSource.setPosition (0.0); }

    bool hasFile() { return currentAudioFileSource != nullptr; }

private:
    AudioFormatManager                     formatManager;
    TimeSliceThread                        thread;
    AudioSourcePlayer                      audioSourcePlayer;
    AudioTransportSource                   audioTransportSource;
    ScopedPointer<AudioFormatReaderSource> currentAudioFileSource;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFilePlayer)
};



#endif  // AUDIOFILEPLAYER_H_INCLUDED
