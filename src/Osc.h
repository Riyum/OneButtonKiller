#pragma once

#include "Constants.h"
#include <JuceHeader.h>
#include <atomic>
#include <random>

template <typename Type>
class Osc
{
public:
    Osc<Type>();

    void setWaveType (const WaveType choice);

    Type getBaseFrequency();
    void setBaseFrequency (const Type newValue);

    Type getFrequency();
    void setFrequency (const Type newValue);

    Type getGainDecibels();
    void setGainDecibels (const Type newValue);

    Type getGainLinear();
    void setGainLinear (const Type newValue);

    void setFmFreq (const Type freq);
    void setFmDepth (const Type depth);
    void setBypass (const bool b);

    void setPanner (const Type newValue);

    Type processSample (const Type input);

    void reset() noexcept;
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept;
    void prepare (const juce::dsp::ProcessSpec& spec);

private:
    enum ProcIdx
    {
        OSC,
        GAIN,
        PAN
    };

    juce::dsp::ProcessorChain<juce::dsp::Oscillator<Type>, juce::dsp::Gain<Type>, juce::dsp::Panner<Type>> pc;
    juce::dsp::Oscillator<Type> fm;

    Type freq_base;
    Type fm_freq;
    Type fm_depth;

    std::atomic<bool> bypass = false;
};
