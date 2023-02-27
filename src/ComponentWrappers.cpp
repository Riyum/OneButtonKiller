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
                      const juce::String& labelText, const juce::StringArray& options,
                      const std::vector<MenuItems>& subOptions)
    : BaseComp (prop, labelText)
{
    for (size_t i = 0; i < static_cast<size_t> (options.size()); ++i)
    {
        sub_menus.add (new juce::PopupMenu());
        for (size_t j = 0; j < subOptions[i].size(); ++j)
            sub_menus[i]->addItem (subOptions[i][j]);

        menu.getRootMenu()->addSubMenu (options[i], *sub_menus[i]);
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

void PopupComp::updateMenu (std::vector<MenuItems>& sub_options)
{
    juce::PopupMenu::MenuItemIterator opt (*menu.getRootMenu());

    for (size_t i = 0; opt.next() != false; ++i)
    {
        auto sub_opt = juce::PopupMenu::MenuItemIterator (*opt.getItem().subMenu.get());

        for (size_t j = 0; sub_opt.next() != false; ++j)
            sub_opt.getItem().setAction (sub_options[i][j].action);
    }

    // for (size_t i = 0; i < static_cast<size_t> (sub_menus.size()); ++i)
    // {
    //     juce::PopupMenu::MenuItemIterator sub_opt (*sub_menus[i]);
    //     for (size_t j = 0; sub_opt.next() != false; ++j)
    //         sub_opt.getItem().setAction (sub_options[i][j].action);
    // }
}
