/*
  ==============================================================================

    AudioSourceSelectorComboBox.h
    Created: 12 Feb 2016 3:43:52pm
    Author:  Sean

  ==============================================================================
*/

#ifndef AUDIOSOURCESELECTORCOMBOBOX_H_INCLUDED
#define AUDIOSOURCESELECTORCOMBOBOX_H_INCLUDED

class AudioSourceSelectorController : ComboBox::Listener
{
public:
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

    static int getComboBoxIDForAudioSourceType (eAudioSourceType type)
    {
        return (int) type + 1;
    }

    static eAudioSourceType getAudioSourceTypeForComboBoxID (int iD)
    {
        return (eAudioSourceType) (iD - 1);
    }

    AudioSourceSelectorController()
    {
        fillComboBoxSelections();
        selectorComboBox.setSelectedId (getComboBoxIDForAudioSourceType (enIncomingAudio));
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
            eAudioSourceType sourceType = getAudioSourceTypeForComboBoxID (comboBoxThatHasChanged->getSelectedId());

            if (audioSourceTypeChangedCallback != nullptr)
                audioSourceTypeChangedCallback (sourceType);
        }
        else
        {
            jassertfalse;
        }
    }

    void setAudioSourceTypeChangedCallback (std::function<void (eAudioSourceType)> f) { audioSourceTypeChangedCallback = f; }

private:
    ComboBox selectorComboBox;
    std::function<void (eAudioSourceType)> audioSourceTypeChangedCallback;

    void fillComboBoxSelections()
    {
        for (int type = 0; type < (int) enNumSourceTypes; type++)
        {
            eAudioSourceType sourceType = (eAudioSourceType) type;
            String sourceTypeText = getAudioSourceTypeString (sourceType);
            selectorComboBox.addItem (sourceTypeText, getComboBoxIDForAudioSourceType (sourceType));
        }
    }
};



#endif  // AUDIOSOURCESELECTORCOMBOBOX_H_INCLUDED
