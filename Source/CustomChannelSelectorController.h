/*
  ==============================================================================

    CustomChannelSelector.h
    Created: 16 Jun 2016 12:29:14pm
    Author:  Sean

  ==============================================================================
*/

#ifndef CUSTOMCHANNELSELECTOR_H_INCLUDED
#define CUSTOMCHANNELSELECTOR_H_INCLUDED

struct CustomAudioDeviceSetupDetails
{
    AudioDeviceManager* manager;
    int minNumInputChannels, maxNumInputChannels;
    int minNumOutputChannels, maxNumOutputChannels;
    bool useStereoPairs;
};

class CustomChannelSelectorListBox  : public  ListBox,
                                      private ListBoxModel
{
public:
    enum CustomBoxType
    {
        audioInputType,
        audioOutputType
    };

    //==============================================================================
    CustomChannelSelectorListBox (const CustomAudioDeviceSetupDetails& setupDetails,
                                  const CustomBoxType boxType, const String& noItemsText)
        : ListBox (String::empty, nullptr),
          setup (setupDetails), type (boxType), 
          noItemsMessage (noItemsText)
    {
        setLookAndFeel (SharedResourcePointer<FeatureExtractorLookAndFeel>());
        setColour (ListBox::ColourIds::backgroundColourId, FeatureExtractorLookAndFeel::getForegroundInteractiveColour());
        refresh();
        setModel (this);
        setOutlineThickness (1);
    }

    void refresh()
    {
        items.clear();

        if (AudioIODevice* const currentDevice = setup.manager->getCurrentAudioDevice())
        {
            if (type == audioInputType)
                items = currentDevice->getInputChannelNames();
            else if (type == audioOutputType)
                items = currentDevice->getOutputChannelNames();

            if (setup.useStereoPairs)
            {
                StringArray pairs;

                for (int i = 0; i < items.size(); i += 2)
                {
                    const String& name = items[i];

                    if (i + 1 >= items.size())
                        pairs.add (name.trim());
                    else
                        pairs.add (getNameForChannelPair (name, items[i + 1]));
                }

                items = pairs;
            }
        }

        updateContent();
        repaint();
    }

    int getNumRows() override
    {
        return items.size();
    }

    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (isPositiveAndBelow (row, items.size()))
        {
            const String item (items [row]);
            bool enabled = false;

            AudioDeviceManager::AudioDeviceSetup config;
            setup.manager->getAudioDeviceSetup (config);

            if (setup.useStereoPairs)
            {
                if (type == audioInputType)
                    enabled = config.inputChannels [row * 2] || config.inputChannels [row * 2 + 1];
                else if (type == audioOutputType)
                    enabled = config.outputChannels [row * 2] || config.outputChannels [row * 2 + 1];
            }
            else
            {
                if (type == audioInputType)
                    enabled = config.inputChannels [row];
                else if (type == audioOutputType)
                    enabled = config.outputChannels [row];
            }

            const int tickX = FeatureExtractorLookAndFeel::getInnerComponentSpacing();
            const float tickW = FeatureExtractorLookAndFeel::getInnerComponentSpacing() * 2;

            getLookAndFeel().drawTickBox (g, *this, tickX, height / 2 - tickW / 2, tickW, tickW,
                                            enabled, true, true, false);

            g.setFont (FeatureExtractorLookAndFeel::getFeatureVisualiserTextHeight());
            g.setColour (findColour (ListBox::textColourId, true).withMultipliedAlpha (enabled ? 1.0f : 0.6f));
            g.drawText (item, tickX + tickW + FeatureExtractorLookAndFeel::getInnerComponentSpacing(), 0, width - tickX - 2, height, Justification::centredLeft, true);
        }
    }

    void listBoxItemClicked (int row, const MouseEvent& e) override
    {
        channelConfigAboutToChange();
        selectRow (row);

        flipEnablement (row);
    }

    void listBoxItemDoubleClicked (int row, const MouseEvent&) override
    {
        channelConfigAboutToChange();
        flipEnablement (row);
    }

    void returnKeyPressed (int row) override
    {
        channelConfigAboutToChange();
        flipEnablement (row);
    }

    void paint (Graphics& g) override
    {
        g.setColour (findColour (backgroundColourId));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), FeatureExtractorLookAndFeel::getCornerSize());
        if (items.size() == 0)
        {
            g.setColour (Colours::grey);
            g.setFont (13.0f);
            g.drawText (noItemsMessage,
                        0, 0, getWidth(), getHeight() / 2,
                        Justification::centred, true);
        }
    }

    int getBestHeight (int maxHeight)
    {
        return getRowHeight() * jlimit (2, jmax (2, maxHeight / getRowHeight()),
                                        getNumRows())
                    + getOutlineThickness() * 2;
    }

    void setChannelsConfigAboutToChangeCallback (std::function<void()> f) { channelsConfigurationAboutToChangeCallback = f; }
