/*
  ==============================================================================

    OSCFeatureAnalysisOutput.h
    Created: 23 Oct 2015 1:27:22pm
    Author:  Sean

  ==============================================================================
*/

#ifndef OSCFEATUREANALYSISOUTPUT_H_INCLUDED
#define OSCFEATUREANALYSISOUTPUT_H_INCLUDED

#include <iostream>

typedef ConcatenatedFeatureBuffer::Feature Feature;



//==============================================================================
//==============================================================================
class OnsetDetector
{
public:
    enum eOnsetDetectionType
    {
        enSpectral = 0,
        enAmplitude,
        enCombination,
        enNumTypes
    };

    static String getStringForDetectionType (eOnsetDetectionType t)
    {
        switch (t)
        {
            case enSpectral:
                return String ("Spectral");
            case enAmplitude:
                return String ("Amplitude");
            case enCombination:
                return String ("Combination");
            default:
                jassertfalse;
                return String ("UNKNOWN");
        }
    }

    OnsetDetector()
    :   spectralFluxHistory (5),
        ampHistory          (5),
        type                (enAmplitude)
    {}

    void addSpectralFluxAndAmpValue (float sf, float amp)
    {
        spectralFluxHistory.insertNewValueAndupdateHistory (sf);
        ampHistory.insertNewValueAndupdateHistory (amp);
    }

    bool detectOnset()
    {
        jassert (ampHistory.history.size() == spectralFluxHistory.history.size());
        //avoid division by 0
        if (ampHistory.recordedHistory == 0 || spectralFluxHistory.recordedHistory == 0)
            return false;

        if (spectralFluxHistory.recordedHistory < (int) spectralFluxHistory.history.size()
            || ampHistory.recordedHistory < (int) ampHistory.history.size())
            return false;

        float meanSpectralFlux = spectralFluxHistory.getTotal() / spectralFluxHistory.recordedHistory;
        float meanAmp          = ampHistory.getTotal() / ampHistory.recordedHistory;
        
        int onsetCandidteIndex = spectralFluxHistory.history.size() - 1;

        if (type == enSpectral || type == enCombination)
            onsetCandidteIndex = spectralFluxHistory.history.size() / 2;

        float candidateSF      = spectralFluxHistory.history[onsetCandidteIndex];
        float candidateAmp     = ampHistory.history[onsetCandidteIndex];

        float ampThreshold = 0.01f;
       
        if (candidateAmp < ampThreshold)
            return false;

        for (int i = 0; i < (int) spectralFluxHistory.history.size(); i++)
        {
            if (i != onsetCandidteIndex)
            {
                float neighbourFlux = spectralFluxHistory.history[i];
                float neighbourAmp  = ampHistory.history[i];

                if (neighbourAmp >= candidateAmp && (type == enAmplitude || type == enCombination))
                    return false;

                if (neighbourFlux >= candidateSF && (type == enSpectral || type == enCombination))
                    return false;
            }
        }

        bool onsetSpectral  = candidateSF  > meanSpectralFlux * meanThresholdMultiplier;
        bool onsetAmplitude = candidateAmp > meanAmp * meanThresholdMultiplier;

        switch (type)
        {
            case enAmplitude:
                return onsetAmplitude;
            case enSpectral:
                return onsetSpectral;
            case enCombination:
                return onsetAmplitude && onsetSpectral;
            default:
                jassertfalse;
                return false;
        }
    }

    ValueHistory        spectralFluxHistory;
    ValueHistory        ampHistory;
    eOnsetDetectionType type;
    float               meanThresholdMultiplier { 1.7f };
};

//==============================================================================
//==============================================================================

class OSCFeatureAnalysisOutput : private Timer
{
public:
    enum OSCFeatureType
    {
        Onset = 0,
        RMS,
        Centroid,
        Flatness,
        Spread,
        Slope,
        F0,
        HER,
        Inharmonicity,
        NumFeatures
    };

    static bool isTriggerFeature (OSCFeatureType oscf) 
    {
        if (oscf == Onset)
            return true;

        return false;
    }

    static String getOSCFeatureName (OSCFeatureType f)
    {
        switch (f)
        {
            case Onset:         return "Onset";
            case RMS:           return "RMS";
            case Centroid:      return "Centroid";
            case Slope:         return "Slope";
            case Spread:        return "Spread";
            case Flatness:      return "Flatness";
            case F0:            return "F0";
            case HER:           return "HER";
            case Inharmonicity: return "Inharm";
            case NumFeatures:   return "NumFeatures";
            default: jassert(false); return "UNKNOWN";
        }
    }

    OSCFeatureAnalysisOutput (RealTimeAnalyser& rta)
    :   realTimeAudioFeatures (rta)
    {
        for (int f = 1; f < OSCFeatureType::NumFeatures; f++)
        {
            if (f > (int) OSCFeatureType::Slope)                            //harmonic
                featureHistories.push_back (ValueHistory (8));
            else                                                            //Spectral
                featureHistories.push_back (ValueHistory (5));
        }
        connectToAddress (IPAddress::local().toString());
    }

    void timerCallback ()
    {
        if (realTimeAudioFeatures.getSpectralFeatures().audioOutput.getNumSamples() > 0)
            sendSpectralFeaturesViaOSC (realTimeAudioFeatures.getHarmonicFeatures().audioOutput.getNumSamples() > 0);
    }

