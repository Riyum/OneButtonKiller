#pragma once

#include "Osc.h"
#include <JuceHeader.h>

//==============================================================================
template <typename Base, typename T>
inline bool instanceof (const T* ptr)
{
    return dynamic_cast<const Base*> (ptr) != nullptr;
}

inline float percentageFrom (const float per, const float amount)
{
    return amount * per / 100;
}

//==============================================================================
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
DECLARE_ID (time)
DECLARE_ID (feedback)

#undef DECLARE_ID
} // namespace IDs

//==============================================================================
static struct DefaultParameters
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
    float del_time = 0.7;
    float del_feedback = 0.5;

} def_param;

static struct ControlLimits
{
    // JUCE slider setter/getters are expecting double types
    // combobox expecting int's

    double master_min = 0, master_max = 1;
    double chan_min = -100.0, chan_max = 0;

    int osc_waveType_min = 1, osc_waveType_max = 6;
    double osc_freq_min = 0, osc_freq_max = 24000;
    double osc_gain_min = -100, osc_gain_max = 5;
    double osc_fm_freq_min = 0, osc_fm_freq_max = 20;
    double osc_fm_depth_min = 0, osc_fm_depth_max = 10;

    int lfo_waveType_min = 1, lfo_waveType_max = 6;
    double lfo_freq_min = 0, lfo_freq_max = 70;
    double lfo_gain_min = 0, lfo_gain_max = 1;

    double delay_mix_min = 0, delay_mix_max = 1;
    double delay_time_min = 0, delay_time_max = 4.79;
    double delay_feedback_min = 0, delay_feedback_max = 1;

} ctl_limits;

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
class LabelComp : public BaseComp
{
public:
    LabelComp (const juce::ValueTree& v, const juce::Identifier propertie, juce::UndoManager& um,
               const juce::String& labelName);

    juce::Component* getComponent() override;
    int getPreferredHeight() override;
    int getPreferredWidth() override;
    juce::String getCurrentValue() const;

private:
    juce::Label label;
};

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
