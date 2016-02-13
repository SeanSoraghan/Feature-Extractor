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
    :   label       ("address", "Send to OSC Address:"),
        bundleLabel ("Bundle Address", "Use bundle address:")
    {
        addressEditor.setInputFilter (new TextEditor::LengthAndCharacterRestriction (30, "1234567890.:"), true);
        
        addAndMakeVisible (label);
        addAndMakeVisible (addressEditor);
        addAndMakeVisible (bundleLabel);
        addAndMakeVisible (bundleAddressEditor);
    }

    void resized() override
    {
        auto centralBounds = getLocalBounds().withSizeKeepingCentre (FeatureExtractorLookAndFeel::getOSCSettingsWidth(),
                                                                     FeatureExtractorLookAndFeel::getOSCSettingsHeight());
        int availableHeight = centralBounds.getHeight() - FeatureExtractorLookAndFeel::getVerticalMargin();
        for (int i = 0; i < getNumChildComponents(); i++)
        {
            getChildComponent (i)->setBounds (centralBounds.removeFromTop (availableHeight / 4));
        }
    }

    TextEditor& getAddressEditor()
    {
        return addressEditor;
    }

    TextEditor& getBundleAddressEditor()
    {
        return bundleAddressEditor;
    }

private:
    Label      label;
    Label      bundleLabel;
    TextEditor addressEditor;
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
        view.getBundleAddressEditor().addListener (this);
    }

    void textEditorReturnKeyPressed (TextEditor& editor) 
    {
        if (&editor == &getView().getAddressEditor())
            if (addressChangedCallback)
                if ( ! addressChangedCallback (editor.getText()))
                    editor.setText (String::empty);
        if (&editor == &getView().getBundleAddressEditor())
            if (bundleAddressChangedCallback)
                bundleAddressChangedCallback (editor.getText());
    }

    void setAddressChangedCallback       (std::function<bool (String address)> function) { addressChangedCallback = function; }
    void setBundleAddressChangedCallback (std::function<void (String address)> function) { bundleAddressChangedCallback = function; }

    OSCSettingsView& getView()
    {
        return view;
    }

private:
    std::function<bool (String address)> addressChangedCallback;
    std::function<void (String address)> bundleAddressChangedCallback;
    OSCSettingsView view;
};

#endif  // OSCSETTINGS_H_INCLUDED
