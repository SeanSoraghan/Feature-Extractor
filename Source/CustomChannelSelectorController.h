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
            if (rowIsSelected)
                g.fillAll (findColour (TextEditor::highlightColourId)
                                .withMultipliedAlpha (0.3f));

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

    int getBestHeight (int maxHeight)
    {
        return getRowHeight() * jlimit (2, jmax (2, maxHeight / getRowHeight()),
                                        getNumRows())
                    + getOutlineThickness() * 2;
    }

private:
    //==============================================================================
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

    int getTickX() const
    {
        return getRowHeight() + 5;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomChannelSelectorListBox)
};



#endif  // CUSTOMCHANNELSELECTOR_H_INCLUDED
