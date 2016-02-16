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

struct ValueHistory 
{
    ValueHistory (int historyLength)
    :   recordedHistory (0)
    {
        history.clear();
        for (int i = 0; i < historyLength; i++)
        {
            history.push_back (0.0f);
        }
    }

    float getTotal()
    {
        float total = 0.0f;
        for (int i = 0; i < (int)history.size(); i++)
        {
            total += history[i];
        }
        return total;
    }

    void insertNewValueAndupdateHistory (float newValue)
    {
        for (int i = 0; i < (int) history.size() - 1; i++)
        {
            history[i] = history[i + 1];
        }
        history[history.size() - 1] = newValue;
        
        //updateHistoryIndex();
        
        if (recordedHistory < (int) history.size())
            recordedHistory ++;
    }

    void printHistory()
    {
        String h (String::empty);
        for (int i = 0; i < (int)history.size(); i++)
        {
            h<<String (history[i]);
            h<<" ";
        }
        DBG(h);
    }

    std::vector<float> history;
    int recordedHistory;
};

//==============================================================================
//==============================================================================
class OnsetDetector
{
public:
    OnsetDetector()
    :   spectralFluxHistory (3),
        rmsHistory (3)
    {}

    void addSpectralFluxAndRMSValue (float sf, float rms)
    {
        spectralFluxHistory.insertNewValueAndupdateHistory (sf);
        rmsHistory.insertNewValueAndupdateHistory (rms);
    }

    bool detectOnset()
    {
        if (spectralFluxHistory.recordedHistory < (int) spectralFluxHistory.history.size())
            return false;

        float meanSpectralFlux = spectralFluxHistory.getTotal() / spectralFluxHistory.recordedHistory;
        
        int onsetCandidteIndex = spectralFluxHistory.history.size() / 2;
        float candidateSF      = spectralFluxHistory.history[onsetCandidteIndex];
        float candidateRMS     = rmsHistory.history[onsetCandidteIndex];

        for (int i = 0; i < (int) spectralFluxHistory.history.size(); i++)
        {
            if (i != onsetCandidteIndex)
            {
                float neighbourFlux = spectralFluxHistory.history[i];
                float neighbourRMS  = rmsHistory.history[i];
                if (neighbourFlux >= candidateSF || neighbourRMS >= candidateRMS)
                    return false;
            }
        }

        return candidateSF > meanSpectralFlux * meanThresholdMultiplier;
    }

    ValueHistory spectralFluxHistory;
    ValueHistory rmsHistory;
    float        meanThresholdMultiplier { 1.7f };
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
            case Onset:       return "Onset";
            case RMS:         return "RMS";
            case Centroid:    return "Centroid";
            case Slope:       return "Slope";
            case Spread:      return "Spread";
            case Flatness:    return "Flatness";
            case F0:          return "F0";
            case NumFeatures: return "NumFeatures";
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

    void sendSpectralFeaturesViaOSC (bool sendHarmonicFeatures)
    {
        float centroid = getRunningAverageAndUpdateHistory (Feature::Centroid, OSCFeatureType::Centroid);
        float flatness = getRunningAverageAndUpdateHistory (Feature::Flatness, OSCFeatureType::Flatness);
        float spread =   getRunningAverageAndUpdateHistory (Feature::Spread, OSCFeatureType::Spread);
        float slope =    getRunningAverageAndUpdateHistory (Feature::Slope, OSCFeatureType::Slope);
        float rmsLevel = getRunningAverageAndUpdateHistory (Feature::Audio, OSCFeatureType::RMS);
        float onset =    detectOnset (rmsLevel); 
        if (sendHarmonicFeatures)
        {
            float f0     =  getRunningAverageAndUpdateHistory (Feature::F0, OSCFeatureType::F0);
            float her    =  getRunningAverageAndUpdateHistory (Feature::HER, OSCFeatureType::HER);
            float inharm =  getRunningAverageAndUpdateHistory (Feature::Inharmonicity, OSCFeatureType::Inharmonicity);
            /*if (! */sender.send ("/Equator/Feature/All", rmsLevel, centroid, flatness, spread, slope, f0, her, inharm);/*)*/
                //DBG ("Error: could not send OSC message.");
            DBG("F0 estimation: "<<f0<<" |her: "<<her<<" |inharm: "<<inharm);
        }
        else
        {
            if (onset == 1.0f && onsetDetectedCallback != nullptr)
            {
                onsetDetectedCallback();
                DBG ("onset");
                onsetDetector.spectralFluxHistory.printHistory();
            }
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
            ValueHistory& h = featureHistories[oscf];
            return h.getTotal() / (float) h.recordedHistory;
        }
        return 0.0f;
    }

    float detectOnset (float rms)
    {
        float currentValue = realTimeAudioFeatures.getSpectralFeatures().getAverageFeatureSample (Feature::Flux, 0);
        onsetDetector.addSpectralFluxAndRMSValue (currentValue, rms);
        return onsetDetector.detectOnset() ? 1.0f : 0.0f;
    }

    float getRunningAverageAndUpdateHistory (Feature f, OSCFeatureType oscf)
    {
        /* use detectOnset() for onsets */
        jassert (oscf != OSCFeatureType::Onset);
        
        float currentValue = 0.0f;

        if (oscf == OSCFeatureType::RMS)
            currentValue = realTimeAudioFeatures.getSpectralFeatures().getAverageRMSLevel();
        else if (ConcatenatedFeatureBuffer::isFeatureSpectral (f))
            currentValue = realTimeAudioFeatures.getSpectralFeatures().getAverageFeatureSample (f, 0);
        else
            currentValue = realTimeAudioFeatures.getHarmonicFeatures().getAverageFeatureSample (f, 0);
        
        ValueHistory& h = featureHistories[oscf];

        float returnValue = (h.getTotal() + currentValue) / (h.recordedHistory + 1);
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
        jassert (s >= 0.0f && s <= 1.0f);
        onsetDetector.meanThresholdMultiplier = 1.0f + s;
    }

    RealTimeAnalyser&         realTimeAudioFeatures;
    OSCSender                 sender;
    OnsetDetector             onsetDetector;
    std::vector<ValueHistory> featureHistories;
    std::function<void()>     onsetDetectedCallback;
    String                    address;
    String bundleAddress      { String("/Audio/Features") };
};



#endif  // OSCFEATUREANALYSISOUTPUT_H_INCLUDED
