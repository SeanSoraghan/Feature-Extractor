/*
  ==============================================================================

    OSCSettings.h
    Created: 11 Feb 2016 10:44:43pm
    Author:  Sean

  ==============================================================================
*/

#ifndef OSCSETTINGS_H_INCLUDED
#define OSCSETTINGS_H_INCLUDED

class OSCSettingsView : public Component
{
public:
    OSCSettingsView()
    :   label ("address", "Send to OSC Address:")
    {
        label.setJustificationType (Justification::centred);
        addressEditor.setInputFilter (new TextEditor::LengthAndCharacterRestriction (30, "1234567890.:"), true);
        addAndMakeVisible (label);
        addAndMakeVisible (addressEditor);
    }

    void resized() override
    {
        auto centralBounds = getLocalBounds().withSizeKeepingCentre (FeatureExtractorLookAndFeel::getOSCSettingsWidth(),
                                                                     FeatureExtractorLookAndFeel::getOSCSettingsHeight());
        int availableHeight = centralBounds.getHeight() - FeatureExtractorLookAndFeel::getVerticalMargin();
        label.setBounds (centralBounds.removeFromTop (availableHeight / 2));
        centralBounds.removeFromTop (availableHeight / 2);
        addressEditor.setBounds (centralBounds.removeFromTop (availableHeight / 2));
    }

    TextEditor& getAddressEditor()
    {
        return addressEditor;
    }

private:
    Label      label;
    TextEditor addressEditor;
};

//==============================================================================
//==============================================================================

class OSCSettingsController : TextEditor::Listener
{
public:
    OSCSettingsController()
    {
        view.getAddressEditor().addListener (this);
    }

    void textEditorReturnKeyPressed (TextEditor& editor) 
    {
        if (addressChangedCallback)
            if ( ! addressChangedCallback (editor.getText()))
                editor.setText (String::empty);
    }

    void setAddressChangedCallback (std::function<bool (String address)> function) { addressChangedCallback = function; }

    OSCSettingsView& getView()
    {
        return view;
    }

private:
    std::function<bool (String address)> addressChangedCallback;
    OSCSettingsView view;
};

#endif  // OSCSETTINGS_H_INCLUDED
