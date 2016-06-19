/*
  ==============================================================================

    AudioFeaturesListComponent.h
    Created: 10 Feb 2016 9:45:19pm
    Author:  Sean

  ==============================================================================
*/

#ifndef AUDIOFEATURESLISTCOMPONENT_H_INCLUDED
#define AUDIOFEATURESLISTCOMPONENT_H_INCLUDED

class FeatureListModel
{
public:
    FeatureListModel() {}

    void addFeature (AudioFeatures::eAudioFeature f)
    {
        visualisedFeatures.add (f);
    }

    Array<AudioFeatures::eAudioFeature>& getFeaturesToVisualise()
    {
        return visualisedFeatures;
    }

    static bool isTriggerFeature (AudioFeatures::eAudioFeature f)
    {
        if (f == AudioFeatures::eAudioFeature::enOnset)
            return true;

        return false;
    }

private:
    Array<AudioFeatures::eAudioFeature> visualisedFeatures;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeatureListModel)
};


//====================================================================================
//====================================================================================
/* visualiser class that calls a paint routine from look and feel */
class FeatureVisualiser : public Component
{
public:
    FeatureVisualiser (AudioFeatures::eAudioFeature f) 
    :   featureType (f),
        maxValue    (AudioFeatures::getMaxValueForFeature (f))
    {}

    void paint (Graphics& g) override 
    {
        auto localBounds = getLocalBounds();
        const auto textBounds = localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getFeatureVisualiserTextHeight()); 
        localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getFeatureVisualiserTextGraphicMargin());
        const auto visualiserBounds = localBounds.withSizeKeepingCentre (FeatureExtractorLookAndFeel::getFeatureVisualiserGraphicWidth(),
                                                                            localBounds.getHeight());
        FeatureExtractorLookAndFeel::paintFeatureVisualiser (g, value, visualiserBounds);
        g.setColour (Colours::white);
        g.drawText  (AudioFeatures::getFeatureName (featureType), textBounds, Justification::centred);
    }

    void setValue (float v) noexcept 
    {
        value = v; 

        MessageManager::getInstance()->callAsync ([this](){ repaint(); });
    }

    AudioFeatures::eAudioFeature featureType;
    float value    { 0.0f };
    float maxValue { 1.0f };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeatureVisualiser)
};
    
//================================================================================
//================================================================================
class TriggerFeatureVisualiser :    private Timer,
                                    public  Component
{
public:
    TriggerFeatureVisualiser (AudioFeatures::eAudioFeature f) 
    :   featureType (f) 
    {}

    void timerCallback() override
    {
        opacity -= 0.1f;
        if (opacity <= 0.0f)
        {    
            stopTimer();
            opacity = 0.0f;
        }
        MessageManager::getInstance()->callAsync ([this](){ repaint(); });
    }

    void paint (Graphics& g)
    {
        auto localBounds = getLocalBounds();
        const auto textBounds = localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getFeatureVisualiserTextHeight()); 
        localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getFeatureVisualiserTextGraphicMargin());
        const auto visualiserBounds = localBounds.withSizeKeepingCentre (FeatureExtractorLookAndFeel::getTriggerFeatureVisualiserSquareSize(),
                                                                         FeatureExtractorLookAndFeel::getTriggerFeatureVisualiserSquareSize());
        
        FeatureExtractorLookAndFeel::paintTriggerFeatureVisualiser (g, visualiserBounds, opacity);
        
        g.setColour (Colours::white);
        g.drawText  (AudioFeatures::getFeatureName (featureType), textBounds, Justification::centred);
    }

    void featureTriggered() noexcept 
    {
        opacity = 1.0f;
        startTimerHz (FeatureExtractorLookAndFeel::getAnimationRateHz());
    }

    AudioFeatures::eAudioFeature featureType;
