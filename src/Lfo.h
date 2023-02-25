#pragma once

#include "Constants.h"
#include "Osc.h"
#include <JuceHeader.h>

template <typename Type>
class Lfo
{
public:
    Lfo();

    void setFrequency (const Type newValue);
    Type getFrequency();
    void setGain (const Type newValue);
    Type getGain();
    void setWaveType (const WaveType choice);

    void addOscillator (Osc<Type>& left, Osc<Type>& right, const juce::ValueTree& v);
    void removeOscillator (Osc<Type>& oscillator);

    void reset() noexcept;
    void process();
    void prepare (const juce::dsp::ProcessSpec& spec);

private:
    juce::dsp::Oscillator<Type> lfo;

    Type frequency;
    Type gain;

    struct OscillatorInfo
    {
        Osc<Type>& l;
        Osc<Type>& r;
        juce::ValueTree state;
    };

    juce::OwnedArray<OscillatorInfo> oscs;

    Lfo (const Lfo&) = delete;
    Lfo& operator= (const Lfo&) = delete;
    JUCE_LEAK_DETECTOR (Lfo)
};
