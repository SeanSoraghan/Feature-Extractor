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

    void addFeature (ConcatenatedFeatureBuffer::Feature f)
    {
        visualisedFeatures.add (f);
    }

    Array<ConcatenatedFeatureBuffer::Feature>& getFeaturesToVisualise()
    {
        return visualisedFeatures;
    }

private:
    Array<ConcatenatedFeatureBuffer::Feature> visualisedFeatures;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeatureListModel)
};


//====================================================================================
//====================================================================================

class FeatureListView : public  Timer,
                        public  Component
{
public:
    /* visualiser class that calls a paint routine from look and feel */
    class FeatureVisualiser : public Component
    {
    public:
        FeatureVisualiser (ConcatenatedFeatureBuffer::Feature f) : featureType (f) {}
        
        void paint (Graphics& g) override 
        {
            auto localBounds = getLocalBounds();
            const auto textBounds = localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getFeatureVisualiserTextHeight()); 
            localBounds.removeFromBottom (FeatureExtractorLookAndFeel::getFeatureVisualiserTextGraphicMargin());
            const auto visualiserBounds = localBounds.withSizeKeepingCentre (FeatureExtractorLookAndFeel::getFeatureVisualiserGraphicWidth(),
                                                                             localBounds.getHeight());

            FeatureExtractorLookAndFeel::paintFeatureVisualiser (g, value, visualiserBounds);
            g.setColour (Colours::white);
            g.drawText  (ConcatenatedFeatureBuffer::getFeatureName (featureType), textBounds, Justification::centred);
        }

        void setValue (float v) noexcept 
        {
            value = v; 
            MessageManager::getInstance()->callAsync ([this](){ repaint(); });
        }

        ConcatenatedFeatureBuffer::Feature featureType;
        float value;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeatureVisualiser)
    };
    
    //================================================================================
    //================================================================================

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
            addAndMakeVisible (featureVisualisers.add (new FeatureVisualiser (feature)));
    }

    void timerCallback() override
    {
        if (getLatestFeatureValue)
            for (auto visualiser : featureVisualisers)
                visualiser->setValue (getLatestFeatureValue (visualiser->featureType));
    }

    void paint (Graphics& g) override
    {}

    void resized() override
    {
        if (featureVisualisers.size() > 0)
        {
            auto localBounds = getLocalBounds();
            const int availableWidth  = localBounds.getWidth();
            const int visualiserWidth = availableWidth / featureVisualisers.size();
            for (auto featureVisualiser : featureVisualisers)
            {
                featureVisualiser->setBounds (localBounds.removeFromLeft (visualiserWidth));
            }
        }
    }
    
    void setFeatureValueQueryCallback (std::function <float (ConcatenatedFeatureBuffer::Feature)> f) { getLatestFeatureValue = f; }

private:
    OwnedArray<FeatureVisualiser> featureVisualisers;
    std::function<float (ConcatenatedFeatureBuffer::Feature)> getLatestFeatureValue;
    FeatureListModel& model;
};



#endif  // AUDIOFEATURESLISTCOMPONENT_H_INCLUDED