private:
    float opacity  { 0.0f };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggerFeatureVisualiser)
};  
//================================================================================
//================================================================================
 class TriggerFeatureView : public Component,
                            public Slider::Listener,
                            public ComboBoxListener
{
public:
    TriggerFeatureView (AudioFeatures::eAudioFeature f) 
    :   visualiser        (f),
        sensitivityLabel  ("Sensitivity", "Sensitivity"),
        windowLengthLabel ("Window Length", "Window Length"),
        typeLabel         ("Type", "Detection Type")
    {
        sensitivityLabel.setJustificationType  (Justification::centred);
        windowLengthLabel.setJustificationType (Justification::centred);
        typeLabel.setJustificationType         (Justification::centred);
        sensitivitySlider.setRange (0.0, 3.0, 0.01);
        sensitivitySlider.addListener (this);
        sensitivitySlider.setDoubleClickReturnValue (true, 0.5);
        sensitivitySlider.setValue (0.5);
        sensitivitySlider.setSliderStyle (Slider::SliderStyle::LinearBar);

        fillComboBoxIDs();
        typeComboBox.setSelectedId (getIDForDetectionType (OnsetDetector::enAmplitude));
        typeComboBox.addListener (this);

        windowLengthSlider.setRange (3.0, 21.0, 2.0);
        windowLengthSlider.addListener (this);
        windowLengthSlider.setDoubleClickReturnValue (true, 0.5);
        windowLengthSlider.setValue (3.0);
        windowLengthSlider.setSliderStyle (Slider::SliderStyle::LinearBar);

        addAndMakeVisible (visualiser);
        addAndMakeVisible (sensitivitySlider);
        addAndMakeVisible (windowLengthSlider);
        addAndMakeVisible (typeComboBox);
        addAndMakeVisible (sensitivityLabel);
        addAndMakeVisible (windowLengthLabel);
        addAndMakeVisible (typeLabel);
    }

    void resized() override
    {
        auto localBounds = getLocalBounds();
        sensitivitySlider.setBounds (localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getSliderHeight())
                                                .reduced (FeatureExtractorLookAndFeel::getHorizontalMargin(), 0));
        sensitivityLabel.setBounds  (localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getSliderHeight())
                                                .reduced (FeatureExtractorLookAndFeel::getHorizontalMargin() / 2, 0));
        windowLengthSlider.setBounds (localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getSliderHeight())
                                                 .reduced (FeatureExtractorLookAndFeel::getHorizontalMargin(), 0));
        windowLengthLabel.setBounds  (localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getSliderHeight())
                                                 .reduced (FeatureExtractorLookAndFeel::getHorizontalMargin() / 2, 0));
        typeComboBox.setBounds       (localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getSliderHeight())
                                                 .reduced (FeatureExtractorLookAndFeel::getHorizontalMargin(), 0));
        typeLabel.setBounds          (localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getSliderHeight())
                                                 .reduced (FeatureExtractorLookAndFeel::getHorizontalMargin() / 2, 0));
        localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getVerticalMargin() / 2);
        visualiser.setBounds         (localBounds);
    }

    void sliderValueChanged (Slider* slider) override
    {
        if (slider == &sensitivitySlider)
            if (sensitivityChanged != nullptr)
                sensitivityChanged ((float) slider->getValue());
        if (slider == &windowLengthSlider)
            if (windowLengthChanged != nullptr)
                windowLengthChanged ((int) slider->getValue());
    }

    void comboBoxChanged (ComboBox* box) override
    {
        if (box == &typeComboBox)
            if (typeChanged != nullptr)
                typeChanged (getDetectionTypeForID (box->getSelectedId()));
    }

    void fillComboBoxIDs()
    {
        for (int type = 0; type < (int) OnsetDetector::enNumTypes; type++)
        {
            OnsetDetector::eOnsetDetectionType t = (OnsetDetector::eOnsetDetectionType) type;
            typeComboBox.addItem (OnsetDetector::getStringForDetectionType (t), getIDForDetectionType (t));
        }
    }

    static int getIDForDetectionType (OnsetDetector::eOnsetDetectionType t) { return (int) t + 1; }
    static OnsetDetector::eOnsetDetectionType getDetectionTypeForID (int t) { return (OnsetDetector::eOnsetDetectionType) (t - 1); }
    
    void featureTriggered() { visualiser.featureTriggered(); }

    AudioFeatures::eAudioFeature getFeatureType() { return visualiser.featureType; }

    void setSensitivityChangedCallback (std::function<void (float)> f)                              { sensitivityChanged  = f; }
    void setWindowLengthCallback       (std::function<void (int)> f)                                { windowLengthChanged = f; }
    void setTypeCallback               (std::function<void (OnsetDetector::eOnsetDetectionType)> f) { typeChanged = f; }
