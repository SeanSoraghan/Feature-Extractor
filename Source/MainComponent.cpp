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
        
        //deviceManager.addAudioCallback (&view.getAudioDisplayComponent());
        audioDataCollector.setSpectralBufferUpdatedCallback ([this] (AudioSampleBuffer& b) 
                                                             {
                                                                 view.getAudioDisplayComponent().pushBuffer (b);
                                                             });

        oscFeatureSender.onsetDetectedCallback = ([this] () 
                                                 {
                                                     view.featureTriggered (OSCFeatureAnalysisOutput::OSCFeatureType::Onset);
                                                 });

        deviceManager.addAudioCallback (&audioDataCollector);
        

        audioFilePlayer.setupAudioCallback (deviceManager);

        view.setFeatureValueQueryCallback ([this] (OSCFeatureAnalysisOutput::OSCFeatureType oscf) 
                                                   { 
                                                       if ( ! OSCFeatureAnalysisOutput::isTriggerFeature (oscf))
                                                           return oscFeatureSender.getRunningAverage (oscf); 
                                                   });

        //switches between listening to input or output
        view.setAudioSourceTypeChangedCallback ([this] (eAudioSourceType type) 
                                                        { 
                                                            bool input = type == eAudioSourceType::enIncomingAudio;
                                                           
                                                            if (input)
                                                                audioFilePlayer.stop();
                                                            
                                                            audioDataCollector.toggleCollectInput (input);
                                                            view.toggleShowTransportControls (type == eAudioSourceType::enAudioFile);
                                                            view.clearAudioDisplayData();
                                                        }
                                                );

        view.setChannelTypeChangedCallback ([this] (eChannelType t) { audioDataCollector.setChannelToCollect ((int) t); });
        
        view.setFileDroppedCallback ([this] (File& f) 
                                    { 
                                        audioDataCollector.clearBuffer();
                                        view.clearAudioDisplayData();
                                        audioFilePlayer.loadFileIntoTransport (f); 

                                        if (audioFilePlayer.hasFile())
                                            view.setAudioTransportState (AudioFileTransportController::eAudioTransportState::enFileStopped);
                                        else
                                            view.setAudioTransportState (AudioFileTransportController::eAudioTransportState::enNoFileSelected);
                                    } );

        view.setOnsetSensitivityCallback   ([this] (float s)                              { oscFeatureSender.setOnsetDetectionSensitivity (s); });
        view.setOnsetWindowSizeCallback    ([this] (int s)                                { oscFeatureSender.setOnsetWindowLength (s); });
        view.setOnsetDetectionTypeCallback ([this] (OnsetDetector::eOnsetDetectionType t) { oscFeatureSender.setOnsetDetectionType (t); });
        view.setPlayPressedCallback        ([this] ()                                     { audioFilePlayer.play(); audioDataCollector.clearBuffer(); });
        view.setPausePressedCallback       ([this] ()                                     { audioFilePlayer.pause(); audioDataCollector.clearBuffer(); });
        view.setStopPressedCallback        ([this] ()                                     { audioFilePlayer.stop(); audioDataCollector.clearBuffer(); });
        view.setRestartPressedCallback     ([this] ()                                     { audioFilePlayer.restart(); });

        view.setAddressChangedCallback       ([this] (String address) { return oscFeatureSender.connectToAddress (address); });
        view.setBundleAddressChangedCallback ([this] (String address) { oscFeatureSender.bundleAddress = address; });
        view.setAudioSettingsDeviceManager   (deviceManager);
        view.setDisplayedOSCAddress          (oscFeatureSender.address);
        view.setDisplayedBundleAddress       (oscFeatureSender.bundleAddress);
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
    AudioFilePlayer          audioFilePlayer;
    AudioDataCollector       audioDataCollector;
    RealTimeAnalyser         audioAnalyser;
    OSCFeatureAnalysisOutput oscFeatureSender;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
