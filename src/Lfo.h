#pragma once

#include "Chain.h"
#include "Constants.h"
#include "Osc.h"
#include <JuceHeader.h>
#include <functional>
#include <random>

template <typename Type>
class Lfo
{
public:
    Lfo (StereoChain& _chain, const size_t _id, const juce::ValueTree& _state);

    void setWaveType (const WaveType choice);

    void setFrequency (const Type newValue);
    Type getFrequency() const;

    void setGain (const Type newValue);
    Type getGain() const;

    void setComp (const juce::Identifier& comp_type);
    void setProp (const juce::Identifier& _prop, Type max);
    void setLfoRoute (const juce::Identifier& comp_type, const juce::Identifier& _prop, Type max);

    void reset() noexcept;
    void process();
    void prepare (const juce::dsp::ProcessSpec& spec);

private:
    juce::dsp::Oscillator<Type> lfo;

    Type frequency;
    Type gain;

    StereoChain& chain;
    const size_t chain_id;
    juce::ValueTree state;
    juce::ValueTree comp_state;

    std::function<void (const Type)> left, right;
    juce::Identifier prop;
    Type maxOut;

    Lfo (const Lfo&) = delete;
    Lfo& operator= (const Lfo&) = delete;
    JUCE_LEAK_DETECTOR (Lfo)
};
