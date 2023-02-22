#pragma once
#include <JuceHeader.h>

// It doesn't matter which, but BaseComp need to derive from some juce class
// it seems that without the inheritance, instances of the derived classes from BaseComp
// are not properly deleted on app exit, and I get memory leaks
// that's odd since all my instances of those classes are unique pointers.
class BaseComp : public juce::ReferenceCountedObject
{
public:
    BaseComp (const juce::ValueTree& v, juce::UndoManager& um, const juce::Identifier& prop,
              const juce::String& labelText);

    virtual juce::Component* getComponent() = 0;
    virtual int getPreferredHeight() = 0;
    virtual int getPreferredWidth() = 0;

    juce::ValueTree getState() const;
    void setState (const juce::ValueTree& st);
    juce::UndoManager* getUndoManager() const;

    const juce::Identifier& propertie;
    juce::Label label;

protected:
    juce::ValueTree state;
    juce::UndoManager& undoManager;

private:
    // BaseComp (const BaseComp&) = delete;
    // BaseComp& operator= (const BaseComp&) = delete;
    // JUCE_LEAK_DETECTOR (BaseComp)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseComp)
};

//==============================================================================
class SliderComp : public BaseComp
{
public:
    SliderComp (const juce::ValueTree& v, juce::UndoManager& um, const juce::Identifier& propertie,
                const juce::String& labelText, juce::Range<double> range, double skew, const juce::String& suffix = {});
    ~SliderComp();

    juce::Component* getComponent() override;
    int getPreferredHeight() override;
    int getPreferredWidth() override;
    double getCurrentValue() const;

private:
    juce::Slider slider;

    // SliderComp (const SliderComp&) = delete;
    // SliderComp& operator= (const SliderComp&) = delete;
    // JUCE_LEAK_DETECTOR (SliderComp)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderComp)
};

//==============================================================================
class ChoiceComp : public BaseComp
{
public:
    ChoiceComp (const juce::ValueTree& v, juce::UndoManager& um, const juce::Identifier& propertie,
                const juce::String& labelText, const juce::StringArray& options);

    ~ChoiceComp();

    juce::Component* getComponent() override;
    int getPreferredHeight() override;
    int getPreferredWidth() override;
    int getCurrentValue() const;

private:
    juce::ComboBox parameterBox;

    // ChoiceComp (const ChoiceComp&) = delete;
    // ChoiceComp& operator= (const ChoiceComp&) = delete;
    // JUCE_LEAK_DETECTOR (ChoiceComp)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceComp)
};
