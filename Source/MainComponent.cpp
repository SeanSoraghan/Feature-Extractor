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
                               private ChangeListener,
                               public ListBoxModel
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
        view.setTracksModel (this);
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
        auto localBounds = getLocalBounds();
        const auto deviceSettingsHeight = audioDeviceSelector != nullptr ? audioDeviceSelector->getRequiredHeight() : 0;
        auto selectorBounds = localBounds.removeFromTop (deviceSettingsHeight);
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
            updateAnalysisTracksFromDeviceManager (adm);
        else
            jassertfalse;
    }

    //=======================================================================

    void setAudioSettingsDeviceManager (AudioDeviceManager& deviceManager)
    {
        const int minInputChannels              = 0;
        const int maxInputChannels              = 15;
        const int minOutputchannels             = 0;
        const int maxOutputchannels             = 0;
        const bool showMidiIn                   = false;
        const bool showMidiOut                  = false;
        const bool presentChannelsAsStereoPairs = false;
        
        std::function<void()> clearAllTracksCallback = [this]()
        {
            clearAllTracks();
        };

        addAndMakeVisible (audioDeviceSelector = new CustomAudioDeviceSelectorComponent (deviceManager, 
                                                                                         minInputChannels, maxInputChannels, 
                                                                                         minOutputchannels, maxOutputchannels, showMidiIn, showMidiOut, 
                                                                                         presentChannelsAsStereoPairs, clearAllTracksCallback));
        addAndMakeVisible (channelSelector = new ChannelSelectorPanel (audioDeviceSelector->getDeviceSetupDetails()));
        
        channelSelector->setChannelsConfigAboutToChangeCallback (clearAllTracksCallback);

        resized();
    }

    //=======================================================================

    void updateAnalysisTracksFromDeviceManager (AudioDeviceManager* dm)
    {
        clearAllTracks();

        const auto currentDevice = dm->getCurrentAudioDevice();
        if (currentDevice != nullptr)
        {
            const auto channelNames = currentDevice->getInputChannelNames();
            const auto numInputs = channelNames.size();
            const auto activeInputs  = currentDevice->getActiveInputChannels();
            const auto numActiveChannels = activeInputs.countNumberOfSetBits();

            auto activeInputBitNumber = activeInputs.findNextSetBit(0);
            auto inputChannel = 0;
            for (auto input = 0; input < numInputs; ++input)
            {
                String channelName = channelNames[input];
                if (activeInputBitNumber == input)
                {
                    addAnalyserTrack (inputChannel, channelName);
                    inputChannel ++;
                    activeInputBitNumber = activeInputs.findNextSetBit (activeInputBitNumber + 1);
                }
                //else
                //{
                //    addDisabledAnalyserTrack (channelName);
                //}
            }

            view.updateTracks();
        }
        
        resized();
    }

    void addAnalyserTrack (int channelToAnalyse, String channelName)
    {
        analyserControllers.add (new AnalyserTrackController (deviceManager, channelToAnalyse, channelName, "127.0.0.1:9000", "127.0.0.1:9000", String("/Audio/A") + String(channelToAnalyse)));
    }

    void addDisabledAnalyserTrack (String channelName)
    {
        analyserControllers.add (new AnalyserTrackController (deviceManager, -1, channelName, "127.0.0.1:9000", "127.0.0.1:9000", String::empty));
    }

    void clearAllTracks()
    {
        for (const auto track : analyserControllers)
            track->stopAnalysis();

        view.stopAnimation();
        analyserControllers.clear();
        view.updateTracks();
    }
    //=======================================================================

    int getNumRows() override 
    { 
        return analyserControllers.size();
    }

    void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        g.fillAll (FeatureExtractorLookAndFeel::getBackgroundColour());
    }

    Component* refreshComponentForRow (int rowNumber, bool /*isRowSelected*/,
                                               Component* existingComponentToUpdate) override
    {
        
        AnalyserTrack* existingView = dynamic_cast<AnalyserTrack*> (existingComponentToUpdate);
        if (existingView != nullptr)
            existingView->stopAnimation();

        if (rowNumber < analyserControllers.size())
        {
            AnalyserTrackController* trackController = analyserControllers.getUnchecked (rowNumber);
            if (trackController != nullptr)
            {
                trackController->clearGUICallbacks();
                if (existingView == nullptr)
                    existingView = new AnalyserTrack (trackController->getChannelName());
                trackController->linkGUIDisplayToTrack (existingView, audioDeviceSelector->getDeviceSetupDetails(), String ("/Audio/A") + String (rowNumber));
                return existingView;
            }
        }
        return existingComponentToUpdate;
    }

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
