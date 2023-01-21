/*
  ==============================================================================

    Osci.h
    Created: 17 Jan 2023 12:25:53pm
    Author:  yu

  ==============================================================================
*/

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
class Osci
{
public:
    Osci<Type>();
    void setWaveType (WaveType choice);
    void setFrequency (Type newValue);
    void setLevel (Type newValue);
    Type processSample (Type input);
    void reset() noexcept;
    void process (const juce::dsp::ProcessContextReplacing<Type>& context) noexcept;
    void getNextAudioBlock (juce::dsp::AudioBlock<float>& block);
    void prepare (const juce::dsp::ProcessSpec& spec);

private:
    enum
    {
        oscIdx,
        gainIdx,
    };

    juce::dsp::ProcessorChain<juce::dsp::Oscillator<Type>, juce::dsp::Gain<Type>> pc;
};

//==============================================================================
