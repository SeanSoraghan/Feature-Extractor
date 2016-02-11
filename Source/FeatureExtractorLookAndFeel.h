/*
  ==============================================================================

    FeatureExtractorLookAndFeel.h
    Created: 10 Feb 2016 8:00:52pm
    Author:  Sean

  ==============================================================================
*/

#ifndef FEATUREEXTRACTORLOOKANDFEEL_H_INCLUDED
#define FEATUREEXTRACTORLOOKANDFEEL_H_INCLUDED

//==============================================================================
/* Look and feel and layout functions
*/
class FeatureExtractorLookAndFeel : private LookAndFeel_V3
{
public:
    static float getAudioDisplayHeightRatio()               noexcept { return 0.1f; }
    static float getFeatureVisualiserComponentHeightRatio() noexcept { return 0.4f; }
    static float getSettingsHeightRatio()                   noexcept { return 0.5f; }
    static int   getFeatureVisualiserTextHeight()           noexcept { return 10; }
    static int   getFeatureVisualiserGraphicWidth()         noexcept { return 2; }
    static int   getFeatureVisualiserTextGraphicMargin()    noexcept { return 15; }
    static int   getVerticalMargin()                        noexcept { return 10; }
    static int   getHorizontalMargin()                      noexcept { return 10; }
    static int   getAnimationRateHz ()                      noexcept { return 60; }

    static void paintFeatureVisualiser (Graphics& g, float value, Rectangle<int> visualiserBounds)
    {
        g.setColour  (Colours::white);
        g.fillRect   (visualiserBounds);
        int height = (int) (visualiserBounds.getHeight() * value);
        g.setColour  (Colours::green);
        g.fillRect   (visualiserBounds.removeFromBottom (height));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeatureExtractorLookAndFeel)
};



#endif  // FEATUREEXTRACTORLOOKANDFEEL_H_INCLUDED
