#include "ComponentWrappers.h"

BaseComp::BaseComp (const juce::ValueTree& v, juce::UndoManager& um, const juce::Identifier& prop,
                    const juce::String& labelText)
    : propertie (prop), label ("", labelText), state (v), undoManager (um)

{
}

juce::ValueTree BaseComp::getState() const
{
    return state;
}

void BaseComp::setState (const juce::ValueTree& st)
{
    state = st;
}

juce::UndoManager* BaseComp::getUndoManager() const
{
    return &undoManager;
}

//==============================================================================
SliderComp::SliderComp (const juce::ValueTree& v, juce::UndoManager& um, const juce::Identifier& prop,
                        const juce::String& labelText, juce::Range<double> range, double skew,
                        const juce::String& suffix)
    : BaseComp (v, um, prop, labelText)
{
    // std::cout << "Creating SliderComp\n";
    slider.setRange (range.getStart(), range.getEnd(), 0.001);
    slider.setSliderStyle (juce::Slider::SliderStyle::Rotary);
    slider.setTextBoxStyle (juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 60, 20);
    slider.setSkewFactor (skew);

    if (suffix.isNotEmpty())
        slider.setTextValueSuffix (suffix);

    label.attachToComponent (&slider, false);
    label.setJustificationType (juce::Justification::centredBottom);

    slider.getValueObject().referTo (getState().getPropertyAsValue (prop, getUndoManager()));
    // slider.setValue (v[prop]);
}

SliderComp::~SliderComp()
{
    // std::cout << "Destroying SliderComp\n";
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
ChoiceComp::ChoiceComp (const juce::ValueTree& v, juce::UndoManager& um, const juce::Identifier& prop,
                        const juce::String& labelText, const juce::StringArray& options)
    : BaseComp (v, um, prop, labelText)
{
    parameterBox.addItemList (options, 1);

    parameterBox.getSelectedIdAsValue().referTo (getState().getPropertyAsValue (prop, getUndoManager()));
    // parameterBox.setSelectedId (v[prop], juce::NotificationType::dontSendNotification);
    // std::cout << "Creating ChoiceComp\n";
}

ChoiceComp::~ChoiceComp()
{

    // std::cout << "Destroying ChoiceComp\n";
}

juce::Component* ChoiceComp::getComponent()
{
    return &parameterBox;
}

int ChoiceComp::getPreferredHeight()
{
    return 25;
}

int ChoiceComp::getPreferredWidth()
{
    return 80;
}

int ChoiceComp::getCurrentValue() const
{
    return parameterBox.getSelectedId();
}
