#include "Osc.h"

//==============================================================================
template <typename Type>
Osc<Type>::Osc()
{
}

//==============================================================================
template <typename Type>
void Osc<Type>::setWaveType (WaveType choice)
{
    auto& osc = pc.template get<oscIdx>();

    switch (choice)
    {
    case WaveType::SIN:
        osc.initialise ([] (float x) { return std::sin (x); });
        return;

    case WaveType::SAW:
        osc.initialise ([] (float x) { return x / juce::MathConstants<float>::pi; });
        return;

    case WaveType::SQR:
        osc.initialise ([] (float x) { return x < 0.0f ? -1.0f : 1.0f; });
        return;

    case WaveType::WSIN:
        osc.initialise ([] (float x) { return std::sin (x); }, 100);
        return;

    case WaveType::WSAW:
        osc.initialise ([] (float x) { return x / juce::MathConstants<float>::pi; }, 100);
        return;

    case WaveType::WSQR:
        osc.initialise ([] (float x) { return x < 0.0f ? -1.0f : 1.0f; }, 200);
        return;

    default:
        osc.initialise ([] (float x) { return std::sin (x); });
        return;
    }
}
//==============================================================================
template <typename Type>
void Osc<Type>::setFrequency (Type newValue)
{
    pc.template get<oscIdx>().setFrequency (newValue);
}

//==============================================================================
template <typename Type>
void Osc<Type>::setGainDecibels (Type newValue)
{
    pc.template get<gainIdx>().setGainDecibels (newValue);
}

//==============================================================================
template <typename Type>
void Osc<Type>::setGainLinear (Type newValue)
{
    pc.template get<gainIdx>().setGainLinear (newValue);
}

//==============================================================================
template <typename Type>
Type Osc<Type>::processSample (Type input)
{
    return pc.template get<oscIdx>().processSample (input);
}

//==============================================================================
template <typename Type>
void Osc<Type>::reset() noexcept
{
    return;
}

//==============================================================================
template <typename Type>
template <typename ProcessContext>
void Osc<Type>::process (const ProcessContext& context) noexcept
{
    auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();
    auto numSamples = outputBlock.getNumSamples();
    auto numChannels = outputBlock.getNumChannels();

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        auto* input = inputBlock.getChannelPointer (ch);
        auto* output = outputBlock.getChannelPointer (ch);

        for (size_t i = 0; i < numSamples; ++i)
        {
            pc.template get<oscIdx>().setFrequency (pc.template get<oscIdx>().getFrequency() + fm_freq * fm_depth);
            output[i] = pc.template get<oscIdx>().processSample (input[i]);
        }
    }
    pc.template get<gainIdx>().process (context);
}

//==============================================================================
template <typename Type>
void Osc<Type>::prepare (const juce::dsp::ProcessSpec& spec)
{
    pc.prepare (spec);
    fm.prepare (spec);

    pc.template get<oscIdx>().initialise ([] (float x) { return std::sin (x); });
    pc.template get<gainIdx>().setGainDecibels (-100.0);

    fm.initialise ([] (float x) { return std::sin (x); });
    fm_freq = 0;
    fm_depth = 0;
}

//==============================================================================
template <typename Type>
void Osc<Type>::setFmFreq (const Type freq)
{
    fm.setFrequency (freq);
    fm_freq = freq;
}

//==============================================================================
template <typename Type>
void Osc<Type>::setFmDepth (const Type depth)
{
    fm_depth = depth;
}

// Explicit template instantiations to satisfy the linker

template class Osc<float>;
template void Osc<float>::process<juce::dsp::ProcessContextReplacing<float>> (
    const juce::dsp::ProcessContextReplacing<float>& context);
