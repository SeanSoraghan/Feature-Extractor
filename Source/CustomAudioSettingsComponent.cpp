/*
  ==============================================================================

    CustomAudioSettingsComponent.cpp
    Created: 16 Jun 2016 11:34:21am
    Author:  Sean

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "FeatureExtractorLookAndFeel.h"
#include "CustomChannelSelectorController.h"
#include "CustomChannelSelectorPanel.h"
#include "CustomAudioSettingsComponent.h"
/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/
class CustomSimpleDeviceManagerInputLevelMeter  : public Component,
                                                  public Timer
{
public:
    CustomSimpleDeviceManagerInputLevelMeter (AudioDeviceManager& m)
        : manager (m), level (0)
    {
        startTimer (50);
        manager.enableInputLevelMeasurement (true);
    }

    ~CustomSimpleDeviceManagerInputLevelMeter()
    {
        manager.enableInputLevelMeasurement (false);
    }

    void timerCallback() override
    {
        if (isShowing())
        {
            const float newLevel = (float) manager.getCurrentInputLevel();

            if (std::abs (level - newLevel) > 0.005f)
            {
                level = newLevel;
                repaint();
            }
        }
        else
        {
            level = 0;
        }
    }

    void paint (Graphics& g) override
    {
        getLookAndFeel().drawLevelMeter (g, getWidth(), getHeight(),
                                         (float) exp (log (level) / 3.0)); // (add a bit of a skew to make the level more obvious)
    }

private:
    AudioDeviceManager& manager;
    float level;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomSimpleDeviceManagerInputLevelMeter)
};
//==============================================================================


//==============================================================================

static String getNoDeviceString()   { return "<< " + TRANS("none") + " >>"; }

class AudioDeviceSettingsPanel : public Component,
                                 private ChangeListener,
                                 private ComboBoxListener  // (can't use ComboBox::Listener due to idiotic VC2005 bug)
{
public:
    AudioDeviceSettingsPanel (AudioIODeviceType& t, CustomAudioDeviceSetupDetails& setupDetails, std::function<void()> audioDeviceAboutToChangeCB)
        :   type (t), 
            setup (setupDetails), 
            audioDeviceAboutToChangeCallback (audioDeviceAboutToChangeCB)
    {
        type.scanForDevices();

        setup.manager->addChangeListener (this);
    }

    ~AudioDeviceSettingsPanel()
    {
        setup.manager->removeChangeListener (this);
    }

    void resized() override
    {
        Rectangle<int> total    = getLocalBounds();
        const int comboBoxWidth = (int) (total.getWidth() * FeatureExtractorLookAndFeel::getDeviceSettingsComboBoxWidthRatio());

        auto r = total.removeFromRight (comboBoxWidth);

        const int maxListBoxHeight = FeatureExtractorLookAndFeel::getMaxDeviceSettingsListBoxHeight();
        const int h                = FeatureExtractorLookAndFeel::getDeviceSettingsItemHeight();
        const int space            = FeatureExtractorLookAndFeel::getInnerComponentSpacing();

        if (outputDeviceDropDown != nullptr)
        {
            Rectangle<int> row (r.removeFromTop (h));

            outputDeviceDropDown->setBounds (row);
            r.removeFromTop (space);
        }

        if (inputDeviceDropDown != nullptr)
        {
            Rectangle<int> row (r.removeFromTop (h));
            inputDeviceDropDown->setBounds (row);
            r.removeFromTop (space);
        }

        if (sampleRateDropDown != nullptr)
        {
            sampleRateDropDown->setBounds (r.removeFromTop (h));
            r.removeFromTop (space);
        }

        if (bufferSizeDropDown != nullptr)
        {
            bufferSizeDropDown->setBounds (r.removeFromTop (h));
            r.removeFromTop (space);
        }
    }

    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override
    {
        if (comboBoxThatHasChanged == nullptr)
            return;

        AudioDeviceManager::AudioDeviceSetup config;
        setup.manager->getAudioDeviceSetup (config);
        String error;

        if (comboBoxThatHasChanged == outputDeviceDropDown
              || comboBoxThatHasChanged == inputDeviceDropDown)
        {
            audioDeviceAboutToChange();

            if (outputDeviceDropDown != nullptr)
                config.outputDeviceName = outputDeviceDropDown->getSelectedId() < 0 ? String::empty
                                                                                    : outputDeviceDropDown->getText();

            if (inputDeviceDropDown != nullptr)
                config.inputDeviceName = inputDeviceDropDown->getSelectedId() < 0 ? String::empty
                                                                                  : inputDeviceDropDown->getText();

            if (! type.hasSeparateInputsAndOutputs())
                config.inputDeviceName = config.outputDeviceName;

            if (comboBoxThatHasChanged == inputDeviceDropDown)
                config.useDefaultInputChannels = true;
            else
                config.useDefaultOutputChannels = true;

            error = setup.manager->setAudioDeviceSetup (config, true);

            showCorrectDeviceName (inputDeviceDropDown, true);
            showCorrectDeviceName (outputDeviceDropDown, false);

            resized();
        }
        else if (comboBoxThatHasChanged == sampleRateDropDown)
        {
            if (sampleRateDropDown->getSelectedId() > 0)
            {
                config.sampleRate = sampleRateDropDown->getSelectedId();
                error = setup.manager->setAudioDeviceSetup (config, true);
            }
        }
        else if (comboBoxThatHasChanged == bufferSizeDropDown)
        {
            if (bufferSizeDropDown->getSelectedId() > 0)
            {
                config.bufferSize = bufferSizeDropDown->getSelectedId();
                error = setup.manager->setAudioDeviceSetup (config, true);
            }
        }

        if (error.isNotEmpty())
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                              TRANS("Error when trying to open audio device!"),
                                              error);
    }

    void audioDeviceAboutToChange()
    {
        if (audioDeviceAboutToChangeCallback != nullptr)
            audioDeviceAboutToChangeCallback();
    }

    bool showDeviceControlPanel()
    {
        if (AudioIODevice* const device = setup.manager->getCurrentAudioDevice())
        {
            Component modalWindow (String::empty);
            modalWindow.setOpaque (true);
            modalWindow.addToDesktop (0);
            modalWindow.enterModalState();

            return device->showControlPanel();
        }

        return false;
    }

    void updateAllControls()
    {
        updateOutputsComboBox();
        updateInputsComboBox();

        if (AudioIODevice* const currentDevice = setup.manager->getCurrentAudioDevice())
        {
            updateSampleRateComboBox (currentDevice);
            updateBufferSizeComboBox (currentDevice);
        }
        else
        {
            jassert (setup.manager->getCurrentAudioDevice() == nullptr); // not the correct device type!

            sampleRateLabel = nullptr;
            bufferSizeLabel = nullptr;
            sampleRateDropDown = nullptr;
            bufferSizeDropDown = nullptr;

            if (outputDeviceDropDown != nullptr)
                outputDeviceDropDown->setSelectedId (-1, dontSendNotification);

            if (inputDeviceDropDown != nullptr)
                inputDeviceDropDown->setSelectedId (-1, dontSendNotification);
        }

        sendLookAndFeelChange();
        resized();
        setSize (getWidth(), getLowestY() + 4);
    }

    void changeListenerCallback (ChangeBroadcaster*) override
    {
        updateAllControls();
    }

    void resetDevice()
    {
        setup.manager->closeAudioDevice();
        setup.manager->restartLastAudioDevice();
    }

private:
    std::function<void()> audioDeviceAboutToChangeCallback;
    AudioIODeviceType& type;
    const CustomAudioDeviceSetupDetails setup;

    ScopedPointer<ComboBox> outputDeviceDropDown, inputDeviceDropDown, sampleRateDropDown, bufferSizeDropDown;
    ScopedPointer<Label> outputDeviceLabel, inputDeviceLabel, sampleRateLabel, bufferSizeLabel;

    void showCorrectDeviceName (ComboBox* const box, const bool isInput)
    {
        if (box != nullptr)
        {
            AudioIODevice* const currentDevice = setup.manager->getCurrentAudioDevice();

            const int index = type.getIndexOfDevice (currentDevice, isInput);

            box->setSelectedId (index + 1, dontSendNotification);
        }
    }

    void addNamesToDeviceBox (ComboBox& combo, bool isInputs)
    {
        const StringArray devs (type.getDeviceNames (isInputs));

        combo.clear (dontSendNotification);

        for (int i = 0; i < devs.size(); ++i)
            combo.addItem (devs[i], i + 1);

        combo.addItem (getNoDeviceString(), -1);
        combo.setSelectedId (-1, dontSendNotification);
    }

    int getLowestY() const
    {
        int y = 0;

        for (int i = getNumChildComponents(); --i >= 0;)
            y = jmax (y, getChildComponent (i)->getBottom());

        return y;
    }

    void updateOutputsComboBox()
    {
        if (setup.maxNumOutputChannels > 0 || ! type.hasSeparateInputsAndOutputs())
        {
            if (outputDeviceDropDown == nullptr)
            {
                outputDeviceDropDown = new ComboBox (String::empty);
                outputDeviceDropDown->addListener (this);
                addAndMakeVisible (outputDeviceDropDown);

                outputDeviceLabel = new Label (String::empty,
                                               type.hasSeparateInputsAndOutputs() ? TRANS("Output:")
                                                                                  : TRANS("Device:"));
                outputDeviceLabel->attachToComponent (outputDeviceDropDown, true);
            }

            addNamesToDeviceBox (*outputDeviceDropDown, false);
        }

        showCorrectDeviceName (outputDeviceDropDown, false);
    }

    void updateInputsComboBox()
    {
        if (setup.maxNumInputChannels > 0 && type.hasSeparateInputsAndOutputs())
        {
            if (inputDeviceDropDown == nullptr)
            {
                inputDeviceDropDown = new ComboBox (String::empty);
                inputDeviceDropDown->addListener (this);
                addAndMakeVisible (inputDeviceDropDown);

                inputDeviceLabel = new Label (String::empty, TRANS("Input:"));
                inputDeviceLabel->attachToComponent (inputDeviceDropDown, true);
            }

            addNamesToDeviceBox (*inputDeviceDropDown, true);
        }

        showCorrectDeviceName (inputDeviceDropDown, true);
    }

    void updateSampleRateComboBox (AudioIODevice* currentDevice)
    {
        if (sampleRateDropDown == nullptr)
        {
            addAndMakeVisible (sampleRateDropDown = new ComboBox (String::empty));

            sampleRateLabel = new Label (String::empty, TRANS("Sample rate:"));
            sampleRateLabel->attachToComponent (sampleRateDropDown, true);
        }
        else
        {
            sampleRateDropDown->clear();
            sampleRateDropDown->removeListener (this);
        }

        const Array<double> rates (currentDevice->getAvailableSampleRates());

        for (int i = 0; i < rates.size(); ++i)
        {
            const int rate = roundToInt (rates[i]);
            sampleRateDropDown->addItem (String (rate) + " Hz", rate);
        }

        sampleRateDropDown->setSelectedId (roundToInt (currentDevice->getCurrentSampleRate()), dontSendNotification);
        sampleRateDropDown->addListener (this);
    }

    void updateBufferSizeComboBox (AudioIODevice* currentDevice)
    {
        if (bufferSizeDropDown == nullptr)
        {
            addAndMakeVisible (bufferSizeDropDown = new ComboBox (String::empty));

            bufferSizeLabel = new Label (String::empty, TRANS("Audio buffer size:"));
            bufferSizeLabel->attachToComponent (bufferSizeDropDown, true);
        }
        else
        {
            bufferSizeDropDown->clear();
            bufferSizeDropDown->removeListener (this);
        }

        const Array<int> bufferSizes (currentDevice->getAvailableBufferSizes());

        double currentRate = currentDevice->getCurrentSampleRate();
        if (currentRate == 0)
            currentRate = 48000.0;

        for (int i = 0; i < bufferSizes.size(); ++i)
        {
            const int bs = bufferSizes[i];
            bufferSizeDropDown->addItem (String (bs) + " samples (" + String (bs * 1000.0 / currentRate, 1) + " ms)", bs);
        }

        bufferSizeDropDown->setSelectedId (currentDevice->getCurrentBufferSizeSamples(), dontSendNotification);
        bufferSizeDropDown->addListener (this);
    }
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDeviceSettingsPanel)
};

//==============================================================================
CustomAudioDeviceSelectorComponent::CustomAudioDeviceSelectorComponent (AudioDeviceManager& dm,
                                                                        const int minInputChannels_,
                                                                        const int maxInputChannels_,
                                                                        const int minOutputChannels_,
                                                                        const int maxOutputChannels_,
                                                                        const bool showMidiInputOptions,
                                                                        const bool showMidiOutputSelector,
                                                                        const bool showChannelsAsStereoPairs_,
                                                                        std::function<void()> audioDeviceAboutToChangeCB)
:   deviceManager                    (dm),
    audioDeviceAboutToChangeCallback (audioDeviceAboutToChangeCB),
    itemHeight                       (24),
    minOutputChannels                (minOutputChannels_),
    maxOutputChannels                (maxOutputChannels_),
    minInputChannels                 (minInputChannels_),
    maxInputChannels                 (maxInputChannels_),
    showChannelsAsStereoPairs        (showChannelsAsStereoPairs_)
{
    jassert (minOutputChannels >= 0 && minOutputChannels <= maxOutputChannels);
    jassert (minInputChannels >= 0 && minInputChannels <= maxInputChannels);

    const OwnedArray<AudioIODeviceType>& types = deviceManager.getAvailableDeviceTypes();

    if (types.size() > 1)
    {
        deviceTypeDropDown = new ComboBox (String::empty);

        for (int i = 0; i < types.size(); ++i)
            deviceTypeDropDown->addItem (types.getUnchecked(i)->getTypeName(), i + 1);

        addAndMakeVisible (deviceTypeDropDown);
        deviceTypeDropDown->addListener (this);

        deviceTypeDropDownLabel = new Label (String::empty, TRANS("Audio device type:"));
        deviceTypeDropDownLabel->setJustificationType (Justification::centredRight);
        deviceTypeDropDownLabel->attachToComponent (deviceTypeDropDown, true);
    }

    deviceManager.addChangeListener (this);
    updateAllControls();
    startTimer (1000);
}

CustomAudioDeviceSelectorComponent::~CustomAudioDeviceSelectorComponent()
{
    deviceManager.removeChangeListener (this);
}

void CustomAudioDeviceSelectorComponent::setItemHeight (int newItemHeight)
{
    itemHeight = newItemHeight;
    resized();
}

void CustomAudioDeviceSelectorComponent::resized()
{
    Rectangle<int> r        = getLocalBounds().reduced (FeatureExtractorLookAndFeel::getComponentInset());
    const int comboBoxWidth = (int) (r.getWidth() * FeatureExtractorLookAndFeel::getDeviceSettingsComboBoxWidthRatio());
    const int space         = FeatureExtractorLookAndFeel::getInnerComponentSpacing();

    if (deviceTypeDropDown != nullptr)
    {
        deviceTypeDropDown->setBounds (r.removeFromTop (itemHeight).removeFromRight (comboBoxWidth));
        r.removeFromTop (space);
    }

    if (audioDeviceSettingsComp != nullptr)
    {
        audioDeviceSettingsComp->setBounds (r);
    }
}

void CustomAudioDeviceSelectorComponent::timerCallback()
{
    // TODO
    // unfortunately, the AudioDeviceManager only gives us changeListenerCallbacks
    // if an audio device has changed, but not if a MIDI device has changed.
    // This needs to be implemented properly. Until then, we use a workaround
    // where we update the whole component once per second on a timer callback.
    updateAllControls();
}

void CustomAudioDeviceSelectorComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == deviceTypeDropDown)
    {
        if (audioDeviceAboutToChangeCallback != nullptr)
            audioDeviceAboutToChangeCallback();

        if (AudioIODeviceType* const type = deviceManager.getAvailableDeviceTypes() [deviceTypeDropDown->getSelectedId() - 1])
        {
            audioDeviceSettingsComp = nullptr;
            deviceManager.setCurrentAudioDeviceType (type->getTypeName(), true);
            updateAllControls(); // needed in case the type hasn't actually changed
        }
    }
}

void CustomAudioDeviceSelectorComponent::changeListenerCallback (ChangeBroadcaster*)
{
    updateAllControls();
}

void CustomAudioDeviceSelectorComponent::updateAllControls()
{
    if (deviceTypeDropDown != nullptr)
        deviceTypeDropDown->setText (deviceManager.getCurrentAudioDeviceType(), dontSendNotification);

    if (audioDeviceSettingsComp == nullptr
         || audioDeviceSettingsCompType != deviceManager.getCurrentAudioDeviceType())
    {
        audioDeviceSettingsCompType = deviceManager.getCurrentAudioDeviceType();
        audioDeviceSettingsComp = nullptr;

        if (AudioIODeviceType* const type
                = deviceManager.getAvailableDeviceTypes() [deviceTypeDropDown == nullptr
                                                            ? 0 : deviceTypeDropDown->getSelectedId() - 1])
        {
            CustomAudioDeviceSetupDetails details;
            details.manager = &deviceManager;
            details.minNumInputChannels = minInputChannels;
            details.maxNumInputChannels = maxInputChannels;
            details.minNumOutputChannels = minOutputChannels;
            details.maxNumOutputChannels = maxOutputChannels;
            details.useStereoPairs = showChannelsAsStereoPairs;

            AudioDeviceSettingsPanel* sp = new AudioDeviceSettingsPanel (*type, details, audioDeviceAboutToChangeCallback);
            audioDeviceSettingsComp = sp;
            addAndMakeVisible (sp);
            sp->updateAllControls();
        }
    }

    resized();
}

const CustomAudioDeviceSetupDetails CustomAudioDeviceSelectorComponent::getDeviceSetupDetails()
{
    CustomAudioDeviceSetupDetails details;
    details.manager = &deviceManager;
    details.minNumInputChannels = minInputChannels;
    details.maxNumInputChannels = maxInputChannels;
    details.minNumOutputChannels = minOutputChannels;
    details.maxNumOutputChannels = maxOutputChannels;
    details.useStereoPairs = showChannelsAsStereoPairs;
    return details;
}

