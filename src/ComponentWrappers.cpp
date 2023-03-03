#include "ComponentWrappers.h"
#include "Constants.h"

BaseComp::BaseComp (const juce::Identifier& prop, const juce::String& labelText)
    : propertie (prop), label ("", labelText)
{
}

//==============================================================================
SliderComp::SliderComp (juce::ValueTree& v, juce::UndoManager* um, const juce::Identifier& prop,
                        const juce::String& labelText, juce::Range<double> range, double skew, const juce::String& suffix)
    : BaseComp (prop, labelText)
{
    slider.setRange (range.getStart(), range.getEnd(), 0.001);
    slider.setSliderStyle (juce::Slider::SliderStyle::Rotary);
    slider.setTextBoxStyle (juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 60, 20);
    slider.setSkewFactor (skew);

    if (suffix.isNotEmpty())
        slider.setTextValueSuffix (suffix);

    label.attachToComponent (&slider, false);
    label.setJustificationType (juce::Justification::centredBottom);

    slider.getValueObject().referTo (v.getPropertyAsValue (prop, um));
    // slider.setValue (v[prop]);
}

juce::Component* SliderComp::getComponent()
{
    return &slider;
}

int SliderComp::getPreferredHeight()
{
    return 70;
}

int SliderComp::getPreferredWidth()
{
    return 70;
}

double SliderComp::getCurrentValue() const
{
    return slider.getValue();
}

//==============================================================================
ComboComp::ComboComp (juce::ValueTree& v, juce::UndoManager* um, const juce::Identifier& prop,
                      const juce::String& labelText, const juce::StringArray& options)
    : BaseComp (prop, labelText)
{
    comboBox.addItemList (options, 1);

    comboBox.getSelectedIdAsValue().referTo (v.getPropertyAsValue (prop, um));
    // comboBox.setSelectedId (v[prop], juce::NotificationType::dontSendNotification);
}

juce::Component* ComboComp::getComponent()
{
    return &comboBox;
}

int ComboComp::getPreferredHeight()
{
    return 25;
}

int ComboComp::getPreferredWidth()
{
    return 80;
}

int ComboComp::getCurrentValue() const
{
    return comboBox.getSelectedId();
}

//==============================================================================
PopupComp::PopupComp (juce::ValueTree& v, juce::UndoManager* um, const juce::Identifier& prop,
                      const juce::String& labelText, const PopMenuParameters& params)
    : BaseComp (prop, labelText)
{
    for (auto const& [sub_menu_desc, sub_menu_items] : params)
    {
        sub_menus.add (new juce::PopupMenu());
        for (auto const& [itemId, item_desc] : sub_menu_items)
            sub_menus.getLast()->addItem (itemId, item_desc);

        menu.getRootMenu()->addSubMenu (sub_menu_desc, *sub_menus.getLast());
    }
    menu.getSelectedIdAsValue().referTo (v.getPropertyAsValue (prop, um));
}

juce::Component* PopupComp::getComponent()
{
    return &menu;
}

int PopupComp::getPreferredHeight()
{
    return 25;
}

int PopupComp::getPreferredWidth()
{
    return 80;
}
