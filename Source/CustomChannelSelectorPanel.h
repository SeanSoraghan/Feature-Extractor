/*
  ==============================================================================

    CustomChannelSelectorPanel.h
    Created: 16 Jun 2016 12:56:42pm
    Author:  Sean

  ==============================================================================
*/

#ifndef CUSTOMCHANNELSELECTORPANEL_H_INCLUDED
#define CUSTOMCHANNELSELECTORPANEL_H_INCLUDED

class ChannelSelectorPanel : public Component,
                             private ChangeListener
{
public:
    ChannelSelectorPanel (const CustomAudioDeviceSetupDetails& setupDetails)
    :   setup (setupDetails)
    {
        setup.manager->addChangeListener (this);
        updateControls();
    }

    ~ChannelSelectorPanel()
    {
        setup.manager->removeChangeListener (this);
    }

    void setChannelsConfigAboutToChangeCallback (std::function<void()> f) 
    { 
        if (inputChanList != nullptr)
            inputChanList->setChannelsConfigAboutToChangeCallback (f); 
        if (outputChanList != nullptr)
            outputChanList->setChannelsConfigAboutToChangeCallback (f);
    }

private:
    void resized()
    {
        const int maxListBoxHeight = FeatureExtractorLookAndFeel::getMaxDeviceSettingsListBoxHeight();
        const int h                = FeatureExtractorLookAndFeel::getDeviceSettingsItemHeight();//parent->getItemHeight();
        const int space            = FeatureExtractorLookAndFeel::getInnerComponentSpacing();
        Rectangle<int> r           = getLocalBounds().reduced (FeatureExtractorLookAndFeel::getComponentInset());//(proportionOfWidth (0.35f), 15, proportionOfWidth (0.6f), 3000);
        
        if (outputChanList != nullptr)
        {
            outputChanList->setBounds (r.removeFromTop (outputChanList->getBestHeight (maxListBoxHeight)));
            outputChanLabel->setBounds (0, outputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
            r.removeFromTop (space);
        }

        if (inputChanList != nullptr)
        {
            inputChanList->setBounds (r.removeFromTop (inputChanList->getBestHeight (maxListBoxHeight)));
            inputChanLabel->setBounds (0, inputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
            r.removeFromTop (space);
        }
    }

    void updateControls()
    {
        if (setup.maxNumOutputChannels > 0
                 && setup.minNumOutputChannels < setup.manager->getCurrentAudioDevice()->getOutputChannelNames().size())
        {
            if (outputChanList == nullptr)
            {
                addAndMakeVisible (outputChanList
                    = new CustomChannelSelectorListBox (setup, CustomChannelSelectorListBox::audioOutputType,
                                                    TRANS ("(no audio output channels found)")));
                outputChanLabel = new Label (String::empty, TRANS("Active output channels:"));
                outputChanLabel->setJustificationType (Justification::centredRight);
                outputChanLabel->attachToComponent (outputChanList, true);
            }

            outputChanList->refresh();
        }
        else
        {
            outputChanLabel = nullptr;
            outputChanList = nullptr;
        }

        if (setup.maxNumInputChannels > 0
                && setup.minNumInputChannels < setup.manager->getCurrentAudioDevice()->getInputChannelNames().size())
        {
            if (inputChanList == nullptr)
            {
                addAndMakeVisible (inputChanList
                    = new CustomChannelSelectorListBox (setup, CustomChannelSelectorListBox::audioInputType,
                                                    TRANS("(no audio input channels found)")));
                inputChanLabel = new Label (String::empty, TRANS("Active input channels:"));
                inputChanLabel->setJustificationType (Justification::centredRight);
                inputChanLabel->attachToComponent (inputChanList, true);
            }

            inputChanList->refresh();
        }
        else
        {
            inputChanLabel = nullptr;
            inputChanList = nullptr;
        }

        resized();
    }

    void changeListenerCallback (ChangeBroadcaster*) override
    {
        if (setup.manager != nullptr && setup.manager->getCurrentAudioDevice() != nullptr)
            updateControls();
    }

    const CustomAudioDeviceSetupDetails         setup;
    ScopedPointer<Label>                        inputChanLabel, outputChanLabel;
    ScopedPointer<CustomChannelSelectorListBox> inputChanList, outputChanList;
};



#endif  // CUSTOMCHANNELSELECTORPANEL_H_INCLUDED
