/*
  ==============================================================================

    AnalyserTrack.h
    Created: 16 Jun 2016 4:25:58pm
    Author:  Sean

  ==============================================================================
*/

#ifndef ANALYSERTRACK_H_INCLUDED
#define ANALYSERTRACK_H_INCLUDED

class AnalyserTrack : public Component,
                      public Slider::Listener
{
public:
    AnalyserTrack (String channelName)
    :   channelNameLabel                  ("AnalyserTrack", channelName),
        gainLabel                         ("gainLabel", "Gain:"),
        audioScrollingDisplay             (1),
        featureListView                   (featureListModel),
        audioSourceTypeSelectorController (getAudioSourceTypeString)
    {
        setLookAndFeel (SharedResourcePointer<FeatureExtractorLookAndFeel>());  

        audioScrollingDisplay.clear();
        audioScrollingDisplay.setSamplesPerBlock (256);
        audioScrollingDisplay.setBufferSize (1024);
        addAndMakeVisible (audioScrollingDisplay);

        for (int i = (int) AudioFeatures::eAudioFeature::enRMS; i < (int) AudioFeatures::eAudioFeature::numFeatures; i++)
                featureListModel.addFeature ((AudioFeatures::eAudioFeature) i);

        featureListView.recreateVisualisersFromModel();
        
        addAndMakeVisible (featureListView);

        addAndMakeVisible (oscSettingsController.getView());

        addAndMakeVisible (audioFileTransportController.getView());
        audioFileTransportController.getView().setVisible (false);

        addAndMakeVisible (audioSourceTypeSelectorController.getSelector());

        gainLabel.setJustificationType  (Justification::centredRight);
        gainSlider.setRange (0.0, 100.0, 0.01);
        gainSlider.addListener (this);
        gainSlider.setDoubleClickReturnValue (true, 1.0);
        gainSlider.setValue (1.0);
        gainSlider.setSliderStyle (Slider::SliderStyle::LinearBar);

        addAndMakeVisible (gainSlider);
        addAndMakeVisible (gainLabel);

        addAndMakeVisible (channelNameLabel);

    }

    ~AnalyserTrack()
    {
        stopAnimation();
        setAudioSourceTypeChangedCallback (nullptr);
        setAddressChangedCallback         (nullptr);
        setBundleAddressChangedCallback   (nullptr);                                         
        setFileDroppedCallback            (nullptr);
        setPlayPressedCallback            (nullptr);
        setPausePressedCallback           (nullptr);
        setRestartPressedCallback         (nullptr);
        setStopPressedCallback            (nullptr);
        setOnsetSensitivityCallback       (nullptr);
        setOnsetWindowSizeCallback        (nullptr);
        setOnsetDetectionTypeCallback     (nullptr);
        setFeatureValueQueryCallback      (nullptr);
        setGainChangedCallback            (nullptr);
    }

    void setChannelName (String n) { channelNameLabel.setText (n, dontSendNotification); }

    void resized() override
    {
        auto localBounds                = getLocalBounds();
        auto trackBounds                = localBounds.removeFromTop (150);//<- magic number to go in LnF
        const auto& availableWidth       = trackBounds.getWidth();
        const auto& audioDisplayWidth    = (int)(availableWidth * 0.75f);
        const auto& oscWidth             = (int)(availableWidth * 0.25f);
        auto audioAndFeaturesBounds     = trackBounds.removeFromLeft (audioDisplayWidth);
        const int displayHeight          = audioAndFeaturesBounds.getHeight() / 2; 
        
        audioScrollingDisplay.setBounds           (audioAndFeaturesBounds.removeFromTop (displayHeight));
        featureListView.setBounds                 (audioAndFeaturesBounds.removeFromTop (displayHeight));
        oscSettingsController.getView().setBounds (trackBounds.removeFromLeft (audioDisplayWidth));
        
        const auto w = localBounds.getWidth() / 2;
        
        auto labelBounds = getLocalBounds().removeFromTop (FeatureExtractorLookAndFeel::getDeviceSettingsItemHeight()
                                                          + FeatureExtractorLookAndFeel::getInnerComponentSpacing());

        auto sliderLabelBounds = labelBounds.removeFromBottom (FeatureExtractorLookAndFeel::getDeviceSettingsItemHeight());
        channelNameLabel.setBounds (sliderLabelBounds.removeFromLeft (100));
        sliderLabelBounds.removeFromLeft (10);
        gainLabel.setBounds (sliderLabelBounds.removeFromLeft (100));
        gainSlider.setBounds (sliderLabelBounds.removeFromLeft (100));
    }

    void sliderValueChanged (Slider* s) override
    {
        if (s == &gainSlider)
            if (gainChangedCallback != nullptr)
                gainChangedCallback ((float) s->getValue());
    }

    void stopAnimation() 
    { 
        featureListView.setFeatureValueQueryCallback (nullptr); 
        featureListView.stopTimer(); 
    }

    AudioVisualiserComponent&  getAudioDisplayComponent()     { return audioScrollingDisplay; }
    OSCSettingsController& getOSCSettingsController() { return oscSettingsController; }

    void setAudioSourceTypeChangedCallback (std::function<void (eAudioSourceType type)> f) { audioSourceTypeSelectorController.setAudioSourceTypeChangedCallback (f); }
    void setAddressChangedCallback (std::function<bool (String)> f)                        { oscSettingsController.setAddressChangedCallback (f); }
    void setDisplayedOSCAddress (String address)                                           { oscSettingsController.getView().getAddressEditor().setText (address); }
    void setSecondaryAddressChangedCallback (std::function<bool (String)> f)               { oscSettingsController.setSecondaryAddressChangedCallback (f); }
    void setSecondaryDisplayedOSCAddress (String address)                                  { oscSettingsController.getView().getSecondaryAddressEditor().setText (address); }
    void setBundleAddressChangedCallback (std::function<void (String)> f)                  { oscSettingsController.setBundleAddressChangedCallback (f); }
    void setDisplayedBundleAddress (String address)                                        { oscSettingsController.getView().getBundleAddressEditor().setText (address); }

    void setFileDroppedCallback (std::function<void (File&)> f)                            { audioFileTransportController.setFileDroppedCallback (f); }
    void toggleShowTransportControls (bool shouldShowControls)                             { audioFileTransportController.getView().setVisible (shouldShowControls); }
    void setAudioTransportState (AudioFileTransportController::eAudioTransportState state) { audioFileTransportController.setAudioTransportState (state); }
    void setPlayPressedCallback (std::function<void()> f)                                  { audioFileTransportController.setPlayPressedCallback (f); }
    void setPausePressedCallback (std::function<void()> f)                                 { audioFileTransportController.setPausePressedCallback (f); }
    void setRestartPressedCallback (std::function<void()> f)                               { audioFileTransportController.setRestartPressedCallback (f); }
    void setStopPressedCallback (std::function<void()> f)                                  { audioFileTransportController.setStopPressedCallback (f); }

    void clearAudioDisplayData()                                                           { audioScrollingDisplay.clear(); }

    void featureTriggered (AudioFeatures::eAudioFeature triggerType)                                          
    { 
        MessageManager::getInstance()->callAsync ([this, triggerType]()
        {
            featureListView.featureTriggered (triggerType);
        });   
    }

    void updateBufferToPush (AudioSampleBuffer* buffer)
    {
        if (buffer != nullptr)
        {
            bufferToPush = AudioSampleBuffer (*buffer);
            MessageManager::getInstance()->callAsync ([this]()
            {
                audioScrollingDisplay.pushBuffer (bufferToPush);
            });
        }
    } 

    void startAnimation() { featureListView.startAnimation(); }

    void setOnsetSensitivityCallback   (std::function<void (float)> f)                                         { featureListView.setOnsetSensitivityCallback (f); }
    void setOnsetWindowSizeCallback    (std::function<void (int)> f)                                           { featureListView.setOnsetWindowLengthCallback (f); }
    void setOnsetDetectionTypeCallback (std::function<void (OnsetDetector::eOnsetDetectionType)> f)            { featureListView.setOnsetDetectionTypeCallback (f); }
    void setFeatureValueQueryCallback  (std::function<float (AudioFeatures::eAudioFeature, float maxValue)> f) { featureListView.setFeatureValueQueryCallback (f); }
    void setGainChangedCallback        (std::function<void (float)> f)                                         { gainChangedCallback = f; }

private:
    std::function<void (float)>     gainChangedCallback;
    Label                           channelNameLabel;
    Label                           gainLabel;
    Slider                          gainSlider;
    AudioSampleBuffer               bufferToPush;
    AudioVisualiserComponent        audioScrollingDisplay;
    AudioFileTransportController    audioFileTransportController;
    FeatureListModel                featureListModel;
    FeatureListView                 featureListView;
    OSCSettingsController           oscSettingsController;
    AudioSourceSelectorController<> audioSourceTypeSelectorController;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyserTrack);
};



#endif  // ANALYSERTRACK_H_INCLUDED
