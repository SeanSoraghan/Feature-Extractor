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
    AnalyserTrackController (AudioDeviceManager& deviceManagerRef, int channelToAnalyse, String nameOfInputChannel, String ip, String secondaryIP, String bundle)
    :   audioDataCollectorHarm    (channelToAnalyse),
        audioDataCollectorSpec    (channelToAnalyse),
        audioAnalyserHarm         (audioDataCollectorHarm, features, 2048),
        audioAnalyserSpec         (audioDataCollectorSpec, features, 2048),
        oscFeatureSender          (features, ip, bundle),
        secondaryOSCFeatureSender (features, secondaryIP, bundle),
        deviceManager             (deviceManagerRef),
        channelName               (nameOfInputChannel)
    {
        enabled = channelToAnalyse >= 0;

        if (enabled)
        {
            audioDataCollectorHarm.setNotifyAnalysisThreadCallback ([this]()
            {
                audioAnalyserHarm.notify();
            });
            deviceManager.addAudioCallback (&audioDataCollectorHarm);

            audioDataCollectorSpec.setNotifyAnalysisThreadCallback ([this]()
            {
                audioAnalyserSpec.notify();
            });
            deviceManager.addAudioCallback (&audioDataCollectorSpec);

            audioFilePlayer.setupAudioCallback (deviceManager);
        } 
    }

    ~AnalyserTrackController()
    {
        if (enabled)
        {
            deviceManager.removeAudioCallback (audioFilePlayer.getAudioSourcePlayer());
            deviceManager.removeAudioCallback (&audioDataCollectorHarm);
            deviceManager.removeAudioCallback (&audioDataCollectorSpec);
        }
        stopAnalysis();
        audioDataCollectorHarm.setNotifyAnalysisThreadCallback (nullptr);
        audioDataCollectorSpec.setNotifyAnalysisThreadCallback (nullptr);
        clearGUICallbacks(); 
    }

    void clearGUICallbacks()
    {
        audioDataCollectorHarm.setBufferToDrawUpdatedCallback  (nullptr);
        audioDataCollectorSpec.setBufferToDrawUpdatedCallback  (nullptr);
        audioAnalyserSpec.setOnsetDetectedCallback             (nullptr);
        setGUITrackSamplesPerBlockCallback                = nullptr;
    }

    void linkGUIDisplayToTrack (AnalyserTrack* guiTrack, const CustomAudioDeviceSetupDetails& audioSetupDetails, String oscBundleAddress)
    {
        stopAnalysis();
        guiTrack->setChannelName (getChannelName());
        guiTrack->getOSCSettingsController().getView().setBundleAddress (oscBundleAddress);
        audioDataCollectorHarm.setBufferToDrawUpdatedCallback ([this, guiTrack] (AudioSampleBuffer& b) 
        {
            if (guiTrack != nullptr)
                guiTrack->updateBufferToPush (&b);
        });

        audioAnalyserSpec.setOnsetDetectedCallback ([this, guiTrack] () 
        {
            if (guiTrack != nullptr)
                guiTrack->featureTriggered (AudioFeatures::eAudioFeature::enOnset);            
        });
        
        guiTrack->setFeatureValueQueryCallback ([this] (AudioFeatures::eAudioFeature featureType, float maxValue) 
        { 
            return getAudioFeature (featureType) / maxValue;
        });

        //switches between listening to input or output
        guiTrack->setAudioSourceTypeChangedCallback ([this, guiTrack] (eAudioSourceType type) 
        { 
            bool input = type == eAudioSourceType::enIncomingAudio;
                                                           
            if (input)
                audioFilePlayer.stop();
                                                            
            audioDataCollectorHarm.toggleCollectInput (input);
            audioDataCollectorSpec.toggleCollectInput (input);
            JUCE_COMPILER_WARNING("Should these just go durectly after the callback from within auio analyser track?");
            if (guiTrack != nullptr)
            {
                guiTrack->toggleShowTransportControls (type == eAudioSourceType::enAudioFile);
                guiTrack->clearAudioDisplayData();
            }
        });
        
        guiTrack->setFileDroppedCallback ([this, guiTrack] (File& f) 
        { 
            audioDataCollectorHarm.clearBuffer();
            audioDataCollectorSpec.clearBuffer();
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

        guiTrack->setGainChangedCallback ([this] (float g) 
        { 
            audioDataCollectorHarm.setGain (g);
            audioDataCollectorSpec.setGain (g);
        });

        guiTrack->setOnsetSensitivityCallback   ([this] (float s)                              { audioAnalyserSpec.setOnsetDetectionSensitivity (s); });
        guiTrack->setOnsetWindowSizeCallback    ([this] (int s)                                { audioAnalyserSpec.setOnsetWindowLength (s); });
        guiTrack->setOnsetDetectionTypeCallback ([this] (OnsetDetector::eOnsetDetectionType t) { audioAnalyserSpec.setOnsetDetectionType (t); });
        guiTrack->setPlayPressedCallback        ([this] ()                                     { audioFilePlayer.play(); clearAnalysisBuffers(); });
        guiTrack->setPausePressedCallback       ([this] ()                                     { audioFilePlayer.pause(); clearAnalysisBuffers(); });
        guiTrack->setStopPressedCallback        ([this] ()                                     { audioFilePlayer.stop(); clearAnalysisBuffers(); });
        guiTrack->setRestartPressedCallback     ([this] ()                                     { audioFilePlayer.restart(); });

        guiTrack->setAddressChangedCallback          ([this] (String address) { return oscFeatureSender.connectToAddress (address); });
        guiTrack->setSecondaryAddressChangedCallback ([this] (String address) { return secondaryOSCFeatureSender.connectToAddress (address); });
        
        guiTrack->setBundleAddressChangedCallback ([this] (String address) 
        { 
            oscFeatureSender.bundleAddress = address; 
            secondaryOSCFeatureSender.bundleAddress = address;
        });

        
        guiTrack->setDisplayedOSCAddress          (oscFeatureSender.address);
        guiTrack->setSecondaryDisplayedOSCAddress (secondaryOSCFeatureSender.address);
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

    void clearAnalysisBuffers()
    {
        audioDataCollectorHarm.clearBuffer();
        audioDataCollectorSpec.clearBuffer();
    }

    float getAudioFeature      (AudioFeatures::eAudioFeature featureType) const { return features.getValue (featureType); }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate)
    {
        stopAnalysis();
        audioAnalyserHarm.sampleRateChanged (sampleRate);
        audioAnalyserSpec.sampleRateChanged (sampleRate);

        if (setGUITrackSamplesPerBlockCallback != nullptr)
            setGUITrackSamplesPerBlockCallback (samplesPerBlockExpected);
        
        audioAnalyserHarm.startThread (4);
        audioAnalyserSpec.startThread (4);
        audioDataCollectorHarm.setExpectedSamplesPerBlock (samplesPerBlockExpected);
        audioDataCollectorSpec.setExpectedSamplesPerBlock (samplesPerBlockExpected);
    }

    void stopAnalysis()
    {
        audioAnalyserHarm.stopThread (100);
        audioAnalyserSpec.stopThread (100);
    }

    String getChannelName() const noexcept { return channelName; }
    bool isEnabled()        const noexcept { return enabled; }
private: 
    std::function<void (int)> setGUITrackSamplesPerBlockCallback;
    AudioFilePlayer          audioFilePlayer;
    AudioFeatures            features;
    AudioDataCollector       audioDataCollectorHarm;
    AudioDataCollector       audioDataCollectorSpec;
    RealTimeHarmonicAnalyser audioAnalyserHarm;
    RealTimeSpectralAnalyser audioAnalyserSpec;
    OSCFeatureAnalysisOutput oscFeatureSender;
    OSCFeatureAnalysisOutput secondaryOSCFeatureSender;
    AudioDeviceManager       &deviceManager;
    String                   channelName;
    bool                     enabled { true };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyserTrackController);
};



#endif  // ANALYSERTRACKCONTROLLER_H_INCLUDED
