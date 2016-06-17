/*
  ==============================================================================

    MainView.h
    Created: 10 Feb 2016 8:01:40pm
    Author:  Sean

  ==============================================================================
*/

#ifndef MAINVIEW_H_INCLUDED
#define MAINVIEW_H_INCLUDED

//==============================================================================
/*
    Main view that holds all the individual controls and displays.
*/
class MainView : public Component
{
public:
    MainView ()
    {
        setLookAndFeel (SharedResourcePointer<FeatureExtractorLookAndFeel>());
    }

    ~MainView() {}

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

        const auto trackHeight = 100;

        for (const auto& track : analysers)
            track->setBounds (localBounds.removeFromTop (trackHeight));
    }

    

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

    void addAnalyserTrack (AnalyserTrackController& controller)
    {
        addAndMakeVisible (analysers.add (new AnalyserTrack()));
        controller.linkGUIDisplayToTrack (analysers.getLast(), audioDeviceSelector->getDeviceSetupDetails());
        resized();
    }

    void clearAnalyserTracks()
    {
        analysers.clear();
    }

    OwnedArray<AnalyserTrack>& getAnalysers()
    {
        return analysers;
    }

private:
    ScopedPointer<ChannelSelectorPanel>               channelSelector;
    ScopedPointer<CustomAudioDeviceSelectorComponent> audioDeviceSelector;
    OwnedArray<AnalyserTrack>                         analysers;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};



#endif  // MAINVIEW_H_INCLUDED
