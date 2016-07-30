# Feature-Extractor

Feature Extractor is a real-time audio feature extraction tool. It can analyse audio on multiple input tracks in parallel.

#Dependencies:

The c++ JUCE framework: https://github.com/julianstorer/JUCE

In order to build with the Projucer: Ensure that your JUCE repository folder location is ../JUCE (relative to the feature-extractor repository folder). Then open the Feature-Extractor.jucer file with the Projucer. The Projucer can be found at JUCE/extras/Projucer. If you've just cloned the JUCE repo you'll need to build the Projucer project first. Once you've opened the Feature-Extractor.jucer file in the projucer, click on the config tab and then click 'save and open in IDE' at the bottom left. This will open the project in Visual Studio (windows) or XCode (mac). Then you can build and run the Feature-Extractor app.

#Audio input:

The controls at the top of the app are used to switch bewtween input devices and enable / disable input channels.

#Feature visualisers:

The central bars show the real-time values of the various features, with their names at the bottom.

#Audio settings:

Various audio settings can be set from the bottom-left panel. The advanced settings contain sample rate and audio buffer size.

#OSC Settings:

The various audio features will be sent via OSC to the ip address which is set from the app. They will be sent together in an OSC bundle. 

The address of this bundle can also be set from the app. The bundle will contain the various features as an array of floats, between 
0 and 1. The features will be ordered as they are in the app from left to right.

#Features:

#Amplitude
Onset - the onset value will be 1 when an onset is detected and 0 otherwise.

RMS (Amplitude) - the root-mean-squared amplitude of the audio signal. This is the average 'volume' of the signal.

#Spectral

Centroid - the spectral centroid of the signal. This can be a good indication of the 'brightness' of a sound.

spectral Slope - the slope of the spectrum. Linearly dependent on the centroid.

Spectral Spread - the spread of the spectrum around the mean. signals with narrow bandwidth will have smaller spectral spread.

Spectral Flatness - good indication of the 'noisyness' of the signal (noisy tones have flatter spectrums).

Spectral Flux - the level of change in spectral enery between consecutive frames.

#Harmonic

Pitch - a very crude estimation of the fundamental frequency. It is currently normalised between 0 and 5000 Hz

Harmonic Energy Ratio - the proportion of the energy in the spectrum that is harmonic

Inharmonicity - a measure of how much the peaks in the energy spectrum deviate from their closest harmonics


#OSC Bundle Structure
The OSC bundles will contain 10 floats in the following order:

Onset, RMS amplitude, pitch, centroid, slope, spread, flatness, flux, harmonic energy ratio, inharmonicity
