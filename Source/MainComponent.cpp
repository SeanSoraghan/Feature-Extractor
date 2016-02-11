/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "include.h"

//==============================================================================
/*
    Application for linking up audio device manager with different GUI elements
*/
class MainContentComponent   : public AudioAppComponent
{
public:
    //==============================================================================
    MainContentComponent() 
    :   audioAnalyser    (audioDataCollector), 
        oscFeatureSender (audioAnalyser)
    {
        setSize (800, 600);
        // specify the number of input and output channels that we want to open
        setAudioChannels  (2, 2);
        addAndMakeVisible (view);
        deviceManager.addAudioCallback (&view.getAudioDisplayComponent());
        deviceManager.addAudioCallback (&audioDataCollector);

        view.setFeatureValueQueryCallback ([this] (ConcatenatedFeatureBuffer::Feature f) { return oscFeatureSender.getRunningAverage (f); });
        //switches between listening to input or output
        view.setAudioDataStreamToggleCallback ([this] (bool input) { audioDataCollector.toggleCollectInput (input); });
        view.setAddressChangedCallback ([this] (String address) { return oscFeatureSender.connectToAddress (address); });
        view.setAudioSettingsDeviceManager (deviceManager);
        view.setDisplayedOSCAddress (oscFeatureSender.address);
    }

    ~MainContentComponent()
    {
        shutdownAudio();
    }

    //=======================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        view.getAudioDisplayComponent().setSamplesPerBlock (samplesPerBlockExpected);
        audioAnalyser.sampleRateChanged (sampleRate);
        audioDataCollector.setExpectedSamplesPerBlock (samplesPerBlockExpected);
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        // Your audio-processing code goes here!

        // For more details, see the help for AudioProcessor::getNextAudioBlock()

        // Right now we are not producing any data, in which case we need to clear the buffer
        // (to prevent the output of random noise)
        bufferToFill.clearActiveBufferRegion();
    }

    void releaseResources() override
    {
        // This will be called when the audio device stops, or when it is being
        // restarted due to a setting change.

        // For more details, see the help for AudioProcessor::releaseResources()
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
    }

    void resized() override
    {
        view.setBounds (getLocalBounds());
    }

private:
    SharedResourcePointer<FeatureExtractorLookAndFeel> lookAndFeel;
    MainView                 view;
    AudioDataCollector       audioDataCollector;
    RealTimeAnalyser         audioAnalyser;
    OSCFeatureAnalysisOutput oscFeatureSender;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