    void sendSpectralFeaturesViaOSC (bool updateHarmonicFeatures)
    {
        float rmsLevel = getRunningAverageAndUpdateHistory (Feature::Audio, OSCFeatureType::RMS);
        float centroid = getRunningAverageAndUpdateHistory (Feature::Centroid, OSCFeatureType::Centroid);
        float flatness = getRunningAverageAndUpdateHistory (Feature::Flatness, OSCFeatureType::Flatness);
        float spread =   getRunningAverageAndUpdateHistory (Feature::Spread, OSCFeatureType::Spread);
        float slope =    getRunningAverageAndUpdateHistory (Feature::Slope, OSCFeatureType::Slope);
        float onset =    detectOnset ();  
        if (updateHarmonicFeatures)
        {
            float f0     =  getRunningAverageAndUpdateHistory (Feature::F0, OSCFeatureType::F0);
            float her    =  getRunningAverageAndUpdateHistory (Feature::HER, OSCFeatureType::HER);
            float inharm =  getRunningAverageAndUpdateHistory (Feature::Inharmonicity, OSCFeatureType::Inharmonicity);  
            //DBG("F0 estimation: "<<f0<<" |her: "<<her<<" |inharm: "<<inharm);
            sender.send (bundleAddress, onset, rmsLevel, centroid, flatness, spread, slope, f0 / (7902.13f/* B8 */ - 16.35f/* C0 */), her, inharm);
        }
        else
        {
            sender.send (bundleAddress, onset, rmsLevel, centroid, flatness, spread, slope);
        }  
    }

    float getRunningAverage (OSCFeatureType oscf)
    {
        jassert (oscf != OSCFeatureType::Onset);

        ValueHistory rms = featureHistories[OSCFeatureType::RMS];
        float rmsAverage = rms.getTotal() / (float) rms.recordedHistory;
        if (rmsAverage > 0.01f)
        {
            ValueHistory& h = featureHistories[oscf - 1];
            return h.getTotal() / (float) h.recordedHistory;
        }
        return 0.0f;
    }

    float detectOnset ()
    {
        float currentValue = realTimeAudioFeatures.getSpectralFeatures().getAverageFeatureSample (Feature::Flux, 0);
        
        /* RMS or magnitude? */
        float amp = realTimeAudioFeatures.getSpectralFeatures().getMaxAmplitude();
        //float amp = realTimeAudioFeatures.getSpectralFeatures().getAverageRMSLevel();
        //
        onsetDetector.addSpectralFluxAndAmpValue (currentValue, amp);
        return onsetDetector.detectOnset() ? 1.0f : 0.0f;
    }

    float getRunningAverageAndUpdateHistory (Feature f, OSCFeatureType oscf)
    {
        /* use detectOnset() for onsets */
        jassert (oscf != OSCFeatureType::Onset);
        
        float currentValue = 0.0f;

        if (oscf == OSCFeatureType::RMS)
            currentValue = realTimeAudioFeatures.getSpectralFeatures().getAverageRMSLevel();// max amp or rms?
        else if (ConcatenatedFeatureBuffer::isFeatureSpectral (f))
            currentValue = realTimeAudioFeatures.getSpectralFeatures().getAverageFeatureSample (f, 0);
        else
            currentValue = realTimeAudioFeatures.getHarmonicFeatures().getAverageFeatureSample (f, 0);
        
        ValueHistory& h = featureHistories[oscf - 1];
        
        float returnValue = currentValue;
        if (h.recordedHistory >= 1)
        {
            float average = h.getTotal() / h.recordedHistory;
            if (currentValue < 1.3f * average && currentValue > 0.7f * average)
                returnValue = (h.getTotal() + currentValue) / (h.recordedHistory + 1);
        }

        float eps = 0.0001f;

        if (returnValue < eps)
            returnValue = 0.0f;

        h.insertNewValueAndupdateHistory (returnValue);
        return returnValue;
    }

    bool connectToAddress (String newAddress)
    {
        int port = 9000;
        int portSeparatorIndex = newAddress.lastIndexOfAnyOf (":");

        if (portSeparatorIndex != -1)
            port = newAddress.fromLastOccurrenceOf (":", false, true).getIntValue();

        address = newAddress.upToFirstOccurrenceOf (":", false, true);

        DBG("connecting to: "<<address);
        if (! sender.connect (address, port))
        {
            DBG ("Error: could not connect to UDP port "<<port);
            return false;
        }
        else
        {
            startTimerHz (FeatureExtractorLookAndFeel::getAnimationRateHz());
            return true;
        }
    }

    void setOnsetDetectionSensitivity (float s)
    {
        jassert (s >= 0.0f);
        onsetDetector.meanThresholdMultiplier = 1.0f + s;
    }

    void setOnsetWindowLength (int length)
    {
        onsetDetector.ampHistory.setHistoryLength          (length);
        onsetDetector.spectralFluxHistory.setHistoryLength (length);
    }

    void setOnsetDetectionType (OnsetDetector::eOnsetDetectionType t) { onsetDetector.type = t; }

    RealTimeAnalyser&         realTimeAudioFeatures;
    OSCSender                 sender;
    OnsetDetector             onsetDetector;
    std::vector<ValueHistory> featureHistories;
    std::function<void()>     onsetDetectedCallback;
    String                    address;
    String bundleAddress      { String("/Audio/Features") };
};



#endif  // OSCFEATUREANALYSISOUTPUT_H_INCLUDED
