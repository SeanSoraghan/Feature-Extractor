/*
  ==============================================================================

    AudioAnalysis.h
    Created: 6 Aug 2015 10:17:49am
    Author:  Sean Soraghan

  ==============================================================================
*/

#ifndef AUDIOANALYSIS_H_INCLUDED
#define AUDIOANALYSIS_H_INCLUDED


struct AudioAnalyser
{
    struct SpectralCharacteristics
    {
        SpectralCharacteristics (float sCentroid, float sSpread, float sFlatness)
        :   centroid (sCentroid),
            spread   (sSpread),
            flatness (sFlatness)
        {}
        
        float centroid;
        float spread;
        float flatness;
    };
    
    struct HarmonicCharacteristics
    {
        HarmonicCharacteristics (float fundamental, float her, float inharm)
        :   f0                  (fundamental),
            harmonicEnergyRatio (her),
            inharmonicity       (inharm)
        {}

        float f0;
        float harmonicEnergyRatio;
        float inharmonicity;
    };
    
    struct F0Candidate
    {
        F0Candidate (int frequencyBinInterval) 
        :   binInterval (frequencyBinInterval), 
            count       (1) 
        {}
        
        F0Candidate (double her, double f)
        :   harmonicEnergyRatio (her),
            frequency           (f)
        {}

        void incrementCount() { count++; }

        void updateFrequency (double frequencyRangePerBin) 
        { 
            frequency = (double) binInterval * frequencyRangePerBin; 
        }

        double getWeightedCount()
        {
            return (double) count * harmonicEnergyRatio;
        }

        void updateFrequencyAndHarmonicEnergyRatio (AudioSampleBuffer& fftResults, 
                                                    int channel, 
                                                    double frequencyRangePerBin, 
                                                    double totalMagnitude, 
                                                    double numHarmonics)
        {
            updateFrequency (frequencyRangePerBin);
            updateHarmonicEnergyRatio (fftResults, channel, frequencyRangePerBin, totalMagnitude, numHarmonics);
        }

        void updateHarmonicEnergyRatio (AudioSampleBuffer& fftResults, 
                                        int channel, 
                                        double frequencyRangePerBin, 
                                        double totalMagnitude, 
                                        double numHarmonics)
        {
            double harmonicScore = 0.0;
            for (double harmonic = 1.0; harmonic < numHarmonics + 1.0; harmonic++)
            {
                double harmonicFrequency = frequency * harmonic;
                int harmonicBin = (int) (ceil (harmonicFrequency / frequencyRangePerBin));

                if (harmonicBin >= fftResults.getNumSamples())
                    break;

                double binMagnitude = (double) fftResults.getSample (channel, harmonicBin);
                harmonicScore += binMagnitude;
            }
            harmonicScore /= totalMagnitude;
            harmonicEnergyRatio = harmonicScore;
        }

        double harmonicEnergyRatio {0.0};
        double frequency {0.0};
        int    binInterval {0}; //the frequency measured in number of bins (e.g. if binInterval = 2, frequency = 2 * frequencyRangePerBin)
        int    count {1};
    };

    AudioAnalyser (int windowSize, int numChannels, double nyquistFrequency, bool spectralFeatures, bool harmonicFeatures)
    :   nyquist    (nyquistFrequency),
        previousF0 (0.0),
        fftIn      (numChannels, windowSize),
        fftOut     (numChannels, windowSize / 2 + 1),
        fft        (/*order*/int (log(windowSize)/log(2) + 1e-6), /*inverse*/false),
        analyseSpectralCharacteristics (spectralFeatures),
        analyseHarmonicCharacteristics (harmonicFeatures)
    {
        DBG("AudioAnalyser WindowSize: "<<windowSize);
    }
    
