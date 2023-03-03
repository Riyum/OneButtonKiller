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
using _DEL = Delay<float, 1>;

// Chain definition
// signal flow: ... ---> Gain (channel) ---> Gain (master) ----> out
using Chain = juce::dsp::ProcessorChain<_OSC, _FILT, _DEL, _Gain, _Gain>;

// we need two chains for each stereo output channel
using StereoChain = std::pair<std::unique_ptr<Chain>, std::unique_ptr<Chain>>;

// each (stereo)chain need its own (stereo)block
using StereoBlock = std::pair<juce::dsp::AudioBlock<float>, juce::dsp::AudioBlock<float>>;
