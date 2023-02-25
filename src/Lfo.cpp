#include "Lfo.h"
#include "Constants.h"

template <typename Type>
Lfo<Type>::Lfo()
{
}

template <typename Type>
void Lfo<Type>::setWaveType (const WaveType choice)
{
    switch (choice)
    {
    case WaveType::SIN:
        lfo.initialise ([] (float x) { return std::sin (x); });
        return;

    case WaveType::SAW:
        lfo.initialise ([] (float x) { return x / juce::MathConstants<float>::pi; });
        return;

    case WaveType::SQR:
        lfo.initialise ([] (float x) { return x < 0.0f ? -1.0f : 1.0f; });
        return;

    case WaveType::WSIN:
        lfo.initialise ([] (float x) { return std::sin (x); }, 100);
        return;

    case WaveType::WSAW:
        lfo.initialise ([] (float x) { return x / juce::MathConstants<float>::pi; }, 100);
        return;

    case WaveType::WSQR:
        lfo.initialise ([] (float x) { return x < 0.0f ? -1.0f : 1.0f; }, 200);
        return;

    default:
        lfo.initialise ([] (float x) { return std::sin (x); });
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
Type Lfo<Type>::getFrequency()
{
    return lfo.getFrequency();
}

template <typename Type>
void Lfo<Type>::setGain (const Type newValue)
{

    gain = newValue;
}

template <typename Type>
Type Lfo<Type>::getGain()
{
    return gain;
}

template <typename Type>
void Lfo<Type>::addOscillator (Osc<Type>& left, Osc<Type>& right, const juce::ValueTree& v)
{
    oscs.add (new OscillatorInfo{left, right, v});
}

template <typename Type>
void Lfo<Type>::removeOscillator (Osc<Type>& oscillator)
{
    for (int i = 0; i < oscs.size(); ++i)
    {
        if (&oscs[i]->l == &oscillator)
        {
            oscs.remove (i);
            break;
        }
    }
}

// template <typename Type>
//     void Lfo<Type>::reset() noexcept;

template <typename Type>
void Lfo<Type>::process()
{
    for (int i = 0; i < oscs.size(); ++i)
    {
        float lfo_val = lfo.processSample (0.0f);
        Type osc_freq = oscs[i]->state.getProperty (IDs::freq);
        Type max_freq = param_limits.osc_freq_max;
        auto mod = juce::jmap (lfo_val, -1.f, 1.f, -1 * osc_freq, max_freq);

        oscs[i]->l.setBaseFrequency (osc_freq + mod * gain);
        oscs[i]->r.setBaseFrequency (osc_freq + mod * gain);
    }
}

template <typename Type>
void Lfo<Type>::prepare (const juce::dsp::ProcessSpec& spec)
{
    setWaveType (WaveType::SIN);
    gain = 0;
    lfo.prepare (spec);
}

template class Lfo<float>;
