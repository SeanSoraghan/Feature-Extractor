/*
  ==============================================================================

    FeatureExtractorLookAndFeel.h
    Created: 10 Feb 2016 8:00:52pm
    Author:  Sean

  ==============================================================================
*/

#ifndef FEATUREEXTRACTORLOOKANDFEEL_H_INCLUDED
#define FEATUREEXTRACTORLOOKANDFEEL_H_INCLUDED

//==============================================================================
/* Look and feel and layout functions
*/
class FeatureExtractorLookAndFeel : public LookAndFeel_V3
{
public:
    FeatureExtractorLookAndFeel() {}

    static float getAudioDisplayHeightRatio()               noexcept { return 0.2f; }
    static float getFeatureVisualiserComponentHeightRatio() noexcept { return 0.4f; }
    static float getSettingsHeightRatio()                   noexcept { return 0.4f; }
    static int   getFeatureVisualiserTextHeight()           noexcept { return 10; }
    static int   getFeatureVisualiserGraphicWidth()         noexcept { return 2; }
    static int   getOSCSettingsWidth()                      noexcept { return 200; }
    static int   getOSCSettingsHeight()                     noexcept { return 100; }
    static int   getFeatureVisualiserTextGraphicMargin()    noexcept { return 15; }
    static int   getVerticalMargin()                        noexcept { return 50; }
    static int   getHorizontalMargin()                      noexcept { return 30; }
    static int   getAnimationRateHz ()                      noexcept { return 60; }
    static float getCornerSize()                            noexcept { return 4.0f; }
    static float getLineThickness()                         noexcept { return 1.0f; }
    static Colour getTextColour()                           noexcept { return Colours::white; }
    static Colour getControlBackgroundColour()              noexcept { return Colours::green; }

    static void paintFeatureVisualiser (Graphics& g, float value, Rectangle<int> visualiserBounds)
    {
        g.setColour            (Colours::white);
        g.fillRoundedRectangle (visualiserBounds.toFloat(), getCornerSize());
        int height =           (int) (visualiserBounds.getHeight() * value);
        g.setColour            (Colours::green);
        g.fillRoundedRectangle (visualiserBounds.removeFromBottom (height).toFloat(), getCornerSize());
    }

    void drawLabel (Graphics& g, Label& label) override
    {
        g.fillAll (label.findColour (Label::backgroundColourId));

        if (! label.isBeingEdited())
        {
            const float alpha = label.isEnabled() ? 1.0f : 0.5f;
            const Font font (getLabelFont (label));

            g.setColour (getTextColour().withMultipliedAlpha (alpha));
            g.setFont (font);

            Rectangle<int> textArea (label.getBorderSize().subtractedFrom (label.getLocalBounds()));

            g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                              jmax (1, (int) (textArea.getHeight() / font.getHeight())),
                              label.getMinimumHorizontalScale());

            g.setColour (label.findColour (Label::outlineColourId).withMultipliedAlpha (alpha));
        }
        else if (label.isEnabled())
        {
            g.setColour (label.findColour (Label::outlineColourId));
        }

