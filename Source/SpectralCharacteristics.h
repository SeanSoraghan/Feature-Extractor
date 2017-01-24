/*
  ==============================================================================

    SpectralCharacteristics.h
    Created: 24 Apr 2016 3:56:00pm
    Author:  Sean

  ==============================================================================
*/

#ifndef SPECTRALCHARACTERISTICS_H_INCLUDED
#define SPECTRALCHARACTERISTICS_H_INCLUDED

struct SpectralCharacteristics
{
    SpectralCharacteristics (float sCentroid, float sSpread, float sFlatness, float sLER, float sFlux)
    :   centroid (sCentroid),
        spread   (sSpread),
        flatness (sFlatness),
        ler      (sLER),
        flux     (sFlux)
    {}
        
    float centroid;
    float spread;
    float flatness;
    float ler;
    float flux;
};

class SpectralCharacteristicsAnalyser
{
public:
    SpectralCharacteristicsAnalyser (int numSamplesPerWindow) 
    {
        for (int i = 0; i < numSamplesPerWindow / 2; i++)
            previousBinMagnitudes.push_back (0.0f);
    }

    struct IntermediateSpectralCharacteristics
    {
        IntermediateSpectralCharacteristics (size_t numMagnitudes) : binCentreFrequencies (numMagnitudes),
                                                                     binMagnitudes        (numMagnitudes)
        {}

        double weightedMagnitudeSum = 0.0;
        double varMagnitudeSum      = 0.0;
        double magnitudeSum         = 0.0;
        double magnitudeProduct     = 1.0;
        double flatnessMagnitudeSum = 0.0;
        double flux                 = 0.0;
        double lhr                  = 0.0;
        double numMagnitudesUsedInFlatnessCalculation = 0.0;
        std::vector<double> binCentreFrequencies;
        std::vector<double> binMagnitudes;

        float calculateFlatness (double eps, double invNumMagnitudes)
        {
            return flatnessMagnitudeSum > eps ? (float) (pow (magnitudeProduct, invNumMagnitudes) / (invNumMagnitudes * flatnessMagnitudeSum)) : 0.0f;
        }

        void fillIntermediateValues (AudioSampleBuffer& fftResults, std::vector<double>& previousBinMags, int channel, int numMagnitudes, double eps, double nyquist)
        {
            double frequencyRangePerBin = nyquist / numMagnitudes;
            int lowerPortion = numMagnitudes / 5;
            for (size_t magnitude = 0; magnitude < (size_t) numMagnitudes; magnitude++)
            {
                int fftIndex = magnitude * 2;

                double binCentreFrequency = double(magnitude) * frequencyRangePerBin + (frequencyRangePerBin / 2.0);
                binCentreFrequencies[magnitude] = binCentreFrequency;
                double binValue = (double) fftResults.getSample (channel, (int)fftIndex);
                double binMagnitude = binValue * binValue;

                /*flux*/
                double diff = abs (binMagnitude) - abs (previousBinMags[magnitude]);
                double rectifiedDiff = (diff + abs (diff)) / 2.0;
                if (diff > 0.0)
                    flux += rectifiedDiff;
                ///////
            
                binMagnitudes[magnitude] = binMagnitude;
            
                magnitudeSum += binMagnitude;
            
                if (magnitude == lowerPortion)
                    lhr = magnitudeSum;

                if (binMagnitude > eps)
                {
                    flatnessMagnitudeSum += binMagnitude;
                    magnitudeProduct     *= binMagnitude;
                    numMagnitudesUsedInFlatnessCalculation ++;
                }
                weightedMagnitudeSum += binCentreFrequency * binMagnitude;
            }
        }
    };

    SpectralCharacteristics calculateSpectralCharacteristics (AudioSampleBuffer& fftResults, double rms, int channel, double nyquist)
    {
        jassert (fftResults.getNumSamples() % 2 == 0);
        const int numFFTElements    = (int) fftResults.getNumSamples() / 2;
        const int numMagnitudes     = (int) numFFTElements / 2;
        double frequencyRangePerBin = nyquist / numMagnitudes;
        IntermediateSpectralCharacteristics intermediates (numMagnitudes);
        jassert (previousBinMagnitudes.size() == intermediates.binMagnitudes.size());
        double eps = 0.01 * rms;
        intermediates.fillIntermediateValues (fftResults, previousBinMagnitudes, channel, numMagnitudes, eps, nyquist);
        
        float maxFlux = (numMagnitudes * (numMagnitudes + 1)) / 2.0f;
        intermediates.flux /= maxFlux;
        return calculateSpectralCharacteristicsFromIntermediates (intermediates, eps, nyquist, numMagnitudes);
    }