private:
    std::function<void (float)>                              sensitivityChanged;
    std::function<void (int)>                                windowLengthChanged;
    std::function<void (OnsetDetector::eOnsetDetectionType)> typeChanged;
    TriggerFeatureVisualiser visualiser;
    ComboBox                 typeComboBox;
    Slider                   sensitivitySlider;
    Slider                   windowLengthSlider;
    Label                    sensitivityLabel;
    Label                    windowLengthLabel;
    Label                    typeLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggerFeatureView)
};
//================================================================================
//================================================================================
class FeatureListView : public  Timer,
                        public  Component
{
public:
    FeatureListView (FeatureListModel& flm)
    :   model (flm)
    {
        recreateVisualisersFromModel ();
        startTimerHz (FeatureExtractorLookAndFeel::getAnimationRateHz());
    }

    void recreateVisualisersFromModel()
    {
        featureVisualisers.clear (true);

        for (auto feature : model.getFeaturesToVisualise())
        {
            if (FeatureListModel::isTriggerFeature (feature))
                addAndMakeVisible (triggerFeatureViews.add (new TriggerFeatureView (feature)));
            else
                addAndMakeVisible (featureVisualisers.add (new FeatureVisualiser (feature)));
        }
    }

    void timerCallback() override
    {
        if (getLatestFeatureValue)
        {
            for (auto visualiser : featureVisualisers)
                visualiser->setValue (getLatestFeatureValue (visualiser->featureType, visualiser->maxValue));
        }

    }

    void resized() override
    {
        auto localBounds = getLocalBounds();
        const int availableWidth  = localBounds.getWidth();
        const int visualiserWidth = availableWidth / (featureVisualisers.size() + triggerFeatureViews.size());
        
        if (triggerFeatureViews.size() > 0)
            for (auto triggerFeatureView : triggerFeatureViews)
                triggerFeatureView->setBounds (localBounds.removeFromLeft (visualiserWidth));

        if (featureVisualisers.size() > 0)
            for (auto featureVisualiser : featureVisualisers)
                featureVisualiser->setBounds (localBounds.removeFromLeft (visualiserWidth));
    }
    
    void featureTriggered (AudioFeatures::eAudioFeature triggerType)
    {
        for (auto triggerFeatureView : triggerFeatureViews)
            if (triggerFeatureView->getFeatureType() == triggerType)
                triggerFeatureView->featureTriggered();
    }

    void setFeatureValueQueryCallback (std::function <float (AudioFeatures::eAudioFeature, float)> f) 
    { 
        getLatestFeatureValue = f; 
    }
    
    void setOnsetSensitivityCallback (std::function<void (float)> f)
    {
        for (auto triggerFeatureView : triggerFeatureViews)
            if (triggerFeatureView->getFeatureType() == OSCFeatureAnalysisOutput::OSCFeatureType::Onset)
                triggerFeatureView->setSensitivityChangedCallback (f);
    }

    void setOnsetWindowLengthCallback (std::function<void (int)> f)
    {
        for (auto triggerFeatureView : triggerFeatureViews)
            if (triggerFeatureView->getFeatureType() == OSCFeatureAnalysisOutput::OSCFeatureType::Onset)
                triggerFeatureView->setWindowLengthCallback (f);
    }

    void setOnsetDetectionTypeCallback (std::function<void (OnsetDetector::eOnsetDetectionType)> f)
    {
        for (auto triggerFeatureView : triggerFeatureViews)
            if (triggerFeatureView->getFeatureType() == OSCFeatureAnalysisOutput::OSCFeatureType::Onset)
                triggerFeatureView->setTypeCallback (f);
    }

private:
    OwnedArray<FeatureVisualiser>                                       featureVisualisers;
    OwnedArray<TriggerFeatureView>                                      triggerFeatureViews;
    std::function<float (AudioFeatures::eAudioFeature, float maxValue)> getLatestFeatureValue;
    FeatureListModel&                                                   model;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeatureListView)
};



#endif  // AUDIOFEATURESLISTCOMPONENT_H_INCLUDED
