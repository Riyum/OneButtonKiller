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
    void setGainDecibels (Type newValue);
    void setGainLinear (Type newValue);
    Type processSample (Type input);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<Type>& context) noexcept;
    void getNextAudioBlock (juce::dsp::AudioBlock<float>& block);
    void prepare (const juce::dsp::ProcessSpec& spec);

    void hello();

private:
    enum
    {
        oscIdx,
        gainIdx,
    };

    juce::dsp::ProcessorChain<juce::dsp::Oscillator<Type>, juce::dsp::Gain<Type>> pc;
};

//==============================================================================
