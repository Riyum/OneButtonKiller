#pragma once

#include "Constants.h"
#include <JuceHeader.h>
#include <random>

//==============================================================================
template <typename Base, typename T>
inline bool instanceof (const T* ptr)
{
    return dynamic_cast<const Base*> (ptr) != nullptr;
}

inline float percentageFrom (const float amount, const float per)
{
    return amount * per / 100;
}

//==============================================================================
class Broadcaster : public juce::ChangeBroadcaster, protected juce::ValueTree::Listener
{
public:
    Broadcaster (const juce::ValueTree& v, const juce::Identifier& prop);

    juce::ValueTree getState() const;
    const juce::Identifier& propertie;

protected:
    juce::ValueTree state;
    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& p) override;
    // void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree&) override;
    // void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree&, int) override;
    // void valueTreeChildOrderChanged (juce::ValueTree& parentTree, int, int) override;
    // void valueTreeParentChanged (juce::ValueTree&) override;
    // void treeChildrenChanged (const juce::ValueTree& parentTree) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Broadcaster)
};

//==============================================================================
class undoMan : private juce::Timer
{
public:
    undoMan();
    juce::UndoManager& getManagerRef();
    juce::UndoManager* getManagerPtr();
    void undo();
    void redo();

private:
    juce::UndoManager um;
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (undoMan)
};

//==============================================================================
struct RAND_HELPER
{
    std::mt19937& gen;

    std::uniform_real_distribution<> p100;
    std::uniform_real_distribution<> p50;
    std::uniform_real_distribution<> p33;
    std::uniform_real_distribution<> p25;
    std::uniform_real_distribution<> p10;
    std::uniform_real_distribution<> p5;
    std::uniform_real_distribution<> p1;
    std::uniform_real_distribution<> p;

    RAND_HELPER (std::mt19937& _gen) : gen (_gen)
    {
        p100 = std::uniform_real_distribution<> (0, 100);
        p50 = std::uniform_real_distribution<> (0, 50);
        p33 = std::uniform_real_distribution<> (0, 33);
        p25 = std::uniform_real_distribution<> (0, 25);
        p10 = std::uniform_real_distribution<> (0, 10);
        p5 = std::uniform_real_distribution<> (0, 5);
        p1 = std::uniform_real_distribution<> (0, 1);
        p = p33;
    }

    double getVal (const double val)
    {
        // return a value with range [0, val]
        return percentageFrom (val, p100 (gen));
    }

    double getSup (const double val, const int per)
    {
        // reurn a suppressed value with range [0, val * max(p(gen)) / 100]
        switch (per)
        {
        case 1:
            p = p1;
            break;
        case 5:
            p = p5;
            break;
        case 10:
            p = p10;
            break;
        case 25:
            p = p25;
            break;
        case 33:
            p = p33;
            break;
        case 50:
            p = p50;
            break;
        }

        return percentageFrom (val, p (gen));
    }
};

//==============================================================================
juce::ValueTree createDefaultTree();
juce::ValueTree createSelectorsTree();
