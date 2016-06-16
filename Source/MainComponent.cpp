/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "include.h"

//==============================================================================
/*
    Application for linking up audio device manager with different GUI elements
*/
class MainContentComponent   : public  AudioAppComponent,
                               private ChangeListener
{
public:
    //==============================================================================
    MainContentComponent() 
    
    {
        setSize (800, 600);
        // specify the number of input and output channels that we want to open
        setAudioChannels  (2, 2);
        
        deviceManager.addChangeListener (this);
        
        view.setAudioSettingsDeviceManager (deviceManager);
        //deviceManager.addAudioCallback (&view.getAudioDisplayComponent());
        updateAnalysisTracksFromDeviceManager (&deviceManager);

        addAndMakeVisible (view);
    }

    ~MainContentComponent()
    {
        deviceManager.removeChangeListener (this);
        shutdownAudio();
    }

    //=======================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        //call prepare to play on all tracks.
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        // Your audio-processing code goes here!

        // For more details, see the help for AudioProcessor::getNextAudioBlock()

        // Right now we are not producing any data, in which case we need to clear the buffer
        // (to prevent the output of random noise)
        bufferToFill.clearActiveBufferRegion();
    }

    void releaseResources() override
    {
        // This will be called when the audio device stops, or when it is being
        // restarted due to a setting change.

        // For more details, see the help for AudioProcessor::releaseResources()
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
    }

    void resized() override
    {
        view.setBounds (getLocalBounds());
    }

    void addAnalyserTrack (int channelToAnalyse)
    {
        analyserControllers.add (new AnalyserTrackController (deviceManager, channelToAnalyse));
        view.addAnalyserTrack (*analyserControllers.getLast());
    }

    void changeListenerCallback (ChangeBroadcaster* broadcaster) override
    {
        juce::AudioDeviceManager* adm = dynamic_cast<juce::AudioDeviceManager*>(broadcaster);
        if (adm != nullptr)
        {
            updateAnalysisTracksFromDeviceManager (adm);
        }
        else
        {
            jassertfalse;
        }
    }

    void updateAnalysisTracksFromDeviceManager (AudioDeviceManager* dm)
    {
        analyserControllers.clear();
        view.clearAnalyserTracks();
        const auto currentDevice = dm->getCurrentAudioDevice();
        const auto activeInputs  = currentDevice->getActiveInputChannels();
        int channel = activeInputs.findNextSetBit (0);
        while (channel >= 0)
        {
            addAnalyserTrack (channel);
            DBG("Set Channel: "<<channel);
            channel = activeInputs.findNextSetBit (channel + 1);            
        }
    }

private:
    SharedResourcePointer<FeatureExtractorLookAndFeel> lookAndFeel;
    OwnedArray<AnalyserTrackController>                analyserControllers;
    MainView                                           view;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
