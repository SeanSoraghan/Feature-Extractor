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
        tracks.setColour (ListBox::ColourIds::backgroundColourId, FeatureExtractorLookAndFeel::getBackgroundColour());
        setLookAndFeel (SharedResourcePointer<FeatureExtractorLookAndFeel>());
        addAndMakeVisible (tracks);
        tracks.setRowHeight (FeatureExtractorLookAndFeel::getAnalyserTrackHeight());
    }

    ~MainView() {}

    void paint (Graphics& g) override
    {
        g.fillAll (FeatureExtractorLookAndFeel::getBackgroundColour());
    }

    void resized() override
    {
        tracks.setBounds (getLocalBounds());
    }

    //void addAnalyserTrack (AnalyserTrackController& controller, String channelName, CustomAudioDeviceSetupDetails& deviceSetupDetails)
    //{
    //    addAndMakeVisible (analysers.add (new AnalyserTrack (channelName)));
    //    controller.linkGUIDisplayToTrack (analysers.getLast(), deviceSetupDetails);
    //    resized();
    //}

    //void addDisabledAnalyserTrack (String channelName)
    //{
    //    analysers.add (new AnalyserTrack (channelName));
    //    analysers.getLast()->setEnabled (false);
    //}

    void setTracksModel (ListBoxModel* m) { tracks.setModel (m); }
    void updateTracks()                   { tracks.updateContent(); }
private:
    ListBox                    tracks;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};



#endif  // MAINVIEW_H_INCLUDED
