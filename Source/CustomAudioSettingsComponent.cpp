/*
  ==============================================================================

    CustomAudioSettingsComponent.cpp
    Created: 16 Jun 2016 11:34:21am
    Author:  Sean

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
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
class CustomAudioDeviceSelectorComponent::MidiInputSelectorComponentListBox  : public ListBox,
                                                                               private ListBoxModel
{
public:
    MidiInputSelectorComponentListBox (AudioDeviceManager& dm, const String& noItems)
        : ListBox (String::empty, nullptr),
          deviceManager (dm),
          noItemsMessage (noItems)
    {
        updateDevices();
        setModel (this);
        setOutlineThickness (1);
    }

    void updateDevices()
    {
        items = MidiInput::getDevices();
    }

    int getNumRows() override
    {
        return items.size();
    }

    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (isPositiveAndBelow (row, items.size()))
        {
            if (rowIsSelected)
                g.fillAll (findColour (TextEditor::highlightColourId)
                               .withMultipliedAlpha (0.3f));

            const String item (items [row]);
            bool enabled = deviceManager.isMidiInputEnabled (item);

            const int x = getTickX();
            const float tickW = height * 0.75f;

            getLookAndFeel().drawTickBox (g, *this, x - tickW, (height - tickW) / 2, tickW, tickW,
                                          enabled, true, true, false);

            g.setFont (height * 0.6f);
            g.setColour (findColour (ListBox::textColourId, true).withMultipliedAlpha (enabled ? 1.0f : 0.6f));
            g.drawText (item, x, 0, width - x - 2, height, Justification::centredLeft, true);
        }
    }

    void listBoxItemClicked (int row, const MouseEvent& e) override
    {
        selectRow (row);

        if (e.x < getTickX())
            flipEnablement (row);
    }

    void listBoxItemDoubleClicked (int row, const MouseEvent&) override
    {
        flipEnablement (row);
    }

    void returnKeyPressed (int row) override
    {
        flipEnablement (row);
    }

    void paint (Graphics& g) override
    {
        ListBox::paint (g);

        if (items.size() == 0)
        {
            g.setColour (Colours::grey);
            g.setFont (13.0f);
            g.drawText (noItemsMessage,
                        0, 0, getWidth(), getHeight() / 2,
                        Justification::centred, true);
        }
    }

    int getBestHeight (const int preferredHeight)
    {
        const int extra = getOutlineThickness() * 2;

        return jmax (getRowHeight() * 2 + extra,
                     jmin (getRowHeight() * getNumRows() + extra,
                           preferredHeight));
    }

private:
    //==============================================================================
    AudioDeviceManager& deviceManager;
    const String noItemsMessage;
    StringArray items;

    void flipEnablement (const int row)
    {
        if (isPositiveAndBelow (row, items.size()))
        {
            const String item (items [row]);
            deviceManager.setMidiInputEnabled (item, ! deviceManager.isMidiInputEnabled (item));
        }
    }

    int getTickX() const
    {
        return getRowHeight() + 5;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiInputSelectorComponentListBox)
};


//==============================================================================

static String getNoDeviceString()   { return "<< " + TRANS("none") + " >>"; }

class AudioDeviceSettingsPanel : public Component,
                                 private ChangeListener,
                                 private ComboBoxListener,  // (can't use ComboBox::Listener due to idiotic VC2005 bug)
                                 private ButtonListener
{
public:
    AudioDeviceSettingsPanel (AudioIODeviceType& t, CustomAudioDeviceSetupDetails& setupDetails,
                              const bool hideAdvancedOptionsWithButton, std::function<void()> audioDeviceAboutToChangeCB)
        :   type (t), 
            setup (setupDetails), 
            audioDeviceAboutToChangeCallback (audioDeviceAboutToChangeCB)
    {
        if (hideAdvancedOptionsWithButton)
        {
            addAndMakeVisible (showAdvancedSettingsButton = new TextButton (TRANS("Show advanced settings...")));
            showAdvancedSettingsButton->addListener (this);
        }

        type.scanForDevices();

        setup.manager->addChangeListener (this);
    }

    ~AudioDeviceSettingsPanel()
    {
        setup.manager->removeChangeListener (this);
    }

    void resized() override
    {
        if (CustomAudioDeviceSelectorComponent* parent = findParentComponentOfClass<CustomAudioDeviceSelectorComponent>())
        {
            Rectangle<int> r (proportionOfWidth (0.35f), 0, proportionOfWidth (0.6f), 3000);

            const int maxListBoxHeight = 100;
            const int h = parent->getItemHeight();
            const int space = h / 4;

            if (outputDeviceDropDown != nullptr)
            {
                Rectangle<int> row (r.removeFromTop (h));

                if (testButton != nullptr)
                {
                    testButton->changeWidthToFitText (h);
                    testButton->setBounds (row.removeFromRight (testButton->getWidth()));
                    row.removeFromRight (space);
                }

                outputDeviceDropDown->setBounds (row);
                r.removeFromTop (space);
            }

            if (inputDeviceDropDown != nullptr)
            {
                Rectangle<int> row (r.removeFromTop (h));

                inputLevelMeter->setBounds (row.removeFromRight (testButton != nullptr ? testButton->getWidth() : row.getWidth() / 6));
                row.removeFromRight (space);
                inputDeviceDropDown->setBounds (row);
                r.removeFromTop (space);
            }


            r.removeFromTop (space * 2);

            if (showAdvancedSettingsButton != nullptr)
            {
                showAdvancedSettingsButton->setBounds (r.withHeight (h));
                showAdvancedSettingsButton->changeWidthToFitText();
            }

            const bool advancedSettingsVisible = showAdvancedSettingsButton == nullptr
                                                    || ! showAdvancedSettingsButton->isVisible();

            if (sampleRateDropDown != nullptr)
            {
                sampleRateDropDown->setVisible (advancedSettingsVisible);
                sampleRateDropDown->setBounds (r.removeFromTop (h));
                r.removeFromTop (space);
            }

            if (bufferSizeDropDown != nullptr)
            {
                bufferSizeDropDown->setVisible (advancedSettingsVisible);
                bufferSizeDropDown->setBounds (r.removeFromTop (h));
                r.removeFromTop (space);
            }

            r.removeFromTop (space);

            if (showUIButton != nullptr || resetDeviceButton != nullptr)
            {
                Rectangle<int> buttons (r.removeFromTop (h));

                if (showUIButton != nullptr)
                {
                    showUIButton->setVisible (advancedSettingsVisible);
                    showUIButton->changeWidthToFitText (h);
                    showUIButton->setBounds (buttons.removeFromLeft (showUIButton->getWidth()));
                    buttons.removeFromLeft (space);
                }

                if (resetDeviceButton != nullptr)
                {
                    resetDeviceButton->setVisible (advancedSettingsVisible);
                    resetDeviceButton->changeWidthToFitText (h);
                    resetDeviceButton->setBounds (buttons.removeFromLeft (resetDeviceButton->getWidth()));
                }

                r.removeFromTop (space);
            }

            setSize (getWidth(), r.getY());
        }
        else
        {
            jassertfalse;
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

            updateControlPanelButton();
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

    void buttonClicked (Button* button) override
    {
        if (button == showAdvancedSettingsButton)
        {
            showAdvancedSettingsButton->setVisible (false);
            resized();
        }
        else if (button == showUIButton)
        {
            if (showDeviceControlPanel())
            {
                setup.manager->closeAudioDevice();
                setup.manager->restartLastAudioDevice();
                getTopLevelComponent()->toFront (true);
            }
        }
        else if (button == testButton && testButton != nullptr)
        {
            setup.manager->playTestSound();
        }
        else if (button == resetDeviceButton)
        {
            resetDevice();
        }
    }

    void updateAllControls()
    {
        updateOutputsComboBox();
        updateInputsComboBox();

        updateControlPanelButton();
        updateResetButton();

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
    ScopedPointer<TextButton> testButton;
    ScopedPointer<Component> inputLevelMeter;
    ScopedPointer<TextButton> showUIButton, showAdvancedSettingsButton, resetDeviceButton;

    void showCorrectDeviceName (ComboBox* const box, const bool isInput)
    {
        if (box != nullptr)
        {
            AudioIODevice* const currentDevice = setup.manager->getCurrentAudioDevice();

            const int index = type.getIndexOfDevice (currentDevice, isInput);

            box->setSelectedId (index + 1, dontSendNotification);

            if (testButton != nullptr && ! isInput)
                testButton->setEnabled (index >= 0);
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

    void updateControlPanelButton()
    {
        AudioIODevice* const currentDevice = setup.manager->getCurrentAudioDevice();
        showUIButton = nullptr;

        if (currentDevice != nullptr && currentDevice->hasControlPanel())
        {
            addAndMakeVisible (showUIButton = new TextButton (TRANS ("Control panel"),
                                                              TRANS ("Opens the device's own control panel")));
            showUIButton->addListener (this);
        }

        resized();
    }

    void updateResetButton()
    {
        if (AudioIODevice* const currentDevice = setup.manager->getCurrentAudioDevice())
        {
            if (currentDevice->hasControlPanel())
            {
                if (resetDeviceButton == nullptr)
                {
                    addAndMakeVisible (resetDeviceButton = new TextButton (TRANS ("Reset device"),
                        TRANS ("Resets the audio interface - sometimes needed after changing a device's properties in its custom control panel")));

                    resetDeviceButton->addListener (this);
                    resized();
                }

                return;
            }
        }

        resetDeviceButton = nullptr;
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

                if (setup.maxNumOutputChannels > 0)
                {
                    addAndMakeVisible (testButton = new TextButton (TRANS("Test"),
                                                                    TRANS("Plays a test tone")));
                    testButton->addListener (this);
                }
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

                addAndMakeVisible (inputLevelMeter
                    = new CustomSimpleDeviceManagerInputLevelMeter (*setup.manager));
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
                                                                        const bool hideAdvancedOptionsWithButton_,
                                                                        std::function<void()> audioDeviceAboutToChangeCB)
:   deviceManager                    (dm),
    audioDeviceAboutToChangeCallback (audioDeviceAboutToChangeCB),
    itemHeight                       (24),
    minOutputChannels                (minOutputChannels_),
    maxOutputChannels                (maxOutputChannels_),
    minInputChannels                 (minInputChannels_),
    maxInputChannels                 (maxInputChannels_),
    showChannelsAsStereoPairs        (showChannelsAsStereoPairs_),
    hideAdvancedOptionsWithButton    (hideAdvancedOptionsWithButton_)
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

    if (showMidiInputOptions)
    {
        addAndMakeVisible (midiInputsList
                            = new MidiInputSelectorComponentListBox (deviceManager,
                                                                     "(" + TRANS("No MIDI inputs available") + ")"));

        midiInputsLabel = new Label (String::empty, TRANS ("Active MIDI inputs:"));
        midiInputsLabel->setJustificationType (Justification::topRight);
        midiInputsLabel->attachToComponent (midiInputsList, true);

        if (BluetoothMidiDevicePairingDialogue::isAvailable())
        {
            addAndMakeVisible (bluetoothButton = new TextButton (TRANS("Bluetooth MIDI"),
                                                                 TRANS("Scan for bluetooth MIDI devices")));
            bluetoothButton->addListener (this);
        }
    }
    else
    {
        midiInputsList = nullptr;
        midiInputsLabel = nullptr;
        bluetoothButton = nullptr;
    }

    if (showMidiOutputSelector)
    {
        addAndMakeVisible (midiOutputSelector = new ComboBox (String::empty));
        midiOutputSelector->addListener (this);

        midiOutputLabel = new Label ("lm", TRANS("MIDI Output:"));
        midiOutputLabel->attachToComponent (midiOutputSelector, true);
    }
    else
    {
        midiOutputSelector = nullptr;
        midiOutputLabel = nullptr;
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
    Rectangle<int> r (proportionOfWidth (0.35f), 15, proportionOfWidth (0.6f), 3000);
    const int space = itemHeight / 4;

    if (deviceTypeDropDown != nullptr)
    {
        deviceTypeDropDown->setBounds (r.removeFromTop (itemHeight));
        r.removeFromTop (space * 3);
    }

    if (audioDeviceSettingsComp != nullptr)
    {
        audioDeviceSettingsComp->resized();
        audioDeviceSettingsComp->setBounds (r.removeFromTop (audioDeviceSettingsComp->getHeight())
                                                .withX (0).withWidth (getWidth()));
        r.removeFromTop (space);
    }

    if (midiInputsList != nullptr)
    {
        midiInputsList->setBounds (r.removeFromTop (midiInputsList->getBestHeight (jmin (itemHeight * 8,
                                                                                         getHeight() - r.getY() - space - itemHeight))));
        r.removeFromTop (space);
    }

    if (bluetoothButton != nullptr)
    {
        bluetoothButton->setBounds (r.removeFromTop (24));
        r.removeFromTop (space);
    }

    if (midiOutputSelector != nullptr)
        midiOutputSelector->setBounds (r.removeFromTop (itemHeight));
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
    else if (comboBoxThatHasChanged == midiOutputSelector)
    {
        String midiDeviceName (midiOutputSelector->getText());

        if (midiDeviceName == getNoDeviceString())
            midiDeviceName = String();

        deviceManager.setDefaultMidiOutput (midiDeviceName);
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

            AudioDeviceSettingsPanel* sp = new AudioDeviceSettingsPanel (*type, details, hideAdvancedOptionsWithButton, audioDeviceAboutToChangeCallback);
            audioDeviceSettingsComp = sp;
            addAndMakeVisible (sp);
            sp->updateAllControls();
        }
    }

    if (midiInputsList != nullptr)
    {
        midiInputsList->updateDevices();
        midiInputsList->updateContent();
        midiInputsList->repaint();
    }

    if (midiOutputSelector != nullptr)
    {
        midiOutputSelector->clear();

        const StringArray midiOuts (MidiOutput::getDevices());

        midiOutputSelector->addItem (getNoDeviceString(), -1);
        midiOutputSelector->addSeparator();

        for (int i = 0; i < midiOuts.size(); ++i)
            midiOutputSelector->addItem (midiOuts[i], i + 1);

        int current = -1;

        if (deviceManager.getDefaultMidiOutput() != nullptr)
            current = 1 + midiOuts.indexOf (deviceManager.getDefaultMidiOutputName());

        midiOutputSelector->setSelectedId (current, dontSendNotification);
    }

    resized();
}

CustomAudioDeviceSetupDetails CustomAudioDeviceSelectorComponent::getDeviceSetupDetails()
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

void CustomAudioDeviceSelectorComponent::buttonClicked (Button* btn)
{
    if (bluetoothButton == btn)
        BluetoothMidiDevicePairingDialogue::open();
}

ListBox* CustomAudioDeviceSelectorComponent::getMidiInputSelectorListBox() const noexcept
{
    return midiInputsList;
}

