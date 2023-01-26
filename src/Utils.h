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

    // The magic button
    BTN = 0,

    // Master Gain
    MASTER,

    // Channels
    CHAN1_GAIN,
    CHAN2_GAIN,

    // Osc
    OSC1_WAVETYPE,
    OSC1_FREQ,
    OSC1_GAIN,
    OSC1_FM_FREQ,
    OSC1_FM_DEPTH,

    OSC2_WAVETYPE,
    OSC2_FREQ,
    OSC2_GAIN,
    OSC2_FM_FREQ,
    OSC2_FM_DEPTH,

    // LFO
    LFO1_WAVETYPE,
    LFO1_FREQ,
    LFO1_GAIN,

    LFO2_WAVETYPE,
    LFO2_FREQ,
    LFO2_GAIN,

    // Delay
    DEL_MIX,
};

//==============================================================================
struct BaseComp : public juce::ChangeBroadcaster
{
    BaseComp (const ParamId id, const juce::String& labelName) : id (id), name (labelName)
    {
    }
    virtual ~BaseComp() = default;

    virtual juce::Component* getComponent() = 0;

    virtual int getPreferredHeight() = 0;
    virtual int getPreferredWidth() = 0;

    ParamId id;
    juce::String name;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseComp)
};

//==============================================================================
struct SliderComp : public BaseComp
{
    SliderComp (juce::Range<double> range, double skew, double initialValue, const ParamId id,
                const juce::String& labelName, const juce::String& suffix = {})
        : BaseComp (id, labelName)
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

    void setValue (const double val)
    {
        slider.setValue (val);
    }

private:
    juce::Slider slider;
};

//==============================================================================
struct ChoiceComp : public BaseComp
{
    ChoiceComp (const juce::StringArray& options, int initialId, const ParamId id, const juce::String& labelName)
        : BaseComp (id, labelName)
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

    void setValue (const int val)
    {
        parameterBox.setSelectedId (val);
    }

private:
    juce::ComboBox parameterBox;
};

struct BtnComp : public BaseComp
{
    BtnComp (const juce::String& buttonText, std::function<void()> func, const ParamId id,
             const juce::String& labelName)
        : BaseComp (id, labelName)
    {
        btn.setButtonText (buttonText);
        // btn.onStateChange = [this] { sendChangeMessage(); };
        btn.onClick = func;
    }

    juce::Component* getComponent() override
    {
        return &btn;
    }

    int getPreferredHeight() override
    {
        return 20;
    }

    int getPreferredWidth() override
    {
        return btn.getBestWidthForHeight (getPreferredHeight());
    }

private:
    juce::TextButton btn;
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

            if (instanceof <juce::TextButton> (param.get()))
                continue;

            std::unique_ptr<juce::Label> paramLabel = std::make_unique<juce::Label> ("", param->name);

            paramLabel->attachToComponent (param->getComponent(), true);
            paramLabel->setJustificationType (juce::Justification::centredLeft);
            addAndMakeVisible (paramLabel.get());
            labels.push_back (std::move (paramLabel));
        }
    }

    //==============================================================================
    ~ControlComponent()
    {
        for (auto& p : controls)
            p->removeAllChangeListeners();
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

        return width + 110 + 10;
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
    template <typename Type>
    Type getValue (ParamId idx)
    {
        auto* comp = controls[idx].get();

        if (instanceof <SliderComp> (comp))
            return dynamic_cast<SliderComp*> (comp)->getCurrentValue();

        if (instanceof <ChoiceComp> (comp))
            return dynamic_cast<ChoiceComp*> (comp)->getCurrentValue();
    }

    //==============================================================================

    std::vector<std::unique_ptr<BaseComp>> controls;

private:
    std::vector<std::unique_ptr<juce::Label>> labels;

    //==============================================================================
};
//==============================================================================
