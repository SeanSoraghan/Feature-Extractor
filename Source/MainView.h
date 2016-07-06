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

    void stopAnimation()
    {
        for (int t = 0; t < tracks.getModel()->getNumRows(); ++t)
        {
            auto track = dynamic_cast<AnalyserTrack*> (tracks.getComponentForRowNumber(t));
            if (track != nullptr)
                track->stopAnimation();
            //else
            //    jassertfalse;
        }
    }

    void setTracksModel (ListBoxModel* m) { tracks.setModel (m); }
    void updateTracks()                   { tracks.updateContent(); }
private:
    ListBox                    tracks;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};



#endif  // MAINVIEW_H_INCLUDED