    void performSpectralAnalysis (ConcatenatedFeatureBuffer& features)
    {
        
        // calculates an FFT window for each sample in the downsampled waveform.
        
        int numInputSamples =    features.audioOutput.getNumSamples();
        int stepSize =           numInputSamples / features.numDownsamples;
        int numFFTInputSamples = fftIn.getNumSamples();
        int numChannels =        features.audioOutput.getNumChannels();
        
        jassert (stepSize > 0);
        
        int numOutputFFTFrames = features.numDownsamples;
        
        features.nyquistFrequency = features.sampleRate / 2.0;
        features.energyEnvelope.setSize (numChannels, numOutputFFTFrames);

        for (int frame = 0; frame < numOutputFFTFrames; ++frame)
        {
            fftIn.clear();

            int rangeStart  = -numFFTInputSamples / 2;
            int rangeEnd    = numFFTInputSamples / 2;
            int offsetToAdd = numFFTInputSamples / 2;

            //We don't want to pad half of every realtime window with 0s.
            if (numOutputFFTFrames == 1)
            {
                jassert (numFFTInputSamples <= numInputSamples);
                rangeStart = 0;
                rangeEnd = numFFTInputSamples;
                offsetToAdd = 0;
            }

            for (int channel = 0; channel < numChannels; channel++)
            {
                for (int sample = rangeStart; sample < rangeEnd; ++sample) // this is shifted back by half to center window around the current sample
                {
                    int i = frame * stepSize + sample; // index of the sample in the full audio buffer
                    
                    if (i < 0 || i >= numInputSamples) // zero padding for ends
                    {
                        fftIn.getWritePointer (channel)[sample + numFFTInputSamples / 2] = 0.0f;
                    }
                    else
                    {
                        jassert (i >= 0 && i < numInputSamples);
                        fftIn.getWritePointer (channel)[sample + offsetToAdd] = features.audioOutput.getReadPointer (channel)[i]; // copy samples into fftIn
                    }
                }
            }
            
            scaleBufferWithBartlettWindowing (fftIn);
            
            jassert (numFFTInputSamples <= 4096);
            float temp[4096];
            
            for (int channel = 0; channel < numChannels; channel++)
            {
                auto floatIn = (const float*) fftIn.getReadPointer(channel);
                auto bytes = size_t (numFFTInputSamples) * sizeof(float);
                memcpy((void*)temp, (void*)floatIn, bytes);
                fft.performFrequencyOnlyForwardTransform (temp);
                
                auto floatOut = (float*) fftOut.getWritePointer(channel);
                memcpy((void*)floatOut, (void*)temp, bytes / 2 + sizeof(float)); // We have half as many amplitudes.
                
                if (analyseSpectralCharacteristics)
                {
                    SpectralCharacteristics characteristics = calculateSpectralCharacteristics (fftOut, channel);
                    features.setFeatureSample (ConcatenatedFeatureBuffer::Feature::Centroid, frame, characteristics.centroid, channel);
                    features.setFeatureSample (ConcatenatedFeatureBuffer::Feature::Spread, frame, characteristics.spread, channel);
                    features.setFeatureSample (ConcatenatedFeatureBuffer::Feature::Flatness, frame, characteristics.flatness, channel);
                    float slope = calculateNormalisedSpectralSlope (fftOut, channel);
                    features.setFeatureSample (ConcatenatedFeatureBuffer::Feature::Slope, frame, slope, channel);
                    features.setFFTBinsForSample (fftOut.getReadPointer (channel), channel, frame);
                }
                if (analyseHarmonicCharacteristics)
                {
                    HarmonicCharacteristics characteristics = calculateHarmonicCharacteristics (fftOut, channel);
                    features.setFeatureSample (ConcatenatedFeatureBuffer::Feature::F0, frame, characteristics.f0, channel);
                    features.setFeatureSample (ConcatenatedFeatureBuffer::Feature::HER, frame, characteristics.harmonicEnergyRatio, channel);
                    features.setFeatureSample (ConcatenatedFeatureBuffer::Feature::Inharmonicity, frame, characteristics.inharmonicity, channel);
                }
            }
            /* maybe use RMS for energy envelope? Or just multiply each total by a gain factor?*/
            features.energyEnvelope.setSample (0, frame, sumAccrossChannels (fftOut));
        }
    }
    
