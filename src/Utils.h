#pragma once

#include "Constants.h"
#include <JuceHeader.h>

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
juce::ValueTree createDefaultTree();
juce::ValueTree createSelectorsTree();
