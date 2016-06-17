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
    AnalyserTrackController (AudioDeviceManager& deviceManagerRef, int channelToAnalyse)
    :   audioDataCollector (channelToAnalyse),
        audioAnalyser      (audioDataCollector), 
        oscFeatureSender   (audioAnalyser),
        deviceManager      (deviceManagerRef)
    {
        audioDataCollector.setNotifyAnalysisThreadCallback ([this]()
        {
            audioAnalyser.notify();
        });

        deviceManager.addAudioCallback (&audioDataCollector);
        audioFilePlayer.setupAudioCallback (deviceManager);

        
    }

    ~AnalyserTrackController()
    {
        deviceManager.removeAudioCallback (audioFilePlayer.getAudioSourcePlayer());
        deviceManager.removeAudioCallback (&audioDataCollector);
        stopAnalysis();
        audioDataCollector.setBufferToDrawUpdatedCallback  (nullptr);
        audioDataCollector.setNotifyAnalysisThreadCallback (nullptr);
        audioAnalyser.setOnsetDetectedCallback             (nullptr);
        setGUITrackSamplesPerBlockCallback                = nullptr;  
    }

    void linkGUIDisplayToTrack (AnalyserTrack* guiTrack, CustomAudioDeviceSetupDetails& audioSetupDetails)
    {
        stopAnalysis();

        audioDataCollector.setBufferToDrawUpdatedCallback ([this, guiTrack] (AudioSampleBuffer& b) 
        {
            MessageManager::getInstance()->callAsync ([this, b, guiTrack]()
            {
                if (guiTrack != nullptr)
                    guiTrack->getAudioDisplayComponent().pushBuffer (b);
            });
        });

        audioAnalyser.setOnsetDetectedCallback ([this, guiTrack] () 
        {
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
            guiTrack->toggleShowTransportControls (type == eAudioSourceType::enAudioFile);
            guiTrack->clearAudioDisplayData();
        });
        
        guiTrack->setFileDroppedCallback ([this, guiTrack] (File& f) 
        { 
            audioDataCollector.clearBuffer();
            guiTrack->clearAudioDisplayData();
            audioFilePlayer.loadFileIntoTransport (f); 

            if (audioFilePlayer.hasFile())
                guiTrack->setAudioTransportState (AudioFileTransportController::eAudioTransportState::enFileStopped);
            else
                guiTrack->setAudioTransportState (AudioFileTransportController::eAudioTransportState::enNoFileSelected);
        } );

        //Overlapped audio visuals
        //guiTrack->getPitchEstimationVisualiser().getOverlappedBufferVisualiser().setGetBufferCallback ([this]()
        //{
        //    return audioAnalyser.getOverlapper().getBufferToDraw();
        //});

        //guiTrack->getPitchEstimationVisualiser().getOverlappedBufferVisualiser().setTagBufferForUpdateCallback ([this]()
        //{
        //    audioAnalyser.getOverlapper().enableBufferToDrawNeedsUpdating();
        //});

        ////Overlapped audio visuals
        //guiTrack->getPitchEstimationVisualiser().getFFTBufferVisualiser().setGetBufferCallback ([this]()
        //{
        //    return audioAnalyser.getFFTAnalyser().getFFTBufferToDraw();
        //});

        //guiTrack->getPitchEstimationVisualiser().getFFTBufferVisualiser().setTagBufferForUpdateCallback ([this]()
        //{
        //    audioAnalyser.getFFTAnalyser().enableFFTBufferToDrawNeedsUpdating();
        //});

        ////autocorrelation visuals
        //guiTrack->getPitchEstimationVisualiser().getAutoCorrelationBufferVisualiser().setGetBufferCallback ([this]()
        //{
        //    return audioAnalyser.getPitchAnalyser().getAutoCorrelationBufferToDraw();
        //});

        //guiTrack->getPitchEstimationVisualiser().getAutoCorrelationBufferVisualiser().setTagBufferForUpdateCallback ([this]()
        //{
        //    audioAnalyser.getPitchAnalyser().enableAutoCorrelationBufferToDrawNeedsUpdating();
        //});

        ////cumulative differnece visuals
        //guiTrack->getPitchEstimationVisualiser().getCumulativeDifferenceBufferVisualiser().setGetBufferCallback ([this]()
        //{
        //    return audioAnalyser.getPitchAnalyser().getCumulativeDifferenceBufferToDraw();
        //});

        //guiTrack->getPitchEstimationVisualiser().getCumulativeDifferenceBufferVisualiser().setGetPeakPositionCallback ([this]()
        //{
        //    return audioAnalyser.getPitchAnalyser().getNormalisedLagPosition();
        //});

        //guiTrack->getPitchEstimationVisualiser().getCumulativeDifferenceBufferVisualiser().setTagBufferForUpdateCallback ([this]()
        //{
        //    audioAnalyser.getPitchAnalyser().enableCumulativeDifferenceBufferNeedsUpdating();
        //});

        ////Pitch value visuals
        //guiTrack->getPitchEstimationVisualiser().setGetPitchEstimateCallback ([this]()
        //{
        //    return audioAnalyser.getAudioFeature (AudioFeatures::eAudioFeature::enF0);
        //});

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
private:
    std::function<void (int)> setGUITrackSamplesPerBlockCallback;
    AudioFilePlayer          audioFilePlayer;
    AudioDataCollector       audioDataCollector;
    RealTimeAnalyser         audioAnalyser;
    OSCFeatureAnalysisOutput oscFeatureSender;
    AudioDeviceManager       &deviceManager;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyserTrackController);
};



#endif  // ANALYSERTRACKCONTROLLER_H_INCLUDED
