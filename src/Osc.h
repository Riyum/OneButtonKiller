#pragma once

#include <JuceHeader.h>
#include <atomic>

enum WaveType
{
    SIN = 1,
    SAW,
    SQR,
    WSIN,
    WSAW,
    WSQR
};

template <typename Type>
class Osc
{
public:
    Osc<Type>();

    void setWaveType (WaveType choice);

    Type getBaseFrequency();
    void setBaseFrequency (Type newValue);

    Type getFrequency();
    void setFrequency (Type newValue);

    Type getGainDecibels();
    void setGainDecibels (Type newValue);

    Type getGainLinear();
    void setGainLinear (Type newValue);

    void setFmFreq (const Type freq);
    void setFmDepth (const Type depth);
    void setBypass (const bool b);

    Type processSample (Type input);

    void reset() noexcept;
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept;
    void prepare (const juce::dsp::ProcessSpec& spec);

private:
    enum ProcIdx
    {
        OSC,
        GAIN,
    };

    juce::dsp::ProcessorChain<juce::dsp::Oscillator<Type>, juce::dsp::Gain<Type>> pc;
    juce::dsp::Oscillator<Type> fm;
    Type freq_base;
    Type fm_freq;
    Type fm_depth;
    std::atomic<bool> bypass = false;
};
