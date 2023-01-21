/*
  ==============================================================================

    Osci.cpp
    Created: 17 Jan 2023 12:25:53pm
    Author:  yu

  ==============================================================================
*/

#include "Osci.h"

//==============================================================================
template <typename Type>
Osci<Type>::Osci()
{
    pc.template get<oscIdx>().initialise ([] (float x)
                                          { return std::sin (x); });
    pc.template get<gainIdx>().setGainDecibels (-100.0);
}
//==============================================================================
template <typename Type>
void Osci<Type>::setWaveType (WaveType choice)
{
    auto& osc = pc.template get<oscIdx>();

    switch (choice)
    {
        case WaveType::SIN:
            osc.initialise ([] (float x)
                            { return std::sin (x); });
            return;

        case WaveType::SAW:
            osc.initialise ([] (float x)
                            { return x / juce::MathConstants<float>::pi; });
            return;

        case WaveType::SQR:
            osc.initialise ([] (float x)
                            { return x < 0.0f ? -1.0f : 1.0f; });
            return;

        case WaveType::WSIN:
            osc.initialise ([] (float x)
                            { return std::sin (x); },
                            100);
            return;

        case WaveType::WSAW:
            osc.initialise ([] (float x)
                            { return x / juce::MathConstants<float>::pi; },
                            100);
            return;

        case WaveType::WSQR:
            osc.initialise ([] (float x)
                            { return x < 0.0f ? -1.0f : 1.0f; },
                            200);
            return;

        default:
            osc.initialise ([] (float x)
                            { return std::sin (x); });
            return;
    }
}
//==============================================================================
template <typename Type>
void Osci<Type>::setFrequency (Type newValue)
{
    pc.template get<oscIdx>().setFrequency (newValue);
}

//==============================================================================
template <typename Type>
void Osci<Type>::setLevel (Type newValue)
{
    pc.template get<gainIdx>().setGainDecibels (newValue);
}

//==============================================================================
template <typename Type>
Type Osci<Type>::processSample (Type input)
{
    return pc.template get<oscIdx>().processSample (input);
}

//==============================================================================
template <typename Type>
void Osci<Type>::reset() noexcept
{
    return;
}

//==============================================================================
template <typename Type>
void Osci<Type>::process (const juce::dsp::ProcessContextReplacing<Type>& context) noexcept
{
    pc.process (context);
}

//==============================================================================
template <typename Type>
void Osci<Type>::getNextAudioBlock (juce::dsp::AudioBlock<float>& block)
{
    // for (int channel = 0; channel < block.getNumChannels(); ++channel)
    // {
    //     for (int sample = 0; sample < block.getNumSamples(); ++sample)
    //     {
    //     }
    // }

    process (juce::dsp::ProcessContextReplacing<float> (block));
}

//==============================================================================
template <typename Type>
void Osci<Type>::prepare (const juce::dsp::ProcessSpec& spec)
{
    pc.prepare (spec);
}

//==============================================================================

// Explicit template instantiations to fix linking errors
// https://www.cs.technion.ac.il/users/yechiel/c++-faq/separate-template-class-defn-from-decl.html

template class Osci<float>;
