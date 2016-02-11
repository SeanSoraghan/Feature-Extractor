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
    :   featureListView     (featureListModel)
    {
        addAndMakeVisible (audioDeviceSelector);
        addAndMakeVisible (audioScrollingDisplay);

        for (int i = 0; i < (int) ConcatenatedFeatureBuffer::Feature::ZeroCrosses; i++)
            featureListModel.addFeature ((ConcatenatedFeatureBuffer::Feature) i);
        
        featureListView.recreateVisualisersFromModel();
        
        addAndMakeVisible (featureListView);
    }

    void setAudioSettingsDeviceManager (AudioDeviceManager& deviceManager)
    {
        addAndMakeVisible (audioDeviceSelector = new AudioDeviceSelectorComponent (deviceManager, 2, 2, 2, 2, false, false, true, true));
        resized();
    }

    void resized() override
    {
        auto localBounds = getLocalBounds();
        const int verticalMargin           = (int) (FeatureExtractorLookAndFeel::getVerticalMargin());
        const int horizontalMargin         = (int) (FeatureExtractorLookAndFeel::getHorizontalMargin());

        const int availableHeight          = localBounds.getHeight() - 2 * verticalMargin;
        const int settingsHeight           = (int) (FeatureExtractorLookAndFeel::getSettingsHeightRatio() * availableHeight);
        const int displayHeight            = (int) (FeatureExtractorLookAndFeel::getAudioDisplayHeightRatio() * availableHeight);
        const int featureVisualiserHeight  = (int) (FeatureExtractorLookAndFeel::getFeatureVisualiserComponentHeightRatio() * availableHeight);

        audioScrollingDisplay.setBounds (localBounds.removeFromTop (displayHeight));
        localBounds.removeFromTop       (verticalMargin);
        featureListView.setBounds       (localBounds.removeFromTop (featureVisualiserHeight));
        localBounds.removeFromTop       (verticalMargin);

        auto settingsBounds = localBounds.removeFromTop (settingsHeight);
        const auto panelWidth = (settingsBounds.getWidth() - horizontalMargin) / 2;
        if (audioDeviceSelector)
            audioDeviceSelector->setBounds (settingsBounds.removeFromLeft (panelWidth));
    }

    LiveScrollingAudioDisplay& getAudioDisplayComponent()
    {
        return audioScrollingDisplay;
    }

    void setFeatureValueQueryCallback (std::function<float (ConcatenatedFeatureBuffer::Feature)> f)
    {
        featureListView.setFeatureValueQueryCallback (f);
    }

    void setAudioDataStreamToggleCallback (std::function<void (bool)> f)
    {
    }

private:
    LiveScrollingAudioDisplay                   audioScrollingDisplay;
    ScopedPointer<AudioDeviceSelectorComponent> audioDeviceSelector;
    FeatureListModel                            featureListModel;
    FeatureListView                             featureListView;
       
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};



#endif  // MAINVIEW_H_INCLUDED
