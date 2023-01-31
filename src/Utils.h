#pragma once

#include "Osc.h"
#include <JuceHeader.h>

//==============================================================================

template <typename Base, typename T>
inline bool instanceof (const T* ptr)
{
    return dynamic_cast<const Base*> (ptr) != nullptr;
}

namespace IDs
{
#define DECLARE_ID(name) const juce::Identifier name (#name);

DECLARE_ID (ROOT)

DECLARE_ID (OUTPUT_GAIN)
DECLARE_ID (MASTER)
DECLARE_ID (CHAN1)
DECLARE_ID (CHAN2)

DECLARE_ID (OSC)
DECLARE_ID (OSC1)
DECLARE_ID (OSC2)
DECLARE_ID (LFO)
DECLARE_ID (LFO1)
DECLARE_ID (LFO2)

DECLARE_ID (wavetype)
DECLARE_ID (freq)
DECLARE_ID (gain)
DECLARE_ID (fm_freq)
DECLARE_ID (fm_depth)

DECLARE_ID (DELAY)
DECLARE_ID (DELAY1)
DECLARE_ID (DELAY2)
DECLARE_ID (mix)

#undef DECLARE_ID
} // namespace IDs

juce::ValueTree createDefaultTree();

//==============================================================================
class BaseComp : public juce::ChangeBroadcaster, protected juce::ValueTree::Listener
{

public:
    BaseComp (const juce::ValueTree& v, const juce::Identifier p, juce::UndoManager& um, const juce::String& labelName);

    virtual juce::Component* getComponent() = 0;
    virtual int getPreferredHeight() = 0;
    virtual int getPreferredWidth() = 0;

    juce::ValueTree getState() const;
    juce::UndoManager* getUndoManager() const;

    juce::String name;
    juce::Identifier prop;

protected:
    juce::ValueTree state;
    juce::UndoManager& undoManager;
    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& p) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseComp)
};

//==============================================================================
class SliderComp : public BaseComp
{
public:
    SliderComp (const juce::ValueTree& v, const juce::Identifier propertie, juce::UndoManager& um,
                const juce::String& labelName, juce::Range<double> range, double skew, double initialValue,
                const juce::String& suffix = {});

    juce::Component* getComponent() override;

    int getPreferredHeight() override;
    int getPreferredWidth() override;
    double getCurrentValue() const;

private:
    juce::Slider slider;
};

//==============================================================================
class ChoiceComp : public BaseComp
{
public:
    ChoiceComp (const juce::ValueTree& v, const juce::Identifier propertie, juce::UndoManager& um,
                const juce::String& labelName, const juce::StringArray& options, int initialId);

    juce::Component* getComponent() override;
    int getPreferredHeight() override;
    int getPreferredWidth() override;
    int getCurrentValue() const;

private:
    juce::ComboBox parameterBox;
};

//==============================================================================
// class BtnComp : public BaseComp
// {
// public:
//     BtnComp (const juce::ValueTree& v, juce::UndoManager& um, const juce::String& buttonText,
//              std::function<void()> func)
//         : BaseComp (v, juce::Identifier(), um, "")
//     {
//         btn.setButtonText (buttonText);
//         btn.onClick = func;
//     }

//     juce::Component* getComponent()
//     {
//         return &btn;
//     }

//     int getPreferredHeight()
//     {
//         return 20;
//     }

//     int getPreferredWidth()
//     {
//         return btn.getBestWidthForHeight (getPreferredHeight());
//     }

// private:
//     juce::TextButton btn;
// };

//==============================================================================
class ControlComponent : public juce::Component
{
public:
    ControlComponent (std::vector<std::unique_ptr<BaseComp>>& params);
    void resized() override;
    int getWidthNeeded();
    int getHeightNeeded();

private:
    std::vector<std::unique_ptr<BaseComp>> controls;
    std::vector<std::unique_ptr<juce::Label>> labels;
};
