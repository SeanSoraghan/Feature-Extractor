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
        setLookAndFeel (lookAndFeel);
        setSize (800, 600);
        // specify the number of input and output channels that we want to open
        setAudioChannels  (2, 2);
        
        deviceManager.addChangeListener (this);
        
        setAudioSettingsDeviceManager (deviceManager);
        //deviceManager.addAudioCallback (&view.getAudioDisplayComponent());
        addAndMakeVisible (view);

        updateAnalysisTracksFromDeviceManager (&deviceManager);

        
    }

    ~MainContentComponent()
    {
        deviceManager.removeChangeListener (this);
        shutdownAudio();
    }

    //=======================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        for (const auto track : analyserControllers)
            track->prepareToPlay (samplesPerBlockExpected, sampleRate);
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
        auto& localBounds = getLocalBounds();
        const auto testH = localBounds.getHeight() / 2;
        auto& selectorBounds = localBounds.removeFromTop (testH);
        const auto channelSelectorWidth = selectorBounds.getWidth() / 2;
        const auto& channelSelectorBounds = selectorBounds.removeFromLeft (channelSelectorWidth);

        if (channelSelector != nullptr)
            channelSelector->setBounds (channelSelectorBounds);
        if (audioDeviceSelector != nullptr)
            audioDeviceSelector->setBounds (selectorBounds);

        view.setBounds (localBounds);
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

    //=======================================================================

    void setAudioSettingsDeviceManager (AudioDeviceManager& deviceManager)
    {
        const int minInputChannels              = 1;
        const int maxInputChannels              = 4;
        const int minOutputchannels             = 1;
        const int maxOutputchannels             = 4;
        const bool showMidiIn                   = false;
        const bool showMidiOut                  = false;
        const bool presentChannelsAsStereoPairs = false;
        const bool hideAdvancedSettings         = true;
        addAndMakeVisible (audioDeviceSelector = new CustomAudioDeviceSelectorComponent (deviceManager, 
                                                                                         minInputChannels, maxInputChannels, 
                                                                                         minOutputchannels, maxOutputchannels, showMidiIn, showMidiOut, 
                                                                                         presentChannelsAsStereoPairs, hideAdvancedSettings));
        addAndMakeVisible (channelSelector = new ChannelSelectorPanel (audioDeviceSelector->getDeviceSetupDetails()));
        resized();
    }

    //=======================================================================

    void updateAnalysisTracksFromDeviceManager (AudioDeviceManager* dm)
    {
        clearAllTracks();

        const auto currentDevice = dm->getCurrentAudioDevice();
        const auto activeInputs  = currentDevice->getActiveInputChannels();
        int channel = activeInputs.findNextSetBit (0);
        while (channel >= 0)
        {
            addAnalyserTrack (channel);
            DBG("Set Channel: "<<channel);
            channel = activeInputs.findNextSetBit (channel + 1);            
        }
        resized();
    }

    void addAnalyserTrack (int channelToAnalyse)
    {
        analyserControllers.add (new AnalyserTrackController (deviceManager, channelToAnalyse));
        view.addAnalyserTrack (*analyserControllers.getLast(), audioDeviceSelector->getDeviceSetupDetails());
    }

    void clearAllTracks()
    {
        for (const auto track : analyserControllers)
            track->stopAnalysis();

        analyserControllers.clear();
        view.clearAnalyserTracks();
    }
    //=======================================================================

private:
    SharedResourcePointer<FeatureExtractorLookAndFeel> lookAndFeel;
    ScopedPointer<ChannelSelectorPanel>                channelSelector;
    ScopedPointer<CustomAudioDeviceSelectorComponent>  audioDeviceSelector;
    OwnedArray<AnalyserTrackController>                analyserControllers;
    MainView                                           view;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