    HarmonicCharacteristics calculateHarmonicCharacteristics (AudioSampleBuffer& fftResults, int channel)
    {
        std::vector<int> peakBins;
        peakBins.clear();

        std::vector<F0Candidate> frequencyHistogram;
        frequencyHistogram.clear();

        size_t numBins = (size_t) fftResults.getNumSamples();
        //calculate mean bin magnitude and maximum bin magnitude
        double meanMagnitude = 0.0;
        double magnitudeSum = 0.0;
        for (size_t i = 0; i < numBins; i++)
        {
            double binMagnitude = (double) fftResults.getSample (channel, (int)i);
            magnitudeSum += binMagnitude;
        }
        meanMagnitude = magnitudeSum / (double) numBins;

        if (magnitudeSum < 0.001)
            return {0.0, 0.0, 0.0};

        fillPeakBinsAndFrequencyHistogram (fftResults, channel, peakBins, frequencyHistogram, meanMagnitude);

        F0Candidate f0 = estimateF0AndHERFromFrequencyHistogram (fftResults, channel, frequencyHistogram, magnitudeSum);
        if (previousF0 != f0.frequency && previousF0 > 10.0)
        {
            double top = previousF0 > f0.frequency ? previousF0 : f0.frequency;
            double bottom = top == previousF0 ? f0.frequency : previousF0;
            double newF0Ratio = top / bottom; 
            if (newF0Ratio > 2.0)
            {
                double eps = 0.1;
                if (newF0Ratio - floor (newF0Ratio) < eps)
                {
                    f0.frequency = previousF0;
                    f0.updateHarmonicEnergyRatio (fftResults, 
                                                  channel, 
                                                  nyquist / (double) numBins, 
                                                  magnitudeSum, 
                                                  15.0);
                }
            }
        }
        previousF0 = f0.frequency;
        DBG("F0Estimate: "<<f0.frequency);
        double inharmonicity = 0.0;

        if (f0.frequency > 0.0)
            inharmonicity = calculateInharmonicity (fftResults, channel, f0, peakBins, magnitudeSum);

        return {(float) f0.frequency, (float) f0.harmonicEnergyRatio, (float) inharmonicity}; 
    }

    double calculateInharmonicity (AudioSampleBuffer& fftResults, int channel, F0Candidate f0Estimate, std::vector<int>& peakBins, double magnitudeSum)
    {
        double frequencyRangePerBin = nyquist / (double) fftResults.getNumSamples();
        double inharmonicity = 0.0;
        for (auto bin : peakBins)
        {

            if (getBinForFrequency (fftResults, f0Estimate.frequency) == bin) //f0 exists in this bin, so don't add any inharmonicity score
                continue;

            double binStartFrequency = bin * frequencyRangePerBin;

            if (binStartFrequency == 0.0)//avoid peak in bin 1 leading to division by 0
                binStartFrequency = frequencyRangePerBin * 0.5;

            double binEndFrequency   = (double) (bin + 1) * frequencyRangePerBin;
            double binStartF0Ratio   = getFrequencyRatio (binStartFrequency, f0Estimate.frequency);
            double binEndF0Ratio     = getFrequencyRatio (binEndFrequency, f0Estimate.frequency);

            if (floor (binStartF0Ratio) != floor (binEndF0Ratio))//A multiple of f0 exists within this bin
                continue;

            double f0Ratio = binStartF0Ratio < binEndF0Ratio ? binStartF0Ratio : binEndF0Ratio;
            double f0Proportion = f0Ratio - floor (f0Ratio);
            double binEnergyRatio = ((double) fftResults.getSample (channel, bin) / magnitudeSum);
            jassert (binEnergyRatio < 1.0);
            inharmonicity += f0Proportion * binEnergyRatio;
            jassert (inharmonicity < 1.0);
        }
        jassert (inharmonicity >= 0.0);
        return inharmonicity;
    }

    double getFrequencyRatio (double frequency1, double frequency2)
    {
        if (frequency1 == frequency2)
            return 1.0;

        double higher = frequency1 > frequency2 ? frequency1 : frequency2;
        double lower  = higher == frequency1 ? frequency2 : frequency1;
        return higher / lower;
    }