    SpectralCharacteristics calculateSpectralCharacteristicsFromIntermediates (IntermediateSpectralCharacteristics intermediates, 
                                                                               double eps,
                                                                               double nyquist,
                                                                               int numMagnitudes)
    {
        double epsSum = 0.05;
        if (!(intermediates.magnitudeSum > epsSum))
            return {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

        intermediates.lhr /= intermediates.magnitudeSum;

        float centroid = (float) (intermediates.weightedMagnitudeSum / intermediates.magnitudeSum);
        
        const double flatnessMags = intermediates.numMagnitudesUsedInFlatnessCalculation;
        double invNumMagnitudes = 1.0 / (flatnessMags > 0.0 ? flatnessMags : 1.0);
        float flatness = intermediates.calculateFlatness (eps, invNumMagnitudes);
        float logFlatness = log10 (flatness * 9.0 + 1.0);
        float c = centroid / (float)(nyquist / 2.0);
        float logCentroid = log10 (c * 9.0f + 1.0f);
        for (size_t i = 0; i < (size_t) numMagnitudes; ++i)
        {
            intermediates.varMagnitudeSum += pow ((intermediates.binCentreFrequencies[i] / nyquist) - (centroid / nyquist), 2.0) * intermediates.binMagnitudes[i];
            previousBinMagnitudes[i] = intermediates.binMagnitudes[i];
        }
        float maxSpread = (float) ((centroid / nyquist) * (1.0 - (centroid / nyquist)));
        float spread = (float) ((intermediates.varMagnitudeSum / intermediates.magnitudeSum) / maxSpread);
        return {logCentroid, spread, logFlatness, (float) intermediates.lhr, (float) intermediates.flux};
    }

    float calculateNormalisedSpectralSlope (AudioSampleBuffer& fftResults, int channel)
    {
        //calc means
        const int numFFTElements = (int) fftResults.getNumSamples() / 2;
        const int numMagnitudes  = numFFTElements / 2;
        double meanBin = 0.5;
        double meanEnergy = 0.0;
        double prodSum = 0.0;
        double maxFFTMagnitude = fftResults.getMagnitude (channel, 0, numMagnitudes);
        std::vector<double> binMagnitudes ((size_t)numMagnitudes);

        for (int i = 0; i < numMagnitudes; i++)
        {
            double binValue = fftResults.getSample (channel, i * 2);
            double binMagnitude = binValue * binValue;
            binMagnitudes[i] = binMagnitude;
            if (binMagnitude > maxFFTMagnitude)
                maxFFTMagnitude = binMagnitude;
        }

        double eps = 0.0001;
        if (! (maxFFTMagnitude > eps))
            return 0.0f;
        
        for (int i = 0; i < (int) numMagnitudes; i++)
        {
            double binMagnitude = binMagnitudes[i];
            double normedEnergy = binMagnitude / maxFFTMagnitude;
            jassert (normedEnergy >= 0.0 && normedEnergy <= 1.0);
            meanEnergy += normedEnergy;
            prodSum += (double)i * normedEnergy;
        }
        meanEnergy /= (double) numMagnitudes;
        
        //calc std devs
        double binVar = 0.0;
        double energyVar = 0.0;
        for (double i = 0.0; i < numMagnitudes; i++)
        {
            double normedI = i / (double) numMagnitudes;
            binVar += (normedI - meanBin) * (normedI - meanBin);
            double normedEnergy = binMagnitudes[(int)i] / maxFFTMagnitude;
            energyVar += (normedEnergy - meanEnergy) * (normedEnergy - meanEnergy);
        }
        binVar /= (double) numMagnitudes;
        energyVar /= (double) numMagnitudes;
        double binStd = sqrt (binVar);
        double energyStd = sqrt (energyVar);
        
        //calculate sample correlation coefficient
        double rMagBin = (prodSum - (numMagnitudes * meanEnergy * meanBin)) / (numMagnitudes - 1.0f) * energyStd * binStd;
        
        //calculate gradient of best fit
        double grad = rMagBin * (binStd / energyStd);
        return (float) grad;
    }

private:
    std::vector<double> previousBinMagnitudes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralCharacteristicsAnalyser)
};

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
#endif  // SPECTRALCHARACTERISTICS_H_INCLUDED
