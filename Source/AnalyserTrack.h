/*
  ==============================================================================

    AnalyserTrack.h
    Created: 16 Jun 2016 4:25:58pm
    Author:  Sean

  ==============================================================================
*/

#ifndef ANALYSERTRACK_H_INCLUDED
#define ANALYSERTRACK_H_INCLUDED

class AnalyserTrack : public Component
{
public:

private:
    std::function<void (float)>                 gainChangedCallback;
    Label                                       gainLabel;
    Slider                                      gainSlider;
    AudioVisualiserComponent                    audioScrollingDisplay;
    AudioFileTransportController                audioFileTransportController;
    ScopedPointer<ChannelSelectorPanel>         channelSelector;
    FeatureListModel                            featureListModel;
    FeatureListView                             featureListView;
    OSCSettingsController                       oscSettingsController;
    PitchEstimationVisualiser                   pitchEstimationVisualiser;
    AudioSourceSelectorController<>             audioSourceTypeSelectorController;
    AudioSourceSelectorController<eChannelType> channelTypeSelector;
};



#endif  // ANALYSERTRACK_H_INCLUDED