private:
    //==============================================================================
    std::function<void()> channelsConfigurationAboutToChangeCallback;
    const CustomAudioDeviceSetupDetails setup;
    const CustomBoxType type;
    const String noItemsMessage;
    StringArray items;

    static String getNameForChannelPair (const String& name1, const String& name2)
    {
        String commonBit;

        for (int j = 0; j < name1.length(); ++j)
            if (name1.substring (0, j).equalsIgnoreCase (name2.substring (0, j)))
                commonBit = name1.substring (0, j);

        // Make sure we only split the name at a space, because otherwise, things
        // like "input 11" + "input 12" would become "input 11 + 2"
        while (commonBit.isNotEmpty() && ! CharacterFunctions::isWhitespace (commonBit.getLastCharacter()))
            commonBit = commonBit.dropLastCharacters (1);

        return name1.trim() + " + " + name2.substring (commonBit.length()).trim();
    }

    void flipEnablement (const int row)
    {
        jassert (type == audioInputType || type == audioOutputType);

        if (isPositiveAndBelow (row, items.size()))
        {
            AudioDeviceManager::AudioDeviceSetup config;
            setup.manager->getAudioDeviceSetup (config);

            if (setup.useStereoPairs)
            {
                BigInteger bits;
                BigInteger& original = (type == audioInputType ? config.inputChannels
                                                                : config.outputChannels);

                for (int i = 0; i < 256; i += 2)
                    bits.setBit (i / 2, original [i] || original [i + 1]);

                if (type == audioInputType)
                {
                    config.useDefaultInputChannels = false;
                    flipBit (bits, row, setup.minNumInputChannels / 2, setup.maxNumInputChannels / 2);
                }
                else
                {
                    config.useDefaultOutputChannels = false;
                    flipBit (bits, row, setup.minNumOutputChannels / 2, setup.maxNumOutputChannels / 2);
                }

                for (int i = 0; i < 256; ++i)
                    original.setBit (i, bits [i / 2]);
            }
            else
            {
                if (type == audioInputType)
                {
                    config.useDefaultInputChannels = false;
                    flipBit (config.inputChannels, row, setup.minNumInputChannels, setup.maxNumInputChannels);
                }
                else
                {
                    config.useDefaultOutputChannels = false;
                    flipBit (config.outputChannels, row, setup.minNumOutputChannels, setup.maxNumOutputChannels);
                }
            }

            String error (setup.manager->setAudioDeviceSetup (config, true));

            if (error.isNotEmpty())
            {
                jassertfalse;
                //xxx
            }
        }
    }

    static void flipBit (BigInteger& chans, int index, int minNumber, int maxNumber)
    {
        const int numActive = chans.countNumberOfSetBits();

        if (chans [index])
        {
            if (numActive > minNumber)
                chans.setBit (index, false);
        }
        else
        {
            if (numActive >= maxNumber)
            {
                const int firstActiveChan = chans.findNextSetBit (0);

                chans.setBit (index > firstActiveChan
                                    ? firstActiveChan : chans.getHighestBit(),
                                false);
            }

            chans.setBit (index, true);
        }
    }

    void channelConfigAboutToChange()
    {
        if (channelsConfigurationAboutToChangeCallback != nullptr)
            channelsConfigurationAboutToChangeCallback();
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomChannelSelectorListBox)
};



#endif  // CUSTOMCHANNELSELECTOR_H_INCLUDED
