#include "Osc.h"

template <typename Type>
Osc<Type>::Osc()
{
}

template <typename Type>
void Osc<Type>::setWaveType (WaveType choice)
{
    auto& osc = pc.template get<ProcIdx::OSC>();

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

template <typename Type>
Type Osc<Type>::getBaseFrequency()
{
    return freq_base;
}

template <typename Type>
void Osc<Type>::setBaseFrequency (Type newValue)
{
    freq_base = newValue;
}

template <typename Type>
Type Osc<Type>::getFrequency()
{
    return pc.template get<ProcIdx::OSC>().getFrequency();
}

template <typename Type>
void Osc<Type>::setFrequency (Type newValue)
{
    pc.template get<ProcIdx::OSC>().setFrequency (newValue);
}

template <typename Type>
Type Osc<Type>::getGainDecibels()
{
    return pc.template get<ProcIdx::GAIN>().getGainDecibels();
}

template <typename Type>
void Osc<Type>::setGainDecibels (Type newValue)
{
    pc.template get<ProcIdx::GAIN>().setGainDecibels (newValue);
}

template <typename Type>
Type Osc<Type>::getGainLinear()
{
    return pc.template get<ProcIdx::GAIN>().getGainLinear();
}

template <typename Type>
void Osc<Type>::setGainLinear (Type newValue)
{
    pc.template get<ProcIdx::GAIN>().setGainLinear (newValue);
}

template <typename Type>
void Osc<Type>::setBypass (const bool b)
{
    bypass.store (b);
}

template <typename Type>
Type Osc<Type>::processSample (Type input)
{
    return pc.template get<ProcIdx::OSC>().processSample (input);
}

template <typename Type>
void Osc<Type>::reset() noexcept
{
    return;
}

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
            // get the current freq that maybe changed from the main proccess
            Type current_freq = pc.template get<ProcIdx::OSC>().getFrequency();
            pc.template get<ProcIdx::OSC>().setFrequency (current_freq + fm_freq * fm_depth);

            if (bypass.load())
                pc.template get<ProcIdx::OSC>().processSample (input[i]);
            else
                output[i] = pc.template get<ProcIdx::OSC>().processSample (input[i]);
        }
    }
    pc.template get<ProcIdx::GAIN>().process (context);
}

template <typename Type>
void Osc<Type>::prepare (const juce::dsp::ProcessSpec& spec)
{
    bypass.store (true);

    pc.prepare (spec);
    fm.prepare (spec);

    pc.template get<ProcIdx::OSC>().initialise ([] (float x) { return std::sin (x); });
    pc.template get<ProcIdx::GAIN>().setGainDecibels (-100.0);

    fm.initialise ([] (float x) { return std::sin (x); });
    freq_base = 440;
    fm_freq = 0;
    fm_depth = 0;
}

template <typename Type>
void Osc<Type>::setFmFreq (const Type freq)
{
    fm.setFrequency (freq);
    fm_freq = freq;
}

template <typename Type>
void Osc<Type>::setFmDepth (const Type depth)
{
    fm_depth = depth;
}

// Explicit template instantiations to satisfy the linker

template class Osc<float>;
template void Osc<float>::process<juce::dsp::ProcessContextReplacing<float>> (
    const juce::dsp::ProcessContextReplacing<float>& context);