    void fillPeakBinsAndFrequencyHistogram (AudioSampleBuffer& fftResults, 
                                            int channel, 
                                            std::vector<int>& peakBins, 
                                            std::vector<F0Candidate>& histogram,
                                            double meanMagnitude)
    {
        size_t numBins = (size_t) fftResults.getNumSamples();
        for (size_t bin = 0; bin < numBins; bin++)
        {
            bool peak = binIsPeak ((int) bin, fftResults, channel, meanMagnitude);
            
            if (peak)
                addNewPeakBinAndUpdateHistogram ((int) bin, peakBins, histogram);
        }
    }

    bool binIsPeak (int bin, AudioSampleBuffer& fftResults, int channel, double meanMagnitude)
    {
        int numBins = fftResults.getNumSamples();
        double binMagnitude = (double) fftResults.getSample (channel, bin);
        //return binMagnitude > meanMagnitude;
        
        if (binMagnitude <= meanMagnitude)
            return false;

        //offsets to ensure no out of bounds checks
        int leftBinOffset  = bin < 2 ? 2 - bin : 0;
        int rightBinOffset = bin >= numBins - 2 ? 2 - ((numBins - 1) - bin) : 0; 
        for (int neighbour = bin - (2 - leftBinOffset); neighbour < bin + (2 - rightBinOffset); neighbour++)
        {
            double neighbourMagnitude = (double) fftResults.getSample (channel, neighbour);
            if (neighbour != bin && neighbourMagnitude > binMagnitude)
                return false;
        }
        return true;
    }

    void addNewPeakBinAndUpdateHistogram (int newPeakBin, std::vector<int>& peakBins, std::vector<F0Candidate>& histogram)
    {
        peakBins.push_back(newPeakBin);
        for (size_t p = 0; p < peakBins.size() - 1; p++)
        {
            int newBinInterval = newPeakBin - peakBins[p];
            /*getPositionOfIntervalInHistogram() returns -1 if interval doesnt exist in histogram*/
            int intervalPositionInHistogram = getPositionOfIntervalInHistogram (newBinInterval, histogram);
            if (intervalPositionInHistogram != -1)
                histogram[intervalPositionInHistogram].incrementCount();
            else
                histogram.push_back (F0Candidate (newBinInterval));
        }
    }

    int getPositionOfIntervalInHistogram (int queryBinInterval, std::vector<F0Candidate>& histogram)
    {
        for (size_t candidate = 0; candidate < histogram.size(); candidate++)
            if (histogram[candidate].binInterval == queryBinInterval)
                return (int) candidate;
        /*return -1 if interval is not in histogram*/
        return -1; 
    }

    F0Candidate estimateF0AndHERFromFrequencyHistogram (AudioSampleBuffer& fftResults, 
                                                        int channel, 
                                                        std::vector<F0Candidate>& histogram,
                                                        double magnitudeSum)
    {
        double maxWeightedCount = 0.0;
        double f0 = 0.0;
        double her = 0.0;
        jassert (fftResults.getNumSamples() > 0);
        double frequencyRangePerBin = nyquist / (double)fftResults.getNumSamples();
        for (auto candidate : histogram)
        {
            candidate.updateFrequencyAndHarmonicEnergyRatio (fftResults, channel, frequencyRangePerBin, magnitudeSum, 15.0);
            double currentWeightedCount = candidate.getWeightedCount ();
            if (currentWeightedCount > maxWeightedCount)
            {
                maxWeightedCount = currentWeightedCount;
                f0  = candidate.frequency;
                her = candidate.harmonicEnergyRatio;
            }
        }
        return F0Candidate (her, f0);
    }

    int getBinForFrequency (AudioSampleBuffer& fftResults, double freq)
    {
        size_t numBins = (size_t)fftResults.getNumSamples();
        double frequencyRangePerBin = nyquist / numBins;
        return (int)(ceil(freq / frequencyRangePerBin));
    }

    double calculateHarmonicEnergyRatio (AudioSampleBuffer& fftResults, int channel, double magnitudeSum, double f0, int numHarmonics)
    {
        double harmonicMagnitude = 0.0;
        double harmonicEnergyRatio = 0.0;
        if (magnitudeSum > 0.001)
        {
            for (int h = 0; h < numHarmonics; h++)
            {
                double harmonicFrequency = (h + 1) * f0;
                int bin = getBinForFrequency (fftResults, harmonicFrequency);

                if (bin >= (int) fftResults.getNumSamples())
                    break;

                harmonicMagnitude += (double) fftResults.getSample (channel, bin);
            }
            harmonicEnergyRatio = harmonicMagnitude / magnitudeSum;
        }
        return harmonicEnergyRatio;
    }

