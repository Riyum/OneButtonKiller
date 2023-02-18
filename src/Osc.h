#pragma once

#include <JuceHeader.h>

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
    void setFrequency (Type newValue);
    void setFmFreq (const Type freq);
    void setFmDepth (const Type depth);
    void setGainDecibels (Type newValue);
    void setGainLinear (Type newValue);
    Type processSample (Type input);
    void reset() noexcept;
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept;
    void prepare (const juce::dsp::ProcessSpec& spec);

private:
    enum ProcIdx
    {
        osc,
        gain,
    };

    juce::dsp::ProcessorChain<juce::dsp::Oscillator<Type>, juce::dsp::Gain<Type>> pc;
    juce::dsp::Oscillator<Type> fm;
    Type fm_freq;
    Type fm_depth;
};