        g.drawRect (label.getLocalBounds());
    }

    void drawComboBox (Graphics& g, int width, int height, const bool /*isButtonDown*/,
                                   int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& box) override
    {
        g.setColour (getControlBackgroundColour());
        g.fillRoundedRectangle (box.getLocalBounds().toFloat(), getCornerSize());

        const Colour buttonColour (box.findColour (ComboBox::buttonColourId));

        const float arrowX = 0.3f;
        const float arrowH = 0.2f;

        Path p;
        p.addTriangle (buttonX + buttonW * 0.5f,            buttonY + buttonH * (0.45f - arrowH),
                       buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * 0.45f,
                       buttonX + buttonW * arrowX,          buttonY + buttonH * 0.45f);

        p.addTriangle (buttonX + buttonW * 0.5f,            buttonY + buttonH * (0.55f + arrowH),
                       buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * 0.55f,
                       buttonX + buttonW * arrowX,          buttonY + buttonH * 0.55f);

        g.setColour (box.findColour (ComboBox::arrowColourId).withMultipliedAlpha (box.isEnabled() ? 1.0f : 0.3f));
        g.fillPath (p);
    }

    void drawPopupMenuBackground (Graphics& g, int width, int height) override
    {
        g.fillAll (Colours::black);
        g.setColour (getControlBackgroundColour());
        g.fillRoundedRectangle (0, 0, (float) width, (float) height, getCornerSize());
        ignoreUnused (width, height);
    }

    void drawPopupMenuItem (Graphics& g, const Rectangle<int>& area,
                                        const bool isSeparator, const bool isActive,
                                        const bool isHighlighted, const bool isTicked,
                                        const bool hasSubMenu, const String& text,
                                        const String& shortcutKeyText,
                                        const Drawable* icon, const Colour* const textColourToUse) override
    {
        if (isSeparator)
        {
            Rectangle<int> r (area.reduced (5, 0));
            r.removeFromTop (r.getHeight() / 2 - 1);

            g.setColour (Colour (0x33000000));
            g.fillRect (r.removeFromTop (1));

            g.setColour (Colour (0x66ffffff));
            g.fillRect (r.removeFromTop (1));
        }
        else
        {
            Colour textColour (findColour (PopupMenu::textColourId));

            if (textColourToUse != nullptr)
                textColour = *textColourToUse;

            Rectangle<int> r (area.reduced (1));

            if (isHighlighted)
            {
                g.setColour (findColour (PopupMenu::highlightedBackgroundColourId));
                g.fillRoundedRectangle (r.toFloat(), getCornerSize());

                g.setColour (findColour (PopupMenu::highlightedTextColourId));
            }
            else
            {
                g.setColour (textColour);
            }

            if (! isActive)
                g.setOpacity (0.3f);

            Font font (getPopupMenuFont());

            const float maxFontHeight = area.getHeight() / 1.3f;

            if (font.getHeight() > maxFontHeight)
                font.setHeight (maxFontHeight);

            g.setFont (font);

            Rectangle<float> iconArea (r.removeFromLeft ((r.getHeight() * 5) / 4).reduced (3).toFloat());

            if (icon != nullptr)
            {
                icon->drawWithin (g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
            }
            else if (isTicked)
            {
                const Path tick (getTickShape (1.0f));
                g.fillPath (tick, tick.getTransformToScaleToFit (iconArea, true));
            }

            if (hasSubMenu)
            {
                const float arrowH = 0.6f * getPopupMenuFont().getAscent();

                const float x = (float) r.removeFromRight ((int) arrowH).getX();
                const float halfH = (float) r.getCentreY();

                Path p;
                p.addTriangle (x, halfH - arrowH * 0.5f,
                               x, halfH + arrowH * 0.5f,
                               x + arrowH * 0.6f, halfH);

                g.fillPath (p);
            }

            r.removeFromRight (3);
            g.drawFittedText (text, r, Justification::centredLeft, 1);

            if (shortcutKeyText.isNotEmpty())
            {
                Font f2 (font);
                f2.setHeight (f2.getHeight() * 0.75f);
                f2.setHorizontalScale (0.95f);
                g.setFont (f2);

                g.drawText (shortcutKeyText, r, Justification::centredRight, true);
            }
        }
    }

    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour,
                                           bool isMouseOverButton, bool isButtonDown) override
    {
        Colour baseColour (getControlBackgroundColour().withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                                                       .withMultipliedAlpha (button.isEnabled() ? 0.9f : 0.5f));

        if (isButtonDown || isMouseOverButton)
            baseColour = baseColour.contrasting (isButtonDown ? 0.2f : 0.1f);

        g.setColour (baseColour);
        g.fillRoundedRectangle (button.getLocalBounds().toFloat(), getCornerSize());
    }

    void drawTextEditorOutline (Graphics& g, int width, int height, TextEditor& textEditor) override
    {
        if (textEditor.isEnabled())
        {
            g.setColour (getControlBackgroundColour());
            g.drawRoundedRectangle (textEditor.getLocalBounds().toFloat(), getCornerSize(), getLineThickness());
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeatureExtractorLookAndFeel)
};



#endif  // FEATUREEXTRACTORLOOKANDFEEL_H_INCLUDED
