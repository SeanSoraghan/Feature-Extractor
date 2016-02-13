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
    :   currentHistoryIndex (historyLength - 1),
        recordedHistory (0)
    {
        history.clear();
        for (int i = 0; i < historyLength; i++)
        {
            history.push_back (0.0f);
        }
    }

    float getTotal()
    {
        float av = 0.0f;
        for (int i = 0; i < (int)history.size(); i++)
        {
            av += history[i];
        }
        return av;
    }
    
    void updateHistoryIndex()
    {
        currentHistoryIndex --;

        if (currentHistoryIndex < 0)
            currentHistoryIndex = (int)history.size() - 1;

        if (recordedHistory < (int)history.size())
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
    int currentHistoryIndex;
    int recordedHistory;
};

class OSCFeatureAnalysisOutput : private Timer
{
public:
    enum OSCFeatureType
    {
        RMS = 0,
        Centroid,
        Flatness,
        Spread,
        Slope,
        F0,
        HER,
        Inharmonicity,
        NumFeatures
    };

    static String getOSCFeatureName (OSCFeatureType f)
    {
        switch (f)
        {
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

    static OSCFeatureType oscFeatureTypeFromFeature (ConcatenatedFeatureBuffer::Feature f)
    {
        typedef ConcatenatedFeatureBuffer::Feature Feature;
        switch (f)
        {
            case Feature::Audio:
                return OSCFeatureType::RMS;
            case Feature::Centroid:
                return OSCFeatureType::Centroid;
            case Feature::Flatness:
                return OSCFeatureType::Flatness;
            case Feature::Spread:
                return OSCFeatureType::Spread;
            case Feature::Slope:
                return OSCFeatureType::Slope;
            case Feature::HER:
                return OSCFeatureType::HER;
            case Feature::Inharmonicity:
                return OSCFeatureType::Inharmonicity;
            case Feature::F0:
                return OSCFeatureType::F0;
            default:
                jassertfalse;
                return OSCFeatureType::NumFeatures;
        }
    }

    OSCFeatureAnalysisOutput (RealTimeAnalyser& rta)
    :   realTimeAudioFeatures (rta)
    {
        for (int f = 0; f < OSCFeatureType::NumFeatures; f++)
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
            sender.send (bundleAddress, rmsLevel, centroid, flatness, spread, slope);
        }
    }

    float getRunningAverage (OSCFeatureType oscf)
    {
        ValueHistory rms = featureHistories[OSCFeatureType::RMS];
        float rmsAverage = rms.getTotal() / (float) rms.recordedHistory;
        if (rmsAverage > 0.01f)
        {
            ValueHistory& h = featureHistories[oscf];
            return h.getTotal() / (float) h.recordedHistory;
        }
        return 0.0f;
    }

    float getRunningAverageAndUpdateHistory (Feature f, OSCFeatureType oscf)
    {
        float currentValue = 0.0f;

        if (oscf == OSCFeatureType::RMS)
            currentValue = realTimeAudioFeatures.getSpectralFeatures().getAverageRMSLevel();
        else if (ConcatenatedFeatureBuffer::isFeatureSpectral (f))
            currentValue = realTimeAudioFeatures.getSpectralFeatures().getAverageFeatureSample (f, 0);
        else
            currentValue = realTimeAudioFeatures.getHarmonicFeatures().getAverageFeatureSample (f, 0);

        ValueHistory& h = featureHistories[oscf];
        //float rms = featureHistories[OSCFeatureType::RMS].history[featureHistories[OSCFeatureType::RMS].currentHistoryIndex];
        //float lastValue = h.history[h.currentHistoryIndex];
        //float diff = currentValue - lastValue;
        //float weightedDiff = diff;
        //if (oscf != OSCFeatureType::F0 && oscf != OSCFeatureType::RMS)
           // weightedDiff = (1.0f - (float) log10 ((double) (((1.0f - diff) * 9.0f) + 1.0f))) * jlimit (0.0f, 1.0f, rms);
        //float augmentedValue = lastValue + weightedDiff;
        float returnValue = (h.getTotal() + currentValue) / (h.recordedHistory + 1);
        h.history[h.currentHistoryIndex] = returnValue;
        h.updateHistoryIndex();
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

    RealTimeAnalyser&         realTimeAudioFeatures;
    OSCSender                 sender;
    std::vector<ValueHistory> featureHistories;
    String                    address;
    String bundleAddress      { String("/Audio/Features") };
};



#endif  // OSCFEATUREANALYSISOUTPUT_H_INCLUDED
