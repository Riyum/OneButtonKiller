#include "Lfo.h"
#include "Constants.h"

template <typename Type>
Lfo<Type>::Lfo (std::unique_ptr<Chain>& _chain, const size_t _id, const juce::ValueTree& _state)
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
void Lfo<Type>::setProp (const juce::Identifier& _prop, Type _max)
{
    prop = _prop;
    max = _max;

    if (comp_state.getParent().getType() == IDs::OUTPUT_GAIN)
    {
        if (prop == IDs::gain)
        {
            chain_func = std::bind (&juce::dsp::Gain<Type>::setGainDecibels, &chain->get<ProcIdx::CHAN_GAIN>(),
                                    std::placeholders::_1);
            return;
        }
    }

    if (comp_state.getParent().getType() == IDs::OSC)
    {
        if (prop == IDs::gain)
        {
            chain_func = std::bind (&Osc<Type>::setGainDecibels, &chain->get<ProcIdx::OSC>(), std::placeholders::_1);
            return;
        }

        if (prop == IDs::freq)
        {
            chain_func = std::bind (&Osc<Type>::setBaseFrequency, &chain->get<ProcIdx::OSC>(), std::placeholders::_1);
            return;
        }

        if (prop == IDs::fm_freq)
        {
            chain_func = std::bind (&Osc<Type>::setFmFreq, &chain->get<ProcIdx::OSC>(), std::placeholders::_1);
            return;
        }

        if (prop == IDs::fm_depth)
        {
            chain_func = std::bind (&Osc<Type>::setFmDepth, &chain->get<ProcIdx::OSC>(), std::placeholders::_1);
            return;
        }
    }

    if (comp_state.getParent().getType() == IDs::FILT)
    {
        if (prop == IDs::cutOff)
        {
            chain_func = std::bind (&juce::dsp::LadderFilter<Type>::setCutoffFrequencyHz, &chain->get<ProcIdx::FILT>(),
                                    std::placeholders::_1);
            return;
        }

        if (prop == IDs::reso)
        {
            chain_func = std::bind (&juce::dsp::LadderFilter<Type>::setResonance, &chain->get<ProcIdx::FILT>(),
                                    std::placeholders::_1);
            return;
        }

        if (prop == IDs::drive)
        {
            chain_func =
                std::bind (&juce::dsp::LadderFilter<Type>::setDrive, &chain->get<ProcIdx::FILT>(), std::placeholders::_1);
            return;
        }
    }
}

template <typename Type>
void Lfo<Type>::setLfoRoute (const juce::Identifier& comp_type, const juce::Identifier& _prop, Type _max)
{
    setComp (comp_type);
    setProp (_prop, _max);

    if (prop == IDs::gain)
        mod_func = [this]() { return juce::jmap (lfo_val, -1.f, 1.f, 0.f, -1 * cur + max); };
    else
        mod_func = [this]() { return juce::jmap (lfo_val, -1.f, 1.f, 0.f, cur_max); };
}

template <typename Type>
void Lfo<Type>::reset() noexcept
{
    lfo.reset();
}

template <typename Type>
void Lfo<Type>::process()
{
    lfo_val = lfo.processSample (0.0f);
    cur = comp_state.getProperty (prop);
    cur_max = max - cur;

    Type mod = getFrequency() != 0 ? mod_func() : 0;

    chain_func (cur + mod * gain);
}

template <typename Type>
void Lfo<Type>::prepare (const juce::dsp::ProcessSpec& spec)
{
    setWaveType (WaveType::SIN);
    gain = 0;
    lfo.prepare (spec);
}

template class Lfo<float>;
