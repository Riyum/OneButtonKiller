#pragma once

#include "Osc.h"
#include <JuceHeader.h>

namespace IDs
{
#define DECLARE_ID(name) const juce::Identifier name (#name);

DECLARE_ID (ROOT)
DECLARE_ID (uid)

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

namespace Helpers
{

template <typename Base, typename T>
inline bool instanceof (const T* ptr)
{
    return dynamic_cast<const Base*> (ptr) != nullptr;
}

inline juce::ValueTree createDefaultTree()
{

    float master_gain = 0.7; // linear
    float chan_gain = -10;   // dB

    WaveType osc_wavetype = WaveType::SIN;
    double osc_freq = 440;
    float osc_gain = -25;
    float osc_fm_freq = 0;
    float osc_fm_depth = 0;

    WaveType lfo_wavetype = WaveType::SIN;
    double lfo_freq = 0;
    float lfo_gain = 0; // linear

    float del_mix = 0;
    /* juce::ValueTree og1 = {IDs::OUTPUT_GAIN , {{IDs::master, master_gain}}} */

    juce::ValueTree root (IDs::ROOT);

    //===================================================
    juce::ValueTree og (IDs::OUTPUT_GAIN);

    juce::ValueTree master (IDs::MASTER);
    master.setProperty (IDs::gain, master_gain, nullptr);
    og.addChild (master, -1, nullptr);

    juce::ValueTree chan1 (IDs::CHAN1);
    chan1.setProperty (IDs::gain, chan_gain, nullptr);
    og.addChild (chan1, -1, nullptr);

    juce::ValueTree chan2 (IDs::CHAN2);
    chan2.setProperty (IDs::gain, chan_gain, nullptr);
    og.addChild (chan2, -1, nullptr);

    root.addChild (og, -1, nullptr);

    //===================================================
    juce::ValueTree osc (IDs::OSC);

    juce::ValueTree osc1 (IDs::OSC1);
    osc1.setProperty (IDs::wavetype, osc_wavetype, nullptr);
    osc1.setProperty (IDs::freq, osc_freq, nullptr);
    osc1.setProperty (IDs::gain, osc_gain, nullptr);
    osc1.setProperty (IDs::fm_freq, osc_fm_freq, nullptr);
    osc1.setProperty (IDs::fm_depth, osc_fm_depth, nullptr);
    osc.addChild (osc1, -1, nullptr);

    juce::ValueTree osc2 (IDs::OSC2);
    osc2.setProperty (IDs::wavetype, osc_wavetype, nullptr);
    osc2.setProperty (IDs::freq, osc_freq, nullptr);
    osc2.setProperty (IDs::gain, osc_gain, nullptr);
    osc2.setProperty (IDs::fm_freq, osc_fm_freq, nullptr);
    osc2.setProperty (IDs::fm_depth, osc_fm_depth, nullptr);
    osc.addChild (osc2, -1, nullptr);

    root.addChild (osc, -1, nullptr);

    //===================================================
    juce::ValueTree lfo (IDs::LFO);

    juce::ValueTree lfo1 (IDs::LFO1);
    lfo1.setProperty (IDs::wavetype, lfo_wavetype, nullptr);
    lfo1.setProperty (IDs::freq, lfo_freq, nullptr);
    lfo1.setProperty (IDs::gain, lfo_gain, nullptr);
    lfo.addChild (lfo1, -1, nullptr);

    juce::ValueTree lfo2 (IDs::LFO2);
    lfo2.setProperty (IDs::wavetype, lfo_wavetype, nullptr);
    lfo2.setProperty (IDs::freq, lfo_freq, nullptr);
    lfo2.setProperty (IDs::gain, lfo_gain, nullptr);
    lfo.addChild (lfo2, -1, nullptr);

    root.addChild (lfo, -1, nullptr);
    //===================================================
    juce::ValueTree delay (IDs::DELAY);

    juce::ValueTree delay1 (IDs::DELAY1);
    delay1.setProperty (IDs::mix, del_mix, nullptr);
    delay.addChild (delay1, -1, nullptr);

    juce::ValueTree delay2 (IDs::DELAY2);
    delay2.setProperty (IDs::mix, del_mix, nullptr);
    delay.addChild (delay2, -1, nullptr);

    root.addChild (delay, -1, nullptr);

    return root;
}

} // namespace Helpers

class BaseComp : public juce::ChangeBroadcaster, protected juce::ValueTree::Listener
{

public:
    BaseComp (const juce::ValueTree& v, const juce::Identifier p, juce::UndoManager& um, const juce::String& labelName)
        : name (labelName), state (v), undoManager (um)
    {
        state.addListener (this);
        id = v.getType();
        prop = p;
    }

    virtual juce::Component* getComponent() = 0;

