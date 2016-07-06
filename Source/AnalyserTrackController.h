/*
  ==============================================================================

    AnalyserTrackController.h
    Created: 16 Jun 2016 4:41:40pm
    Author:  Sean

  ==============================================================================
*/

#ifndef ANALYSERTRACKCONTROLLER_H_INCLUDED
#define ANALYSERTRACKCONTROLLER_H_INCLUDED

class AnalyserTrackController
{
public:
    AnalyserTrackController (AudioDeviceManager& deviceManagerRef, int channelToAnalyse, String nameOfInputChannel, String ip, String bundle)
    :   audioDataCollector (channelToAnalyse),
        audioAnalyser      (audioDataCollector, 1024), 
        oscFeatureSender   (audioAnalyser, ip, bundle),
        deviceManager      (deviceManagerRef),
        channelName        (nameOfInputChannel)
    {
        enabled = channelToAnalyse >= 0;

        if (enabled)
        {
            audioDataCollector.setNotifyAnalysisThreadCallback ([this]()
            {
                audioAnalyser.notify();
            });

            deviceManager.addAudioCallback (&audioDataCollector);
            audioFilePlayer.setupAudioCallback (deviceManager);
        } 
    }

    ~AnalyserTrackController()
    {
        if (enabled)
        {
            deviceManager.removeAudioCallback (audioFilePlayer.getAudioSourcePlayer());
            deviceManager.removeAudioCallback (&audioDataCollector);
        }
        stopAnalysis();
        audioDataCollector.setNotifyAnalysisThreadCallback (nullptr);
        clearGUICallbacks(); 
    }

    void clearGUICallbacks()
    {
        audioDataCollector.setBufferToDrawUpdatedCallback  (nullptr);
        audioAnalyser.setOnsetDetectedCallback             (nullptr);
        setGUITrackSamplesPerBlockCallback                = nullptr;
    }

    void linkGUIDisplayToTrack (AnalyserTrack* guiTrack, CustomAudioDeviceSetupDetails& audioSetupDetails, String oscBundleAddress)
    {
        stopAnalysis();
        guiTrack->setChannelName (getChannelName());
        guiTrack->getOSCSettingsController().getView().setBundleAddress (oscBundleAddress);
        audioDataCollector.setBufferToDrawUpdatedCallback ([this, guiTrack] (AudioSampleBuffer& b) 
        {
            if (guiTrack != nullptr)
                guiTrack->updateBufferToPush (&b);
        });

        audioAnalyser.setOnsetDetectedCallback ([this, guiTrack] () 
        {
            if (guiTrack != nullptr)
                guiTrack->featureTriggered (AudioFeatures::eAudioFeature::enOnset);            
        });
        
        guiTrack->setFeatureValueQueryCallback ([this] (AudioFeatures::eAudioFeature featureType, float maxValue) 
        { 
            return audioAnalyser.getAudioFeature (featureType) / maxValue;
        });

        //switches between listening to input or output
        guiTrack->setAudioSourceTypeChangedCallback ([this, guiTrack] (eAudioSourceType type) 
        { 
            bool input = type == eAudioSourceType::enIncomingAudio;
                                                           
            if (input)
                audioFilePlayer.stop();
                                                            
            audioDataCollector.toggleCollectInput (input);
            JUCE_COMPILER_WARNING("Should these just go durectly after the callback from within auio analyser track?");
            if (guiTrack != nullptr)
            {
                guiTrack->toggleShowTransportControls (type == eAudioSourceType::enAudioFile);
                guiTrack->clearAudioDisplayData();
            }
        });
        
        guiTrack->setFileDroppedCallback ([this, guiTrack] (File& f) 
        { 
            audioDataCollector.clearBuffer();
            if (guiTrack != nullptr)
                guiTrack->clearAudioDisplayData();
            audioFilePlayer.loadFileIntoTransport (f); 

            if (guiTrack != nullptr)
            {
                if (audioFilePlayer.hasFile())
                    guiTrack->setAudioTransportState (AudioFileTransportController::eAudioTransportState::enFileStopped);
                else
                    guiTrack->setAudioTransportState (AudioFileTransportController::eAudioTransportState::enNoFileSelected);
            }
        } );

        guiTrack->setGainChangedCallback ([this] (float g) { audioDataCollector.setGain (g); });

        guiTrack->setOnsetSensitivityCallback   ([this] (float s)                              { audioAnalyser.setOnsetDetectionSensitivity (s); });
        guiTrack->setOnsetWindowSizeCallback    ([this] (int s)                                { audioAnalyser.setOnsetWindowLength (s); });
        guiTrack->setOnsetDetectionTypeCallback ([this] (OnsetDetector::eOnsetDetectionType t) { audioAnalyser.setOnsetDetectionType (t); });
        guiTrack->setPlayPressedCallback        ([this] ()                                     { audioFilePlayer.play(); audioDataCollector.clearBuffer(); });
        guiTrack->setPausePressedCallback       ([this] ()                                     { audioFilePlayer.pause(); audioDataCollector.clearBuffer(); });
        guiTrack->setStopPressedCallback        ([this] ()                                     { audioFilePlayer.stop(); audioDataCollector.clearBuffer(); });
        guiTrack->setRestartPressedCallback     ([this] ()                                     { audioFilePlayer.restart(); });

        guiTrack->setAddressChangedCallback       ([this] (String address) { return oscFeatureSender.connectToAddress (address); });
        guiTrack->setBundleAddressChangedCallback ([this] (String address) { oscFeatureSender.bundleAddress = address; });

        
        guiTrack->setDisplayedOSCAddress          (oscFeatureSender.address);
        guiTrack->setDisplayedBundleAddress       (oscFeatureSender.bundleAddress);

        setGUITrackSamplesPerBlockCallback = [this, guiTrack](int samplesPerBlockExpected)
        {
            if (guiTrack != nullptr)
                guiTrack->getAudioDisplayComponent().setSamplesPerBlock (samplesPerBlockExpected);
        };

        AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup (setup);
        prepareToPlay (setup.bufferSize, setup.sampleRate);

        guiTrack->startAnimation();
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate)
    {
        stopAnalysis();
        audioAnalyser.sampleRateChanged (sampleRate);

        if (setGUITrackSamplesPerBlockCallback != nullptr)
            setGUITrackSamplesPerBlockCallback (samplesPerBlockExpected);
        
        audioAnalyser.startThread (4);
        audioDataCollector.setExpectedSamplesPerBlock (samplesPerBlockExpected);
    }

    void stopAnalysis()
    {
        audioAnalyser.stopThread (100);
    }


    String getChannelName() const noexcept { return channelName; }
    bool isEnabled()        const noexcept { return enabled; }
private: 
    std::function<void (int)> setGUITrackSamplesPerBlockCallback;
    AudioFilePlayer          audioFilePlayer;
    AudioDataCollector       audioDataCollector;
    RealTimeHarmonicAnalyser audioAnalyser;
    OSCFeatureAnalysisOutput oscFeatureSender;
    AudioDeviceManager       &deviceManager;
    String                   channelName;
    bool                     enabled { true };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyserTrackController);
};



#endif  // ANALYSERTRACKCONTROLLER_H_INCLUDED
