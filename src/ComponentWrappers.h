#pragma once
#include <JuceHeader.h>

class BaseComp : public juce::ChangeBroadcaster, protected juce::ValueTree::Listener
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
    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& p) override;
    // void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree&) override;
    // void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree&, int) override;
    // void valueTreeChildOrderChanged (juce::ValueTree& parentTree, int, int) override;
    // void valueTreeParentChanged (juce::ValueTree&) override;
    // void treeChildrenChanged (const juce::ValueTree& parentTree) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseComp)
};

//==============================================================================
class SliderComp : public BaseComp
{
public:
    SliderComp (const juce::ValueTree& v, juce::UndoManager& um, const juce::Identifier& propertie,
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
class ChoiceComp : public BaseComp
{
public:
    ChoiceComp (const juce::ValueTree& v, juce::UndoManager& um, const juce::Identifier& propertie,
                const juce::String& labelText, const juce::StringArray& options);

    juce::Component* getComponent() override;
    int getPreferredHeight() override;
    int getPreferredWidth() override;
    int getCurrentValue() const;

private:
    juce::ComboBox parameterBox;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceComp)
};
