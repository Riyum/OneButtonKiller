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
    Lfo (std::unique_ptr<Chain>& _chain, const size_t _id, const juce::ValueTree& _state);

    void setWaveType (const WaveType choice);

    void setFrequency (const Type newValue);
    Type getFrequency() const;

    void setGain (const Type newValue);
    Type getGain() const;

    void setComp (const juce::Identifier& comp_type);
    void setProp (const juce::Identifier& _prop, Type _max);
    void setLfoRoute (const juce::Identifier& comp_type, const juce::Identifier& _prop, Type _max);

    void reset() noexcept;
    void process();
    void prepare (const juce::dsp::ProcessSpec& spec);

private:
    juce::dsp::Oscillator<Type> lfo;

    Type frequency;
    Type gain;

    std::unique_ptr<Chain>& chain;
    std::function<void (const Type)> chain_func;

    const size_t chain_id;
    juce::ValueTree state;
    juce::ValueTree comp_state;
    juce::Identifier prop;

    Type lfo_val, cur, cur_max, max;
    std::function<const Type()> mod_func;

    Lfo (const Lfo&) = delete;
    Lfo& operator= (const Lfo&) = delete;
    JUCE_LEAK_DETECTOR (Lfo)
};
