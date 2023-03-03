#include "Lfo.h"
#include "Constants.h"

template <typename Type>
Lfo<Type>::Lfo (StereoChain& _chain, const size_t _id, const juce::ValueTree& _state)
    : chain (_chain), chain_id (_id), state (_state)
{
    setLfoRoute (IDs::OSC, IDs::freq, param_limits.osc_freq_max);
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

    case WaveType::RAND:
        lfo.initialise (
            [] (Type x)
            {
                juce::ignoreUnused (x);
                static std::random_device rd;
                static std::mt19937 gen (rd());
                static std::uniform_real_distribution<Type> dist (-1.0f, 1.0f);
                return dist (gen);
            },
            256);
        return;

    case WaveType::WSIN:
    case WaveType::WSAW:
    case WaveType::WSQR:
        lfo.initialise ([] (Type x) { return std::sin (x); });

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
    if (comp_state.isValid())
        comp_state.sendPropertyChangeMessage (prop);

    comp_state = state.getChildWithName (comp_type).getChild (chain_id);
}

template <typename Type>
void Lfo<Type>::setProp (const juce::Identifier& _prop, Type max)
{
    prop = _prop;
    maxOut = max;

    if (comp_state.getParent().getType() == IDs::OUTPUT_GAIN)
    {
        if (prop == IDs::gain)
        {
            left = std::bind (&juce::dsp::Gain<Type>::setGainDecibels, &chain.first->get<ProcIdx::CHAN_GAIN>(),
                              std::placeholders::_1);
            right = std::bind (&juce::dsp::Gain<Type>::setGainDecibels, &chain.second->get<ProcIdx::CHAN_GAIN>(),
                               std::placeholders::_1);
        }
    }

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

    if (comp_state.getParent().getType() == IDs::DELAY)
    {
    }
}

template <typename Type>
void Lfo<Type>::setLfoRoute (const juce::Identifier& comp_type, const juce::Identifier& _prop, Type max)
{
    setComp (comp_type);
    setProp (_prop, max);
}

template <typename Type>
void Lfo<Type>::reset() noexcept
{
    lfo.reset();
}

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

    if (getFrequency() == 0)
        mod = 0;

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
