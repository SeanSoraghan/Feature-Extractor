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

        const auto trackHeight = 100;

        for (const auto& track : analysers)
            track->setBounds (localBounds.removeFromTop (trackHeight));
    }

    void addAnalyserTrack (AnalyserTrackController& controller, String channelName, CustomAudioDeviceSetupDetails& deviceSetupDetails)
    {
        addAndMakeVisible (analysers.add (new AnalyserTrack (channelName)));
        controller.linkGUIDisplayToTrack (analysers.getLast(), deviceSetupDetails);
        resized();
    }

    void addDisabledAnalyserTrack (String channelName)
    {
        analysers.add (new AnalyserTrack (channelName));
        analysers.getLast()->setEnabled (false);
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
    
    OwnedArray<AnalyserTrack>                         analysers;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};



#endif  // MAINVIEW_H_INCLUDED
