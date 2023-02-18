#pragma once

#include "Constants.h"
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
juce::ValueTree createDefaultTree();
juce::ValueTree createSelectorsTree();
