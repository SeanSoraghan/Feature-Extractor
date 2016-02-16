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

    void addFeature (OSCFeatureAnalysisOutput::OSCFeatureType f)
    {
        visualisedFeatures.add (f);
    }

    Array<OSCFeatureAnalysisOutput::OSCFeatureType>& getFeaturesToVisualise()
    {
        return visualisedFeatures;
    }

    static bool isTriggerFeature (OSCFeatureAnalysisOutput::OSCFeatureType f)
    {
        if (f == OSCFeatureAnalysisOutput::OSCFeatureType::Onset)
            return true;

        return false;
    }

private:
    Array<OSCFeatureAnalysisOutput::OSCFeatureType> visualisedFeatures;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeatureListModel)
};


//====================================================================================
//====================================================================================
/* visualiser class that calls a paint routine from look and feel */
class FeatureVisualiser : public Component
{
public:
    FeatureVisualiser (OSCFeatureAnalysisOutput::OSCFeatureType f) : featureType (f) {}

    void paint (Graphics& g) override 
    {
        auto localBounds = getLocalBounds();
        const auto textBounds = localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getFeatureVisualiserTextHeight()); 
        localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getFeatureVisualiserTextGraphicMargin());
        const auto visualiserBounds = localBounds.withSizeKeepingCentre (FeatureExtractorLookAndFeel::getFeatureVisualiserGraphicWidth(),
                                                                            localBounds.getHeight());
        FeatureExtractorLookAndFeel::paintFeatureVisualiser (g, value, visualiserBounds);
        g.setColour (Colours::white);
        g.drawText  (OSCFeatureAnalysisOutput::getOSCFeatureName (featureType), textBounds, Justification::centred);
    }

    void setValue (float v) noexcept 
    {
        value = v; 

        MessageManager::getInstance()->callAsync ([this](){ repaint(); });
    }

    OSCFeatureAnalysisOutput::OSCFeatureType featureType;
    float value { 0.0f };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeatureVisualiser)
};
    
//================================================================================
//================================================================================
class TriggerFeatureVisualiser :    private Timer,
                                    public  Component
{
public:
    TriggerFeatureVisualiser (OSCFeatureAnalysisOutput::OSCFeatureType f) : featureType (f) {}

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
        g.drawText  (OSCFeatureAnalysisOutput::getOSCFeatureName (featureType), textBounds, Justification::centred);
    }

    void featureTriggered() noexcept 
    {
        opacity = 1.0f;
        startTimerHz (FeatureExtractorLookAndFeel::getAnimationRateHz());
    }

    OSCFeatureAnalysisOutput::OSCFeatureType featureType;
private:
    float opacity { 0.0f };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggerFeatureVisualiser)
};  
//================================================================================
//================================================================================
 class TriggerFeatureView : public Component,
                            public Slider::Listener
{
public:
    TriggerFeatureView (OSCFeatureAnalysisOutput::OSCFeatureType f) 
    :   visualiser (f) ,
        sensitivityLabel ("Sensitivity", "Sensitivity")
    {
        sensitivitySlider.setRange (0.0, 1.0, 0.01);
        sensitivitySlider.setName  ("Sensitivity");
        sensitivitySlider.addListener (this);
        addAndMakeVisible (visualiser);
        addAndMakeVisible (sensitivitySlider);
        sensitivitySlider.setDoubleClickReturnValue (true, 0.5);
        sensitivitySlider.setValue (0.5);
        sensitivitySlider.setSliderStyle (Slider::SliderStyle::LinearBar);
        //sensitivitySlider.setTextBoxStyle (Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
        addAndMakeVisible (sensitivityLabel);
    }

    void resized() override
    {
        auto localBounds = getLocalBounds();
        const auto sliderBounds = localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getSliderHeight());
        sensitivitySlider.setBounds (sliderBounds.reduced (FeatureExtractorLookAndFeel::getHorizontalMargin(), 0));
        sensitivityLabel.setBounds  (localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getSliderHeight())
                                                .reduced (FeatureExtractorLookAndFeel::getHorizontalMargin(), 0));
        visualiser.setBounds        (localBounds);
    }

    virtual void sliderValueChanged (Slider* slider) override
    {
        if (slider == &sensitivitySlider)
            if (sensitivityChanged != nullptr)
                sensitivityChanged ((float) slider->getValue());
    }

    void featureTriggered() { visualiser.featureTriggered(); }

    OSCFeatureAnalysisOutput::OSCFeatureType getFeatureType() { return visualiser.featureType; }

    void setSensitivityChangedCallback (std::function<void (float)> f) { sensitivityChanged = f; }

private:
    std::function<void (float)> sensitivityChanged;
    TriggerFeatureVisualiser visualiser;
    Slider                   sensitivitySlider;
    Label                    sensitivityLabel;

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
                visualiser->setValue (getLatestFeatureValue (visualiser->featureType));
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
    
    void featureTriggered (OSCFeatureAnalysisOutput::OSCFeatureType triggerType)
    {
        for (auto triggerFeatureView : triggerFeatureViews)
            if (triggerFeatureView->getFeatureType() == triggerType)
                triggerFeatureView->featureTriggered();
    }

    void setFeatureValueQueryCallback (std::function <float (OSCFeatureAnalysisOutput::OSCFeatureType)> f) { getLatestFeatureValue = f; }
    
    void setOnsetSensitivityCallback (std::function<void (float)> f)
    {
        for (auto triggerFeatureView : triggerFeatureViews)
            if (triggerFeatureView->getFeatureType() == OSCFeatureAnalysisOutput::OSCFeatureType::Onset)
                triggerFeatureView->setSensitivityChangedCallback (f);
    }
private:
    OwnedArray<FeatureVisualiser>                                   featureVisualisers;
    OwnedArray<TriggerFeatureView>                                  triggerFeatureViews;
    std::function<float (OSCFeatureAnalysisOutput::OSCFeatureType)> getLatestFeatureValue;
    FeatureListModel& model;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeatureListView)
};



#endif  // AUDIOFEATURESLISTCOMPONENT_H_INCLUDED