    virtual int getPreferredHeight() = 0;
    virtual int getPreferredWidth() = 0;

    juce::ValueTree getState() const
    {
        return state;
    }

    juce::UndoManager* getUndoManager() const
    {
        return &undoManager;
    }

    juce::String name;
    juce::Identifier id;
    juce::Identifier prop;

protected:
    juce::ValueTree state;
    juce::UndoManager& undoManager;

    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& p) override
    {
        sendChangeMessage();
    }
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
    SliderComp (const juce::ValueTree& v, const juce::Identifier propertie, juce::UndoManager& um,
                const juce::String& labelName, juce::Range<double> range, double skew, double initialValue,
                const juce::String& suffix = {})
        : BaseComp (v, propertie, um, labelName)
    {
        slider.setRange (range.getStart(), range.getEnd(), 0.001);
        slider.setSkewFactor (skew);
        slider.setValue (initialValue);

        if (suffix.isNotEmpty())
            slider.setTextValueSuffix (suffix);

        slider.getValueObject().referTo (getState().getPropertyAsValue (propertie, getUndoManager()));
    }

    juce::Component* getComponent() override
    {
        return &slider;
    }

    int getPreferredHeight() override
    {
        return 40;
    }
    int getPreferredWidth() override
    {
        return 500;
    }

    double getCurrentValue() const
    {
        return slider.getValue();
    }

private:
    juce::Slider slider;
};

//==============================================================================
class ChoiceComp : public BaseComp
{
public:
    ChoiceComp (const juce::ValueTree& v, const juce::Identifier propertie, juce::UndoManager& um,
                const juce::String& labelName, const juce::StringArray& options, int initialId)
        : BaseComp (v, propertie, um, labelName)
    {
        parameterBox.addItemList (options, 1);
        parameterBox.setSelectedId (initialId);

        parameterBox.getSelectedIdAsValue().referTo (getState().getPropertyAsValue (propertie, getUndoManager()));
    }

    juce::Component* getComponent() override
    {
        return &parameterBox;
    }

    int getPreferredHeight() override
    {
        return 25;
    }
    int getPreferredWidth() override
    {
        return 250;
    }

    int getCurrentValue() const
    {
        return parameterBox.getSelectedId();
    }

    void setValue (const int val)
    {
        parameterBox.setSelectedId (val);
    }

private:
    juce::ComboBox parameterBox;
};

//==============================================================================
class BtnComp : public BaseComp
{
public:
    BtnComp (const juce::ValueTree& v, juce::UndoManager& um, const juce::String& buttonText,
             std::function<void()> func)
        : BaseComp (v, juce::Identifier(), um, "")
    {
        btn.setButtonText (buttonText);
        btn.onClick = func;
    }

    juce::Component* getComponent()
    {
        return &btn;
    }

    int getPreferredHeight()
    {
        return 20;
    }

    int getPreferredWidth()
    {
        return btn.getBestWidthForHeight (getPreferredHeight());
    }

private:
    juce::TextButton btn;
};

//==============================================================================
class ControlComponent : public juce::Component
{
public:
    //==============================================================================
    ControlComponent (std::vector<std::unique_ptr<BaseComp>>& params)
    {
        controls = std::move (params);

        for (auto&& param : controls)
        {
            addAndMakeVisible (param->getComponent());

            if (Helpers:: instanceof <juce::TextButton> (param.get()))
                continue;

            std::unique_ptr<juce::Label> paramLabel = std::make_unique<juce::Label> ("", param->name);
            paramLabel->attachToComponent (param->getComponent(), true);
            paramLabel->setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (paramLabel.get());
            labels.push_back (std::move (paramLabel));
        }
    }
    //==============================================================================
    void resized() override
    {
        auto bounds = getLocalBounds();

        for (auto&& p : controls)
        {
            auto* comp = p->getComponent();

            comp->setSize (juce::jmin (bounds.getWidth(), p->getPreferredWidth()), p->getPreferredHeight());

            auto compBounds = bounds.removeFromTop (p->getPreferredHeight());
            comp->setCentrePosition (compBounds.getCentre());
        }
    }
    //==============================================================================
    int getWidthNeeded()
    {
        auto width = 0;

        for (auto&& p : controls)
            width = std::max (p->getPreferredWidth(), width);

        // width + lable_width + gap
        return width + 10;
    }

    //==============================================================================
    int getHeightNeeded()
    {
        auto height = 0;

        for (auto&& p : controls)
            height += p->getPreferredHeight();

        return height + 10;
    }

    //==============================================================================
    //==============================================================================

private:
    std::vector<std::unique_ptr<BaseComp>> controls;
    std::vector<std::unique_ptr<juce::Label>> labels;

    //==============================================================================
};
