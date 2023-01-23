/* ==============================================================================
    Utils.h
    Created: 16 Jan 2023 9:04:18pm
    Author:  Riyum
   ============================================================================== */

/* ==============================================================================
                Copyright (c) 2022 - Raw Material Software Limited
   ============================================================================== */

#include <JuceHeader.h>

#pragma once

//==============================================================================
template <typename Base, typename T>
inline bool instanceof (const T* ptr)
{
    return dynamic_cast<const Base*> (ptr) != nullptr;
}

//==============================================================================
enum ParamId
{
    /* this enum plays the roll as the id of the component and its
     * index in the std::vector parameters as well */

    // Master Gain
    MASTER = 0,

    // Channels
    CHAN1_GAIN,
    CHAN2_GAIN,

    // Osc
    OSC1_WAVETYPE,
    OSC1_FREQ,
    OSC1_GAIN,

    OSC2_WAVETYPE,
    OSC2_FREQ,
    OSC2_GAIN,

    // LFO
    LFO1_WAVETYPE,
    LFO1_FREQ,
    LFO1_GAIN,

    LFO2_WAVETYPE,
    LFO2_FREQ,
    LFO2_GAIN
};

//==============================================================================
struct ParameterBase : public juce::ChangeBroadcaster
{
    ParameterBase (const ParamId id, const juce::String& labelName) : id (id), name (labelName)
    {
    }
    virtual ~ParameterBase() = default;

    virtual juce::Component* getComponent() = 0;

    virtual int getPreferredHeight() = 0;
    virtual int getPreferredWidth() = 0;

    ParamId id;
    juce::String name;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterBase)
};

//==============================================================================
struct SliderParameter : public ParameterBase
{
    SliderParameter (juce::Range<double> range, double skew, double initialValue, const ParamId id,
                     const juce::String& labelName, const juce::String& suffix = {})
        : ParameterBase (id, labelName)
    {
        /* slider.setRange (range.getStart(), range.getEnd(), 0.01); */
        slider.setRange (range.getStart(), range.getEnd());
        slider.setSkewFactor (skew);
        slider.setValue (initialValue);

        if (suffix.isNotEmpty())
            slider.setTextValueSuffix (suffix);

        slider.onValueChange = [this] { sendChangeMessage(); };
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
struct ChoiceParameter : public ParameterBase
{
    ChoiceParameter (const juce::StringArray& options, int initialId, const ParamId id, const juce::String& labelName)
        : ParameterBase (id, labelName)
    {
        parameterBox.addItemList (options, 1);
        parameterBox.onChange = [this] { sendChangeMessage(); };

        parameterBox.setSelectedId (initialId);
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

private:
    juce::ComboBox parameterBox;
};

//==============================================================================
// template <typename... Param>
// class Parameters
// {
// public:
//     Parameters (Param... p)
//         : parameters (p...)
//     {
//     }

//     template <int idx>
//     auto& get() noexcept
//     {
//         return std::get<idx> (parameters);
//     }

// private:
//     std::tuple<std::unique_ptr<Param...>> parameters;
// };

//==============================================================================
class ParametersComponent : public juce::Component
{
public:
    //==============================================================================
    ParametersComponent (std::vector<std::unique_ptr<ParameterBase>>& params)
    {
        parameters = std::move (params);

        for (auto&& param : parameters)
        {
            addAndMakeVisible (param->getComponent());

            std::unique_ptr<juce::Label> paramLabel = std::make_unique<juce::Label> ("", param->name);

            paramLabel->attachToComponent (param->getComponent(), true);
            paramLabel->setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (paramLabel.get());
            labels.push_back (std::move (paramLabel));
        }
    }

    //==============================================================================
    ~ParametersComponent()
    {
        for (auto& p : parameters)
            p->removeAllChangeListeners();
    }

    //==============================================================================
    void resized() override
    {
        auto bounds = getLocalBounds();

        for (auto&& p : parameters)
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

        for (auto&& p : parameters)
            width = std::max (p->getPreferredWidth(), width);

        return width + 10;
    }

    //==============================================================================
    int getHeightNeeded()
    {
        auto height = 0;

        for (auto&& p : parameters)
            height += p->getPreferredHeight();

        return height + 10;
    }

    //==============================================================================
    template <typename Type>
    Type getParam (ParamId idx)
    {
        auto* comp = parameters[idx].get();

        if (instanceof <SliderParameter> (comp))
            return dynamic_cast<SliderParameter*> (comp)->getCurrentValue();

        if (instanceof <ChoiceParameter> (comp))
            return dynamic_cast<ChoiceParameter*> (comp)->getCurrentValue();
    }

    //==============================================================================
private:
    std::vector<std::unique_ptr<ParameterBase>> parameters;
    std::vector<std::unique_ptr<juce::Label>> labels;

    //==============================================================================
};
//==============================================================================
