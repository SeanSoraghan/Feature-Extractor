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
class MainView : public Component, public Slider::Listener
{
public:
    MainView ()
    :   gainLabel                         ("gainLabel", "Gain:"),
        audioScrollingDisplay             (1),
        featureListView                   (featureListModel),
        audioSourceTypeSelectorController (getAudioSourceTypeString),
        channelTypeSelector               (getChannelTypeString)
    {
        setLookAndFeel (SharedResourcePointer<FeatureExtractorLookAndFeel>());
        
        audioScrollingDisplay.clear();
        audioScrollingDisplay.setSamplesPerBlock (256);
        audioScrollingDisplay.setBufferSize (1024);

        addAndMakeVisible (audioDeviceSelector);
        addAndMakeVisible (audioScrollingDisplay);

        for (int i = 0; i < (int) AudioFeatures::eAudioFeature::numFeatures; i++)
            if (i <= AudioFeatures::eAudioFeature::enCentroid)
                featureListModel.addFeature ((AudioFeatures::eAudioFeature) i);
        
        featureListModel.addFeature (AudioFeatures::enFlatness);

        featureListView.recreateVisualisersFromModel();
        
        addAndMakeVisible (featureListView);

        addAndMakeVisible (oscSettingsController.getView());

        addAndMakeVisible (audioFileTransportController.getView());
        audioFileTransportController.getView().setVisible (false);

        addAndMakeVisible (audioSourceTypeSelectorController.getSelector());
        addAndMakeVisible (channelTypeSelector.getSelector());

        gainLabel.setJustificationType  (Justification::centredLeft);
        gainSlider.setRange (0.0, 100.0, 0.01);
        gainSlider.addListener (this);
        gainSlider.setDoubleClickReturnValue (true, 1.0);
        gainSlider.setValue (1.0);
        gainSlider.setSliderStyle (Slider::SliderStyle::LinearBar);

        addAndMakeVisible (gainSlider);
        addAndMakeVisible (gainLabel);

        addAndMakeVisible (pitchEstimationVisualiser);
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
        auto gainBounds = localBounds.removeFromTop      (verticalMargin).reduced ((int) (getWidth() * 0.35f), (int) (verticalMargin * 0.2f));
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
        auto selectorBounds = displayBounds.removeFromTop  (FeatureExtractorLookAndFeel::getAudioSourceTypeSelectorHeight());
        auto audioSourceSelectorBounds = selectorBounds.removeFromLeft (FeatureExtractorLookAndFeel::getAudioSourceTypeSelectorWidth());
        auto channelSelectorBounds = selectorBounds.removeFromRight (FeatureExtractorLookAndFeel::getAudioSourceTypeSelectorWidth());
        audioSourceTypeSelectorController.getSelector().setBounds (audioSourceSelectorBounds);
        channelTypeSelector.getSelector().setBounds (channelSelectorBounds);

        gainLabel.setBounds  (gainBounds.removeFromLeft (gainBounds.getWidth() / 2));
        gainSlider.setBounds (gainBounds);

        //pitchEstimationVisualiser.setBounds (getLocalBounds().withSizeKeepingCentre (400, 400));
    }

    void sliderValueChanged (Slider* s) override
    {
        if (s == &gainSlider)
            if (gainChangedCallback != nullptr)
                gainChangedCallback ((float) s->getValue());
    }

    AudioVisualiserComponent&  getAudioDisplayComponent()     { return audioScrollingDisplay; }
    PitchEstimationVisualiser& getPitchEstimationVisualiser() { return pitchEstimationVisualiser; }

    void setAudioSettingsDeviceManager (AudioDeviceManager& deviceManager)
    {
        addAndMakeVisible (audioDeviceSelector = new CustomAudioDeviceSelectorComponent (deviceManager, 2, 2, 2, 2, false, false, true, true));
        resized();
    }

    void setAudioSourceTypeChangedCallback (std::function<void (eAudioSourceType type)> f) { audioSourceTypeSelectorController.setAudioSourceTypeChangedCallback (f); }
    void setChannelTypeChangedCallback     (std::function<void (eChannelType)> f)          { channelTypeSelector.setAudioSourceTypeChangedCallback (f); }
    void setAddressChangedCallback (std::function<bool (String)> f)                                                       { oscSettingsController.setAddressChangedCallback (f); }
    void setDisplayedOSCAddress (String address)                                                                          { oscSettingsController.getView().getAddressEditor().setText (address); }
    void setBundleAddressChangedCallback (std::function<void (String)> f)                                                 { oscSettingsController.setBundleAddressChangedCallback (f); }
    void setDisplayedBundleAddress (String address)                                                                       { oscSettingsController.getView().getBundleAddressEditor().setText (address); }

    void setFileDroppedCallback (std::function<void (File&)> f)                                                           { audioFileTransportController.setFileDroppedCallback (f); }
    void toggleShowTransportControls (bool shouldShowControls)                                                            { audioFileTransportController.getView().setVisible (shouldShowControls); }
    void setAudioTransportState (AudioFileTransportController::eAudioTransportState state)                                { audioFileTransportController.setAudioTransportState (state); }
    void setPlayPressedCallback (std::function<void()> f)                                                                 { audioFileTransportController.setPlayPressedCallback (f); }
    void setPausePressedCallback (std::function<void()> f)                                                                { audioFileTransportController.setPausePressedCallback (f); }
    void setRestartPressedCallback (std::function<void()> f)                                                              { audioFileTransportController.setRestartPressedCallback (f); }
    void setStopPressedCallback (std::function<void()> f)                                                                 { audioFileTransportController.setStopPressedCallback (f); }

    void clearAudioDisplayData()                                                                                          { audioScrollingDisplay.clear(); }

    void featureTriggered (AudioFeatures::eAudioFeature triggerType)                                          
    { 
        MessageManager::getInstance()->callAsync ([this, triggerType]()
        {
            featureListView.featureTriggered (triggerType);
        });   
    }

    void setOnsetSensitivityCallback   (std::function<void (float)> f)                                                     { featureListView.setOnsetSensitivityCallback (f); }
    void setOnsetWindowSizeCallback    (std::function<void (int)> f)                                                       { featureListView.setOnsetWindowLengthCallback (f); }
    void setOnsetDetectionTypeCallback (std::function<void (OnsetDetector::eOnsetDetectionType)> f)                        { featureListView.setOnsetDetectionTypeCallback (f); }
    void setFeatureValueQueryCallback  (std::function<float (AudioFeatures::eAudioFeature, float maxValue)> f)             { featureListView.setFeatureValueQueryCallback (f); }
    void setGainChangedCallback        (std::function<void (float)> f)                                                     { gainChangedCallback = f; }

private:
    std::function<void (float)>                 gainChangedCallback;
    Label                                       gainLabel;
    Slider                                      gainSlider;
    AudioVisualiserComponent                    audioScrollingDisplay;
    AudioFileTransportController                audioFileTransportController;
    ScopedPointer<CustomAudioDeviceSelectorComponent> audioDeviceSelector;
    FeatureListModel                            featureListModel;
    FeatureListView                             featureListView;
    OSCSettingsController                       oscSettingsController;
    PitchEstimationVisualiser                   pitchEstimationVisualiser;
    AudioSourceSelectorController<>             audioSourceTypeSelectorController;
    AudioSourceSelectorController<eChannelType> channelTypeSelector;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};



#endif  // MAINVIEW_H_INCLUDED
