/*
  ==============================================================================

    AudioSourceSelectorComboBox.h
    Created: 12 Feb 2016 3:43:52pm
    Author:  Sean

  ==============================================================================
*/

#ifndef AUDIOSOURCESELECTORCOMBOBOX_H_INCLUDED
#define AUDIOSOURCESELECTORCOMBOBOX_H_INCLUDED

enum eAudioSourceType
{
    enIncomingAudio = 0,
    enAudioFile,
    enNumSourceTypes
};

static String getAudioSourceTypeString (eAudioSourceType type)
{
    switch (type)
    {
    case enIncomingAudio:
        return String ("Analyse incoming audio");
    case enAudioFile:
        return String ("Analyse audio file");
    default:
        jassertfalse;
        return ("UNKNOWN");
    }
}

enum eChannelType
{
    enLeft = 0,
    enRight,
    enNumChannels
};

static String getChannelTypeString (eChannelType type)
{
    switch (type)
    {
    case enLeft:
        return String ("Analyse left channel");
    case enRight:
        return String ("Analyse right channel");
    default:
        jassertfalse;
        return ("UNKNOWN");
    }
}

template <class EnumType = eAudioSourceType>
class AudioSourceSelectorController : ComboBox::Listener
{
public:
    static int getComboBoxIDForAudioSourceType (EnumType type)
    {
        return (int) type + 1;
    }

    static EnumType getAudioSourceTypeForComboBoxID (int iD)
    {
        return (EnumType) (iD - 1);
    }

    AudioSourceSelectorController (std::function<String (EnumType)> getSourceTypeFunction)
    :   getSourceTypeString (getSourceTypeFunction)
    {
        fillComboBoxSelections();
        selectorComboBox.setSelectedId (1);
        selectorComboBox.addListener (this);
    }

    ComboBox& getSelector()
    {
        return selectorComboBox;
    }

    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override
    {
        if (comboBoxThatHasChanged == &selectorComboBox)
        {
            EnumType sourceType = getAudioSourceTypeForComboBoxID (comboBoxThatHasChanged->getSelectedId());

            if (audioSourceTypeChangedCallback != nullptr)
                audioSourceTypeChangedCallback (sourceType);
        }
        else
        {
            jassertfalse;
        }
    }

    void setAudioSourceTypeChangedCallback (std::function<void (EnumType)> f) { audioSourceTypeChangedCallback = f; }

private:
    ComboBox selectorComboBox;
    std::function<void   (EnumType)> audioSourceTypeChangedCallback;
    std::function<String (EnumType)> getSourceTypeString;

    void fillComboBoxSelections()
    {
        for (int type = 0; type < (int) enNumSourceTypes; type++)
        {
            EnumType sourceType = (EnumType) type;

            if (getSourceTypeString != nullptr)
            {
                String sourceTypeText = getSourceTypeString (sourceType);
                selectorComboBox.addItem (sourceTypeText, getComboBoxIDForAudioSourceType (sourceType));
            }
        }
    }
};



#endif  // AUDIOSOURCESELECTORCOMBOBOX_H_INCLUDED
