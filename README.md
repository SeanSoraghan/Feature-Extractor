# Feature-Extractor

Feature Extractor is a real-time audio feature extraction tool. It can analyse either incoming audio from an input device, 
or an audio file (wav, mp3 and aiff formats).

Usage:

Audio input:

The drop-down menu can be used to switch between incoming audio or an audio file. If this menu is set to "Analyse audio file," audio files 
can be drag-and-dropped onto the audio visualiser area.

Feature visualisers:

The central bars show the real-time values of the various features, with their names at the bottom.

Audio settings:

Various audio settings can be set from the bottom-left panel. The advanced settings contain sample rate and audio buffer size.

OSC Settings:

The various audio features will be sent via OSC to the ip address which is set from the app. They will be sent together in an OSC bundle. 
The address of this bundle can also be set from the app. The bundle will contain the various features as an array of floats, between 
0 and 1 or -1 and 1. The features will be ordered as they are in the app from left to right.

Features:

RMS - the root-mean-squared amplitude of the audio signal. This is the average 'volume' of the signal.

Centroid (0 - 1)  - the spectral centroid of the signal. This can be a good indication of the 'brightness' of a sound.

Flatness (0 - 1)  - the spectral flatness of the signal. This can be a good indication of the 'noisiness' of a sound.

Spread   (0 - 1)  - the spectral spread of the audio signal. Pure sinusoidal tones will have close to 0 spread. Board broadband signals will have
                    higher spectral spread.

Slope    (-1 - 1) - the spectral slope of the signal. The 'gradient' of the spectrum. Higher slope values indicate a higher proportion
                    of high-end frequency in the spectrum