    SpectralCharacteristics calculateSpectralCharacteristics (AudioSampleBuffer& fftResults, int channel)
    {
        size_t numBins = (size_t)fftResults.getNumSamples();
        double frequencyRangePerBin = nyquist / numBins;
        
        double weightedMagnitudeSum = 0.0;
        double varMagnitudeSum      = 0.0;
        double magnitudeSum         = 0.0;
        double magnitudeProduct     = 1.0;
        
        std::vector<double> binCentreFrequencies((size_t)numBins);
        std::vector<double> binMagnitudes((size_t)numBins);
        
        for (size_t i = 0; i < numBins; ++i)
        {
            double binCentreFrequency = double(i) * frequencyRangePerBin + (frequencyRangePerBin / 2.0);
            binCentreFrequencies[i] = binCentreFrequency;
            double binMagnitude = (double) fftResults.getSample (channel, (int)i);
            binMagnitudes[i] = binMagnitude;
            
            magnitudeSum += binMagnitude;
            magnitudeProduct *= binMagnitude;
            weightedMagnitudeSum += binCentreFrequency * binMagnitude;
        }
        
        double eps = 0.001;
        if (!(magnitudeSum > eps))
            return {0.0f, 0.0f, 0.0f};
        float centroid = (float) (weightedMagnitudeSum / magnitudeSum);
        //        float scaledC = (float)(log2 (1.0 + 1023.0 * (centroid)) / 10.0);
        
        double invNumBins = 1.0 / numBins;
        float flatness = (float) (pow (magnitudeProduct, invNumBins) / (invNumBins * magnitudeSum));
        for (size_t i = 0; i < numBins; ++i)
        {
            varMagnitudeSum += pow ((binCentreFrequencies[i] / nyquist) - (centroid / nyquist), 2.0) * binMagnitudes[i];
        }
        float maxSpread = (float) ((centroid / nyquist) * (1.0 - (centroid / nyquist)));
        float spread = (float) ((varMagnitudeSum / magnitudeSum) / maxSpread);
        return {centroid / (float)nyquist, spread, flatness};
    }
    
    void analyseNormalisedZeroCrosses (ConcatenatedFeatureBuffer& features)
    {
        int numChannels = features.audioOutput.getNumChannels();
        int numInputSamples = features.audioOutput.getNumSamples();
        int stepSize = numInputSamples / features.numDownsamples;
        
        for (int channel = 0; channel < numChannels; channel++)
        {
            for (int i = 0; i < features.numDownsamples; i++)
            {
                float numZeroCrosses = 0;
                int sample = i * stepSize;
                for (int s = 0; s < stepSize - 1; ++s)
                {
                    float first =  features.audioOutput.getReadPointer (channel)[sample + s];
                    float second = features.audioOutput.getReadPointer (channel)[sample + s + 1];
                    
                    if ((first > 0.0f && (first - second) > first) ||
                        (first < 0.0f && (first - second) < first))
                        numZeroCrosses++;
                }
                features.setFeatureSample (ConcatenatedFeatureBuffer::Feature::ZeroCrosses, i, numZeroCrosses * 2.0f / (float)stepSize, channel);
            }
        }
    }
    
    void calculateFFTLBP (AudioSampleBuffer& fftResults, AudioSampleBuffer& previousFFTFrame, int channel)
    {
        size_t numBins = (size_t)fftResults.getNumSamples();
        float threshold = 0.1f;
        std::vector<int> lbp(numBins);
        double decimal = 0.0;
        float totalDiffs = 0.0f;
        float highestBinActivity = 0.0f;
        float totalSum = 0.0f;
        for (size_t i = 0; i < numBins; i++)
        {
            totalSum ++;
            float diff = abs (fftResults.getSample (channel, (int)i) - previousFFTFrame.getSample (channel, (int)i));
            lbp[i] = (int) (diff > threshold);
            totalDiffs += (float)lbp[i];
            decimal = decimal * 2.0 + (double)lbp[i];
            if (diff > threshold)
                highestBinActivity = (float)i;
            std::cout<<lbp[i]<<"|";
        }
        std::cout<<" - "<<highestBinActivity / (float)numBins<<" - "<<totalDiffs / totalSum<<std::endl;
    }
    
