#pragma once

#include "ComponentWrappers.h"
#include "Constants.h"
#include <JuceHeader.h>
#include <array>
#include <functional>

// Helpers
//==============================================================================
void setComponentGraphics (juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::String& text);

//==============================================================================
class ButtonsGui : public juce::Component
{
public:
    ButtonsGui (const std::vector<std::function<void()>>& funcs);
    void paint (juce::Graphics& g) override;
    void resized() override;
    int getWidthNeeded();
    int getHeightNeeded();

private:
    int btn_gap = 5;
    int btn_width = 50, btn_height = 20;
    int panic_btn_width = 20;
    static constexpr int NUM_OF_COMPONENTS = 4;
    std::array<std::unique_ptr<juce::TextButton>, NUM_OF_COMPONENTS> comps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonsGui)
};

//==============================================================================
class OutputGui : public juce::Component
{
public:
    OutputGui (juce::ValueTree& v, juce::UndoManager* um);
    void paint (juce::Graphics& g) override;
    void resized() override;
    int getWidthNeeded();
    int getHeightNeeded();

private:
    static constexpr int NUM_OF_COMPONENTS = 5;
    std::array<std::unique_ptr<SliderComp>, NUM_OF_COMPONENTS> comps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputGui)
};

//==============================================================================
class SequencerGui : public juce::Component
{
public:
    SequencerGui (juce::ValueTree& v, juce::UndoManager* um);
    void paint (juce::Graphics& g) override;
    void resized() override;
    int getWidthNeeded();
    int getHeightNeeded();

private:
    juce::ToggleButton onOff_btn;
    std::unique_ptr<SliderComp> slider;
    int btn_width = 25, btn_height = 20;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerGui)
};

//==============================================================================
class OscGui : public juce::Component
{
public:
    OscGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void setSelector (juce::ValueTree v, juce::UndoManager* um);
    int getWidthNeeded();
    int getHeightNeeded();

private:
    static constexpr int NUM_OF_COMPONENTS = 7;
    std::array<std::unique_ptr<BaseComp>, NUM_OF_COMPONENTS> comps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscGui)
};

//==============================================================================
class LfoGui : public juce::Component
{
public:
    LfoGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void setSelector (juce::ValueTree v, juce::UndoManager* um);
    int getWidthNeeded();
    int getHeightNeeded();

private:
    static constexpr int NUM_OF_COMPONENTS = 5;
    std::array<std::unique_ptr<BaseComp>, NUM_OF_COMPONENTS> comps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoGui)
};

//==============================================================================
class FiltGui : public juce::Component
{
public:
    FiltGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void setSelector (juce::ValueTree v, juce::UndoManager* um);
    int getWidthNeeded();
    int getHeightNeeded();

private:
    static constexpr int NUM_OF_COMPONENTS = 5;
    juce::ToggleButton onOff_btn;
    int btn_width = 25, btn_height = 20;
    std::array<std::unique_ptr<BaseComp>, NUM_OF_COMPONENTS> comps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FiltGui)
};

//==============================================================================
class DelayGui : public juce::Component
{
public:
    DelayGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void setSelector (juce::ValueTree v, juce::UndoManager* um);
    int getWidthNeeded();
    int getHeightNeeded();

private:
    static constexpr int NUM_OF_COMPONENTS = 4;
    std::array<std::unique_ptr<BaseComp>, NUM_OF_COMPONENTS> comps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayGui)
};
