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
        sendSpectralFeaturesViaOSC (true);
    }

    void sendSpectralFeaturesViaOSC (bool updateHarmonicFeatures)
    {
        float rmsLevel = realTimeAudioFeatures.getAudioFeature (AudioFeatures::eAudioFeature::enRMS);
        float centroid = realTimeAudioFeatures.getAudioFeature (AudioFeatures::eAudioFeature::enCentroid);
        float flatness = realTimeAudioFeatures.getAudioFeature (AudioFeatures::eAudioFeature::enFlatness);
        float spread =   realTimeAudioFeatures.getAudioFeature (AudioFeatures::eAudioFeature::enSpread);
        float slope =    realTimeAudioFeatures.getAudioFeature (AudioFeatures::eAudioFeature::enSlope);
        float onset =    realTimeAudioFeatures.getAudioFeature (AudioFeatures::eAudioFeature::enOnset);  
        if (updateHarmonicFeatures)
        {
            float f0     =  realTimeAudioFeatures.getAudioFeature (AudioFeatures::eAudioFeature::enF0);
            float her    =  realTimeAudioFeatures.getAudioFeature (AudioFeatures::eAudioFeature::enHarmonicEnergyRatio);
            float inharm =  realTimeAudioFeatures.getAudioFeature (AudioFeatures::eAudioFeature::enInharmonicity);  
            //DBG("F0 estimation: "<<f0<<" |her: "<<her<<" |inharm: "<<inharm);
            //sender.send (bundleAddress, onset, rmsLevel, centroid, flatness, spread, slope, f0, her, inharm);
            sender.send (bundleAddress, onset, rmsLevel, f0, centroid, flatness);
        }
        else
        {
            //sender.send (bundleAddress, onset, rmsLevel, centroid, flatness, spread, slope);
        }  
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
