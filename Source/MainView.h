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
        setLookAndFeel (SharedResourcePointer<FeatureExtractorLookAndFeel>());
        addAndMakeVisible (audioDeviceSelector);
        addAndMakeVisible (audioScrollingDisplay);

        for (int i = 0; i < (int) OSCFeatureAnalysisOutput::OSCFeatureType::F0; i++)
            featureListModel.addFeature ((OSCFeatureAnalysisOutput::OSCFeatureType) i);
        
        featureListView.recreateVisualisersFromModel();
        
        addAndMakeVisible (featureListView);

        addAndMakeVisible (oscSettingsController.getView());

        addAndMakeVisible (audioFileTransportController.getView());
        audioFileTransportController.getView().setVisible (false);

        addAndMakeVisible (audioSourceTypeSelectorController.getSelector());
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

        audioScrollingDisplay.setBounds                  (localBounds.removeFromTop (displayHeight));
        audioFileTransportController.getView().setBounds (audioScrollingDisplay.getBounds());
        localBounds.removeFromTop                        (verticalMargin);
        featureListView.setBounds                        (localBounds.removeFromTop (featureVisualiserHeight));
        localBounds.removeFromTop                        (verticalMargin);

        auto settingsBounds = localBounds.removeFromTop (settingsHeight).reduced (horizontalMargin, 0);
        const auto panelWidth = (settingsBounds.getWidth() - horizontalMargin) / 2;
        if (audioDeviceSelector)
        {
            audioDeviceSelector->setBounds (settingsBounds.removeFromLeft (panelWidth));
            settingsBounds.removeFromLeft  (horizontalMargin);
        }
        oscSettingsController.getView().setBounds (settingsBounds.removeFromLeft (panelWidth));

        auto displayBounds = audioScrollingDisplay.getBounds();
        auto selectorBounds = displayBounds.removeFromLeft (FeatureExtractorLookAndFeel::getAudioSourceTypeSelectorWidth())
                                           .removeFromTop  (FeatureExtractorLookAndFeel::getAudioSourceTypeSelectorHeight());

        audioSourceTypeSelectorController.getSelector().setBounds (selectorBounds);
    }

    LiveScrollingAudioDisplay& getAudioDisplayComponent()
    {
        return audioScrollingDisplay;
    }

    void setAudioSettingsDeviceManager (AudioDeviceManager& deviceManager)
    {
        addAndMakeVisible (audioDeviceSelector = new AudioDeviceSelectorComponent (deviceManager, 2, 2, 2, 2, false, false, true, true));
        resized();
    }

    void setFeatureValueQueryCallback (std::function<float (OSCFeatureAnalysisOutput::OSCFeatureType)> f)
    {
        featureListView.setFeatureValueQueryCallback (f);
    }

    void setAudioSourceTypeChangedCallback (std::function<void (AudioSourceSelectorController::eAudioSourceType type)> f)
    {
        audioSourceTypeSelectorController.setAudioSourceTypeChangedCallback (f);
    }

    void setAddressChangedCallback (std::function<bool (String)> f) { oscSettingsController.setAddressChangedCallback (f); }

    void setDisplayedOSCAddress (String address)
    {
        oscSettingsController.getView().getAddressEditor().setText (address);
    }

    void setBundleAddressChangedCallback (std::function<void (String)> f) { oscSettingsController.setBundleAddressChangedCallback (f); }

    void setDisplayedBundleAddress (String address)
    {
        oscSettingsController.getView().getBundleAddressEditor().setText (address);
    }

    void setFileDroppedCallback (std::function<void (File&)> f) { audioFileTransportController.setFileDroppedCallback (f); }

    void toggleShowTransportControls (bool shouldShowControls)
    {
        audioFileTransportController.getView().setVisible (shouldShowControls);
    }

    void clearAudioDisplayData()
    {
        audioScrollingDisplay.clear();
    }

    void setAudioTransportState (AudioFileTransportController::eAudioTransportState state)
    {
        audioFileTransportController.setAudioTransportState (state);
    }

    void setPlayPressedCallback (std::function<void()> f)    { audioFileTransportController.setPlayPressedCallback (f); }
    void setPausePressedCallback (std::function<void()> f)   { audioFileTransportController.setPausePressedCallback (f); }
    void setRestartPressedCallback (std::function<void()> f) { audioFileTransportController.setRestartPressedCallback (f); }
    void setStopPressedCallback (std::function<void()> f)    { audioFileTransportController.setStopPressedCallback (f); }
private:
    LiveScrollingAudioDisplay                   audioScrollingDisplay;
    AudioFileTransportController                audioFileTransportController;
    ScopedPointer<AudioDeviceSelectorComponent> audioDeviceSelector;
    FeatureListModel                            featureListModel;
    FeatureListView                             featureListView;
    OSCSettingsController                       oscSettingsController;
    AudioSourceSelectorController               audioSourceTypeSelectorController;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};



#endif  // MAINVIEW_H_INCLUDED