    float calculateNormalisedSpectralSlope (AudioSampleBuffer& fftResults, int channel)
    {
        //calc means
        double numBins = (double) fftResults.getNumSamples();
        double meanBin = 0.5;
        double meanEnergy = 0.0;
        double prodSum = 0.0;
        double fftMagnitude = fftResults.getMagnitude (channel, 0, (int)numBins);
        
        double eps = 0.0001;
        if (! (fftMagnitude > eps))
            return 0.0f;
        
        for (int i = 0; i < (int) numBins; i++)
        {
            double normedEnergy = fftResults.getSample (channel, i) / fftMagnitude;
            jassert (normedEnergy >= 0.0 && normedEnergy <= 1.0);
            meanEnergy += normedEnergy;
            prodSum += (double)i * normedEnergy;
        }
        meanEnergy /= numBins;
        
        //calc std devs
        double binVar = 0.0;
        double energyVar = 0.0;
        for (double i = 0.0; i < numBins; i++)
        {
            double normedI = i / numBins;
            binVar += (normedI - meanBin) * (normedI - meanBin);
            double normedEnergy = fftResults.getSample (channel, (int)i) / fftMagnitude;
            energyVar += (normedEnergy - meanEnergy) * (normedEnergy - meanEnergy);
        }
        binVar /= numBins;
        energyVar /= numBins;
        double binStd = sqrt (binVar);
        double energyStd = sqrt (energyVar);
        
        //calculate sample correlation coefficient
        double rMagBin = (prodSum - (numBins * meanEnergy * meanBin)) / (numBins - 1.0f) * energyStd * binStd;
        
        //calculate gradient of best fit
        double grad = rMagBin * (binStd / energyStd);
        return (float) grad;
    }
    
    void setLogAttackTime (ConcatenatedFeatureBuffer& features)
    {
        AudioSampleBuffer &energyEnvelope = features.energyEnvelope;
        int numSamples  = energyEnvelope.getNumSamples();
        float maxEnergy = energyEnvelope.findMinMax (0, 0, numSamples).getEnd();
        int i = 0;
        while (i < numSamples && energyEnvelope.getSample (0, i) != maxEnergy)
            i++;
        double msPerSample = 1.0 / (double)(features.sampleRate / 1000);
        int samplesPerStep = features.audioOutput.getNumSamples() / features.numDownsamples;
        features.estimatedLogAttackTime = (float) (log10 ((double)(float)(i * samplesPerStep) * (float)msPerSample));
    }
    
    static void scaleBufferWithBartlettWindowing (AudioSampleBuffer& source) // TODO: change to DSPMultiBlock
    {
        int numSamples = source.getNumSamples();
        
        for (int channel = 0; channel < source.getNumChannels(); ++channel)
        {
            source.applyGainRamp (channel, 0, numSamples / 2, 0.0f, 1.0f);
            source.applyGainRamp (channel, numSamples / 2, numSamples / 2, 1.0f, 0.0f);
        }
    }
    
    void setWindowSize (int samplesPerWindow)
    {
        fftIn.setSize  (fftIn.getNumChannels(),  samplesPerWindow);
        fftOut.setSize (fftOut.getNumChannels(), samplesPerWindow / 2 + 1);
    }

    double            nyquist;
    double            previousF0;
    AudioSampleBuffer fftIn;
    AudioSampleBuffer fftOut;
    FFT               fft;
    bool              analyseSpectralCharacteristics;
    bool              analyseHarmonicCharacteristics;
    
private:
    static float sumAccrossChannels (AudioSampleBuffer& buffer)
    {
        float sum = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); channel++)
        {
            for (int i = 0; i < buffer.getNumSamples(); i++)
            {
                sum += buffer.getSample (channel, i);
            }
        }
        return sum;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioAnalyser)
};


#endif  // AUDIOANALYSIS_H_INCLUDED
