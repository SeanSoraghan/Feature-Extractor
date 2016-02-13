/*
  ==============================================================================

    AudioFileTransportComponent.h
    Created: 12 Feb 2016 5:57:29pm
    Author:  Sean

  ==============================================================================
*/

#ifndef AUDIOFILETRANSPORTCOMPONENT_H_INCLUDED
#define AUDIOFILETRANSPORTCOMPONENT_H_INCLUDED
class AudioTransportButton : public TextButton
{
public:
    AudioTransportButton (FeatureExtractorLookAndFeel::eAudioTransportButtonType t) 
    :   TextButton (String::empty),
        type (t)
    {}

    void paint (Graphics& g)
    {
        FeatureExtractorLookAndFeel::drawAudioTransportButton (g, type, getLocalBounds(), *this);
    }

private:
    FeatureExtractorLookAndFeel::eAudioTransportButtonType type;
};

class AudioFileTransportView : public  Component,
                               public  FileDragAndDropTarget
{
public:
    AudioFileTransportView()
    :   dropFileHere ("file target notifier", "Drop file here"),
        playButton    (FeatureExtractorLookAndFeel::eAudioTransportButtonType::enPlay),
        pauseButton   (FeatureExtractorLookAndFeel::eAudioTransportButtonType::enPause),
        restartButton (FeatureExtractorLookAndFeel::eAudioTransportButtonType::enRestart),
        stopButton    (FeatureExtractorLookAndFeel::eAudioTransportButtonType::enStop)
    {
        dropFileHere.setJustificationType (Justification::centredTop);
        addAndMakeVisible (dropFileHere);
        addChildComponent (playButton);
        addChildComponent (pauseButton);
        addChildComponent (restartButton);
        addChildComponent (stopButton);
    }

    static bool isFileRecognised (File& f)
    {
        String extension = f.getFileExtension();
        return extension == String (".wav") || extension == String (".mp3") || extension == String (".aiff");
    }

    void resized() override
    {
        auto localBounds = getLocalBounds();
        const int controlsWidth  = FeatureExtractorLookAndFeel::getAudioTransportControlsWidth();
        const int controlsHeight = FeatureExtractorLookAndFeel::getAudioTransportControlsHeight();
        auto controlBounds = localBounds.withSizeKeepingCentre (controlsWidth, controlsHeight);

        dropFileHere.setBounds (controlBounds);
        const int  buttonWidth   = controlBounds.getWidth() / 3;
        const auto restartBounds = controlBounds.removeFromLeft (buttonWidth);
        
        restartButton.setBounds (restartBounds);
        const auto playAndPauseBounds = controlBounds.removeFromLeft (buttonWidth);
        pauseButton.setBounds (playAndPauseBounds);
        playButton.setBounds  (playAndPauseBounds);
        const auto stopBounds = controlBounds.removeFromLeft (buttonWidth);
        stopButton.setBounds (stopBounds);
    }

    bool isInterestedInFileDrag (const StringArray& files) override
    {
        if (files.size() > 1)
            return false;

        jassert (files.size() == 1);
        String fileName = files[0];

        if (fileName.endsWith ("wav") || fileName.endsWith ("mp3") || fileName.endsWith ("aiff"))
            return true;

        return false;
    }

    void filesDropped (const StringArray& files, int /*x*/, int /*y*/) override
    {
        jassert (files.size() == 1);
        File file = File (files[0]);

        if (isFileRecognised (file) && validFileDropped != nullptr)
            validFileDropped (file);
    }

    void setFileDroppedCallback (std::function<void (File&)> f) { validFileDropped = f; }

    AudioTransportButton& getPlayButton()    { return playButton; }
    AudioTransportButton& getPauseButton()   { return pauseButton; }
    AudioTransportButton& getRestartButton() { return restartButton; }
    AudioTransportButton& getStopButton()    { return stopButton; }

    void hideAllChildComponents()
    {
        for (int i = 0; i < getNumChildComponents(); i++)
        {
            auto child = getChildComponent (i);
            child->setVisible (false); 
        }
    }

    Label& getLabel() { return dropFileHere; }

private:
    std::function<void (File&)> validFileDropped;

    AudioTransportButton playButton;
    AudioTransportButton pauseButton;
    AudioTransportButton restartButton;
    AudioTransportButton stopButton;
    Label      dropFileHere;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFileTransportView)
};

//============================================================================================
//============================================================================================

class AudioFileTransportController : public ButtonListener
{
public:
    enum eAudioTransportState
    {
        enNoFileSelected,
        enFilePlaying,
        enFilePaused,
        enFileStopped
    };

    AudioFileTransportController()
    {
        view.getPlayButton().addListener    (this);
        view.getPauseButton().addListener   (this);
        view.getStopButton().addListener    (this);
        view.getRestartButton().addListener (this);
    }

    void buttonClicked (Button* b)
    {
        if (b == &view.getPlayButton())
        {
            setAudioTransportState (enFilePlaying);
            if (playPressed != nullptr)
                playPressed();
        }
        else if (b == &view.getPauseButton())
        {
            setAudioTransportState (enFilePaused);
            if (pausePressed != nullptr)
                pausePressed();
        }
        else if (b == &view.getRestartButton())
        {
            setAudioTransportState (enFilePlaying);
            if (restartPressed != nullptr)
                restartPressed();
        }
        else if (b == &view.getStopButton())
        {
            setAudioTransportState (enFileStopped);
            if (stopPressed != nullptr)
                stopPressed();
        }
        else
        {
            jassertfalse;
        }
    }
        
    void setAudioTransportState (eAudioTransportState state)
    {
        view.hideAllChildComponents();

        switch (state)
        {
        case enNoFileSelected:
            view.getLabel().setVisible (true);
            break;
        case enFilePlaying:
            view.getPauseButton().setVisible (true);
            view.getRestartButton().setVisible (true);
            view.getStopButton().setVisible (true);
            break;
        case enFilePaused:
            view.getPlayButton().setVisible (true);
            view.getRestartButton().setVisible (true);
            view.getStopButton().setVisible (true);
            break;
        case enFileStopped:
            view.getPlayButton().setVisible (true);
            break;
        default:
            jassertfalse;
            break;
        }
    }

    AudioFileTransportView& getView() { return view; }

    void setFileDroppedCallback (std::function<void (File&)> f) { view.setFileDroppedCallback (f); }

    void setPlayPressedCallback (std::function<void()> f)    { playPressed = f; }
    void setPausePressedCallback (std::function<void()> f)   { pausePressed = f; }
    void setRestartPressedCallback (std::function<void()> f) { restartPressed = f; }
    void setStopPressedCallback (std::function<void()> f)    { stopPressed = f; }

private:
    std::function<void()> playPressed;
    std::function<void()> pausePressed;
    std::function<void()> restartPressed;
    std::function<void()> stopPressed;

    AudioFileTransportView view;
};





#endif  // AUDIOFILETRANSPORTCOMPONENT_H_INCLUDED
