#pragma once

#include "ComponentWrappers.h"
#include "Constants.h"
#include <JuceHeader.h>
#include <array>

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
    static constexpr int NUM_OF_COMPONENTS = 3;
    std::array<std::unique_ptr<juce::TextButton>, NUM_OF_COMPONENTS> comps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonsGui)
};

//==============================================================================
class OutputGui : public juce::Component
{
public:
    OutputGui (const juce::ValueTree& v, juce::UndoManager& um);
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
class OscGui : public juce::Component
{
public:
    OscGui (const juce::ValueTree& v, const juce::ValueTree& vs, juce::UndoManager& um);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void setSelector (const juce::ValueTree& v);
    int getWidthNeeded();
    int getHeightNeeded();

private:
    static constexpr int NUM_OF_COMPONENTS = 6;
    std::array<std::unique_ptr<BaseComp>, NUM_OF_COMPONENTS> comps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscGui)
};

//==============================================================================
class LfoGui : public juce::Component
{
public:
    LfoGui (const juce::ValueTree& v, const juce::ValueTree& vs, juce::UndoManager& um);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void setSelector (const juce::ValueTree& v);
    int getWidthNeeded();
    int getHeightNeeded();

private:
    static constexpr int NUM_OF_COMPONENTS = 4;
    std::array<std::unique_ptr<BaseComp>, NUM_OF_COMPONENTS> comps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoGui)
};

//==============================================================================
class DelayGui : public juce::Component
{
public:
    DelayGui (const juce::ValueTree& v, const juce::ValueTree& vs, juce::UndoManager& um);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void setSelector (const juce::ValueTree& v);
    int getWidthNeeded();
    int getHeightNeeded();

private:
    static constexpr int NUM_OF_COMPONENTS = 4;
    std::array<std::unique_ptr<BaseComp>, NUM_OF_COMPONENTS> comps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayGui)
};
