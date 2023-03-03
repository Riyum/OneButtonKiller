#pragma once

#include <JuceHeader.h>
#include <vector>

using PopMenuParameters = std::vector<std::pair<juce::String, std::vector<std::pair<int, juce::String>>>>;

// It doesn't matter which, but BaseComp need to derive from some juce class
// it seems that without the inheritance, instances of the derived classes from BaseComp
// are not properly deleted on app exit, and I get memory leaks
// that's odd since all my instances of those classes are unique pointers.
class BaseComp : public juce::ReferenceCountedObject
{
public:
    BaseComp (const juce::Identifier& prop, const juce::String& labelText);

    virtual juce::Component* getComponent() = 0;
    virtual int getPreferredHeight() = 0;
    virtual int getPreferredWidth() = 0;

    const juce::Identifier& propertie;
    juce::Label label;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseComp)
};

//==============================================================================
class SliderComp : public BaseComp
{
public:
    SliderComp (juce::ValueTree& v, juce::UndoManager* um, const juce::Identifier& propertie,
                const juce::String& labelText, juce::Range<double> range, double skew, const juce::String& suffix = {});

    juce::Component* getComponent() override;
    int getPreferredHeight() override;
    int getPreferredWidth() override;
    double getCurrentValue() const;

private:
    juce::Slider slider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderComp)
};

//==============================================================================
class ComboComp : public BaseComp
{
public:
    ComboComp (juce::ValueTree& v, juce::UndoManager* um, const juce::Identifier& propertie,
               const juce::String& labelText, const juce::StringArray& options);

    juce::Component* getComponent() override;
    int getPreferredHeight() override;
    int getPreferredWidth() override;
    int getCurrentValue() const;

private:
    juce::ComboBox comboBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComboComp)
};

//==============================================================================
class PopupComp : public BaseComp
{
public:
    PopupComp (juce::ValueTree& v, juce::UndoManager* um, const juce::Identifier& prop, const juce::String& labelText,
               const PopMenuParameters& params);

    juce::Component* getComponent() override;
    int getPreferredHeight() override;
    int getPreferredWidth() override;

private:
    juce::ComboBox menu;
    juce::OwnedArray<juce::PopupMenu> sub_menus;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PopupComp)
};
