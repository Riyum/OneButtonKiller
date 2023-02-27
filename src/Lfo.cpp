#include "Lfo.h"
#include "Constants.h"

template <typename Type>
Lfo<Type>::Lfo (StereoChain& _chain, const size_t _id, const juce::ValueTree& _state)
    : chain (_chain), chain_id (_id), state (_state)
{
    setCompWithProp (IDs::OSC, IDs::freq, param_limits.osc_freq_max);
}

template <typename Type>
void Lfo<Type>::setWaveType (const WaveType choice)
{
    switch (choice)
    {
    case WaveType::SIN:
        lfo.initialise ([] (Type x) { return std::sin (x); });
        return;

    case WaveType::SAW:
        lfo.initialise ([] (Type x) { return x / juce::MathConstants<Type>::pi; });
        return;

    case WaveType::SQR:
        lfo.initialise ([] (Type x) { return x < 0.0f ? -1.0f : 1.0f; });
        return;

    case WaveType::WSIN:
        lfo.initialise ([] (Type x) { return std::sin (x); }, 100);
        return;

    case WaveType::WSAW:
        lfo.initialise ([] (Type x) { return x / juce::MathConstants<Type>::pi; }, 100);
        return;

    case WaveType::WSQR:
        lfo.initialise ([] (Type x) { return x < 0.0f ? -1.0f : 1.0f; }, 200);
        return;

    default:
        lfo.initialise ([] (Type x) { return std::sin (x); });
        return;
    }
}

template <typename Type>
void Lfo<Type>::setFrequency (const Type newValue)
{
    frequency = newValue;
    lfo.setFrequency (newValue);
}

template <typename Type>
Type Lfo<Type>::getFrequency() const
{
    return lfo.getFrequency();
}

template <typename Type>
void Lfo<Type>::setGain (const Type newValue)
{

    gain = newValue;
}

template <typename Type>
Type Lfo<Type>::getGain() const
{
    return gain;
}

template <typename Type>
void Lfo<Type>::setComp (const juce::Identifier& comp_type)
{
    comp_state = state.getChildWithName (comp_type).getChild (chain_id);
}

template <typename Type>
void Lfo<Type>::setProp (const juce::Identifier& _prop, Type max)
{
    prop = _prop;
    maxOut = max;

    if (comp_state.getParent().getType() == IDs::OSC)
    {
        if (prop == IDs::gain)
        {
            left = std::bind (&Osc<Type>::setGainDecibels, &chain.first->get<ProcIdx::OSC>(), std::placeholders::_1);
            right = std::bind (&Osc<Type>::setGainDecibels, &chain.second->get<ProcIdx::OSC>(), std::placeholders::_1);
        }

        if (prop == IDs::freq)
        {
            left = std::bind (&Osc<Type>::setBaseFrequency, &chain.first->get<ProcIdx::OSC>(), std::placeholders::_1);
            right = std::bind (&Osc<Type>::setBaseFrequency, &chain.second->get<ProcIdx::OSC>(), std::placeholders::_1);
        }

        if (prop == IDs::fm_freq)
        {
            left = std::bind (&Osc<Type>::setFmFreq, &chain.first->get<ProcIdx::OSC>(), std::placeholders::_1);
            right = std::bind (&Osc<Type>::setFmFreq, &chain.second->get<ProcIdx::OSC>(), std::placeholders::_1);
        }

        if (prop == IDs::fm_depth)
        {
            left = std::bind (&Osc<Type>::setFmDepth, &chain.first->get<ProcIdx::OSC>(), std::placeholders::_1);
            right = std::bind (&Osc<Type>::setFmDepth, &chain.second->get<ProcIdx::OSC>(), std::placeholders::_1);
        }
    }
}

template <typename Type>
void Lfo<Type>::setCompWithProp (const juce::Identifier& comp_type, const juce::Identifier& _prop, Type max)
{
    comp_state = state.getChildWithName (comp_type).getChild (chain_id);
    setProp (_prop, max);
}

// template <typename Type>
//     void Lfo<Type>::reset() noexcept;

template <typename Type>
void Lfo<Type>::process()
{
    float lfo_val = lfo.processSample (0.0f);
    Type cur = comp_state.getProperty (prop);
    Type max = maxOut - cur;

    Type mod;
    if (prop != IDs::gain)
        mod = juce::jmap (lfo_val, -1.f, 1.f, -1 * cur, max);
    else
        mod = juce::jmap (lfo_val, -1.f, 1.f, 0.f, -1 * cur + maxOut);

    left (cur + mod * gain);
    right (cur + mod * gain);
}

template <typename Type>
void Lfo<Type>::prepare (const juce::dsp::ProcessSpec& spec)
{
    setWaveType (WaveType::SIN);
    gain = 0;
    lfo.prepare (spec);
}

template class Lfo<float>;
