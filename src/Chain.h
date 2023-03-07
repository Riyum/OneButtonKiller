#pragma once

#include "Delay.h"
#include "Osc.h"
#include <JuceHeader.h>

enum ProcIdx
{
    OSC,
    FILT,
    DEL,
    CHAN_GAIN,
    MASTER_GAIN
};

using _Gain = juce::dsp::Gain<float>;
using _OSC = Osc<float>;
using _FILT = juce::dsp::LadderFilter<float>;
using _DEL = Delay<float, 2>;

// signal flow: ... ---> Gain (channel) ---> Gain (master) ----> out
using Chain = juce::dsp::ProcessorChain<_OSC, _FILT, _DEL, _Gain, _Gain>;
