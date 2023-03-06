#include "Osc.h"

template <typename Type>
Osc<Type>::Osc()
{
}

template <typename Type>
void Osc<Type>::setWaveType (const WaveType choice)
{
    auto& osc = pc.template get<ProcIdx::OSC>();
    // Type dutyCycle = 0.25;

    switch (choice)
    {
    case WaveType::SIN:
        osc.initialise ([] (Type x) { return std::sin (x); });
        return;

    case WaveType::SAW:
        osc.initialise ([] (Type x) { return x / juce::MathConstants<Type>::pi; });
        return;

    case WaveType::SQR:
        osc.initialise ([] (Type x) { return x < 0.0f ? -1.0f : 1.0f; });
        // osc.initialise ([dutyCycle] (Type x)
        //                 { return (x < (dutyCycle * juce::MathConstants<Type>::twoPi)) ? 1.0f : -1.0f; });
        return;

    case WaveType::RAND:
        osc.initialise (
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
        osc.initialise ([] (Type x) { return std::sin (x); }, 100);
        return;

    case WaveType::WSAW:
        osc.initialise ([] (Type x) { return x / juce::MathConstants<Type>::pi; }, 100);
        return;

    case WaveType::WSQR:
        osc.initialise ([] (Type x) { return x < 0.0f ? -1.0f : 1.0f; }, 200);
        return;

    default:
        osc.initialise ([] (Type x) { return std::sin (x); });
        return;
    }
}

template <typename Type>
Type Osc<Type>::getBaseFrequency()
{
    return freq_base;
}

template <typename Type>
void Osc<Type>::setBaseFrequency (const Type newValue)
{
    freq_base = newValue;
}

template <typename Type>
Type Osc<Type>::getFrequency()
{
    return pc.template get<ProcIdx::OSC>().getFrequency();
}

template <typename Type>
void Osc<Type>::setFrequency (const Type newValue)
{
    pc.template get<ProcIdx::OSC>().setFrequency (newValue);
}

template <typename Type>
Type Osc<Type>::getGainDecibels()
{
    return pc.template get<ProcIdx::GAIN>().getGainDecibels();
}

template <typename Type>
void Osc<Type>::setGainDecibels (const Type newValue)
{
    pc.template get<ProcIdx::GAIN>().setGainDecibels (newValue);
}

template <typename Type>
Type Osc<Type>::getGainLinear()
{
    return pc.template get<ProcIdx::GAIN>().getGainLinear();
}

template <typename Type>
void Osc<Type>::setGainLinear (const Type newValue)
{
    pc.template get<ProcIdx::GAIN>().setGainLinear (newValue);
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

template <typename Type>
void Osc<Type>::setBypass (const bool b)
{
    bypass.store (b);
}

template <typename Type>
Type Osc<Type>::processSample (const Type input)
{
    return pc.template get<ProcIdx::OSC>().processSample (input);
}

template <typename Type>
void Osc<Type>::reset() noexcept
{
    pc.reset();
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
            Type fm_val = fm.processSample (0.f);
            Type cur_max = param_limits.osc_freq_max - freq_base;
            Type mod = fm.getFrequency() != 0 ? juce::jmap (fm_val, -1.f, 1.f, 0.f, cur_max) : 0;
            pc.template get<ProcIdx::OSC>().setFrequency (freq_base + mod * fm_depth);

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

// Explicit template instantiations to satisfy the linker

template class Osc<float>;
template void
Osc<float>::process<juce::dsp::ProcessContextReplacing<float>> (const juce::dsp::ProcessContextReplacing<float>& context);
