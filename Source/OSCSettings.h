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
    :   label            ("address",           "Send to OSC address:"),
        secondaryIPLabel ("secondary address", "Send to secondary OSC address:"),
        bundleLabel      ("Bundle Address",    "Use bundle address:")
    {
        addressEditor.setInputFilter          (new TextEditor::LengthAndCharacterRestriction (30, "1234567890.:"), true);
        secondaryAddressEditor.setInputFilter (new TextEditor::LengthAndCharacterRestriction (30, "1234567890.:"), true);

        addAndMakeVisible (label);
        addAndMakeVisible (addressEditor);
        addAndMakeVisible (secondaryIPLabel);
        addAndMakeVisible (secondaryAddressEditor);
        addAndMakeVisible (bundleLabel);
        addAndMakeVisible (bundleAddressEditor);
    }

    void resized() override
    {
        const int itemHeight = FeatureExtractorLookAndFeel::getOSCItemHeight();
        const int spacing    = FeatureExtractorLookAndFeel::getInnerComponentSpacing();
        auto b = getLocalBounds().reduced (FeatureExtractorLookAndFeel::getComponentInset());
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            getChildComponent (i)->setBounds (b.removeFromTop (itemHeight));
            b.removeFromTop (spacing);
        }
    }

    TextEditor& getAddressEditor()
    {
        return addressEditor;
    }

    TextEditor& getSecondaryAddressEditor()
    {
        return secondaryAddressEditor;
    }

    TextEditor& getBundleAddressEditor()
    {
        return bundleAddressEditor;
    }

    void setBundleAddress (String bundleAddress)
    {
        bundleAddressEditor.setText (bundleAddress, true);
    }
private:
    Label      label;
    Label      secondaryIPLabel;
    Label      bundleLabel;
    TextEditor addressEditor;
    TextEditor secondaryAddressEditor;
    TextEditor bundleAddressEditor;
};

//==============================================================================
//==============================================================================

class OSCSettingsController : TextEditor::Listener
{
public:
    OSCSettingsController()
    {
        view.getAddressEditor().addListener (this);
        view.getSecondaryAddressEditor().addListener (this);
        view.getBundleAddressEditor().addListener (this);
    }

    void textEditorReturnKeyPressed (TextEditor& editor) 
    {
        if (&editor == &getView().getAddressEditor())
            if (addressChangedCallback)
                if ( ! addressChangedCallback (editor.getText()))
                    editor.setText (String::empty);

        if (&editor == &getView().getSecondaryAddressEditor())
            if (secondaryAddressChangedCallback)
                if ( ! secondaryAddressChangedCallback (editor.getText()))
                    editor.setText (String::empty);

        if (&editor == &getView().getBundleAddressEditor())
            if (bundleAddressChangedCallback)
                bundleAddressChangedCallback (editor.getText());
    }

    void setAddressChangedCallback          (std::function<bool (String address)> function) 
    { 
        addressChangedCallback          = function; 
    }

    void setSecondaryAddressChangedCallback (std::function<bool (String address)> function) 
    { 
        secondaryAddressChangedCallback = function; 
    }
    void setBundleAddressChangedCallback    (std::function<void (String address)> function) { bundleAddressChangedCallback    = function; }

    OSCSettingsView& getView()
    {
        return view;
    }

private:
    std::function<bool (String address)> addressChangedCallback;
    std::function<bool (String address)> secondaryAddressChangedCallback;
    std::function<void (String address)> bundleAddressChangedCallback;
    OSCSettingsView view;
};

#endif  // OSCSETTINGS_H_INCLUDED
