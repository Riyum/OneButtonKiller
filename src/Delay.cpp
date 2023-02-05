#include "Delay.h"
#include <JuceHeader.h>

template <typename Type>
void DelayLine<Type>::clear() noexcept
{
    std::fill (rawData.begin(), rawData.end(), Type (0));
}

template <typename Type>
size_t DelayLine<Type>::size() const noexcept
{
    return rawData.size();
}

template <typename Type>
void DelayLine<Type>::resize (size_t newValue)
{
    rawData.resize (newValue);
    leastRecentIndex = 0;
}

template <typename Type>
Type DelayLine<Type>::back() const noexcept
{
    return rawData[leastRecentIndex];
}

template <typename Type>
Type DelayLine<Type>::get (size_t delayInSamples) const noexcept
{
    jassert (delayInSamples >= 0 && delayInSamples < size());
    return rawData[(leastRecentIndex + 1 + delayInSamples) % size()];
}

/** Set the specified sample in the delay line */
template <typename Type>
void DelayLine<Type>::set (size_t delayInSamples, Type newValue) noexcept
{
    jassert (delayInSamples >= 0 && delayInSamples < size());
    rawData[(leastRecentIndex + 1 + delayInSamples) % size()] = newValue;
}

/** Adds a new value to the delay line, overwriting the least recently added sample */
template <typename Type>
void DelayLine<Type>::push (Type valueToAdd) noexcept
{
    rawData[leastRecentIndex] = valueToAdd;
    leastRecentIndex = leastRecentIndex == 0 ? size() - 1 : leastRecentIndex - 1;
}

// DELAY CLASS
//==============================================================================
template <typename Type, size_t maxNumChannels>
Delay<Type, maxNumChannels>::Delay()
{
    setMaxDelayTime (5.0f);
    setDelayTime (0, 0.7f);
    setWetLevel (0.8f);
    setFeedback (0.5f);
}

//==============================================================================
template <typename Type, size_t maxNumChannels>
void Delay<Type, maxNumChannels>::prepare (const juce::dsp::ProcessSpec& spec)
{
    jassert (spec.numChannels <= maxNumChannels);
    sampleRate = (Type)spec.sampleRate;
    updateDelayLineSize();
    updateDelayTime();

    filterCoefs = juce::dsp::IIR::Coefficients<Type>::makeFirstOrderLowPass (sampleRate, Type (1e3));

    for (auto& f : filters)
    {
        f.prepare (spec);
        f.coefficients = filterCoefs;
    }
}

//==============================================================================
template <typename Type, size_t maxNumChannels>
void Delay<Type, maxNumChannels>::reset() noexcept
{
    for (auto& f : filters)
        f.reset();

    for (auto& dline : delayLines)
        dline.clear();
}

//==============================================================================
template <typename Type, size_t maxNumChannels>
size_t Delay<Type, maxNumChannels>::getNumChannels() const noexcept
{
    return delayLines.size();
}

//==============================================================================
template <typename Type, size_t maxNumChannels>
void Delay<Type, maxNumChannels>::setMaxDelayTime (Type newValue)
{
    jassert (newValue > Type (0));
    maxDelayTime = newValue;
    updateDelayLineSize();
}

//==============================================================================
template <typename Type, size_t maxNumChannels>
void Delay<Type, maxNumChannels>::setFeedback (Type newValue) noexcept
{
    jassert (newValue >= Type (0) && newValue <= Type (1));
    feedback = newValue;
}

//==============================================================================
template <typename Type, size_t maxNumChannels>
void Delay<Type, maxNumChannels>::setWetLevel (Type newValue) noexcept
{
    jassert (newValue >= Type (0) && newValue <= Type (1));
    wetLevel = newValue;
}

//==============================================================================
template <typename Type, size_t maxNumChannels>
void Delay<Type, maxNumChannels>::setDelayTime (size_t channel, Type newValue)
{
    if (channel >= getNumChannels())
    {
        jassertfalse;
        return;
    }

    jassert (newValue >= Type (0));
    delayTimes[channel] = newValue;

    updateDelayTime();
}
//==============================================================================
template <typename Type, size_t maxNumChannels>
void Delay<Type, maxNumChannels>::setDelayTime (Type newValue)
{

    delayTimes[0] = newValue;
    updateDelayTime();
}

//==============================================================================
template <typename Type, size_t maxNumChannels>
template <typename ProcessContext>
void Delay<Type, maxNumChannels>::process (const ProcessContext& context) noexcept
// void Delay<Type, maxNumChannels>::process (const juce::dsp::ProcessContextReplacing<float>& context) noexcept
{
    auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();
    auto numSamples = outputBlock.getNumSamples();
    auto numChannels = outputBlock.getNumChannels();

    jassert (inputBlock.getNumSamples() == numSamples);
    jassert (inputBlock.getNumChannels() == numChannels);

    for (size_t ch = 0; ch < numChannels; ++ch)
    {
        auto* input = inputBlock.getChannelPointer (ch);
        auto* output = outputBlock.getChannelPointer (ch);
        auto& dline = delayLines[ch];
        auto delayTime = delayTimesSample[ch];
        auto& filter = filters[ch];

        for (size_t i = 0; i < numSamples; ++i)
        {
            auto delayedSample = dline.get (delayTime);
            auto inputSample = input[i];
            auto dlineInputSample = std::tanh (inputSample + feedback * delayedSample);
            dline.push (dlineInputSample);
            auto outputSample = inputSample + wetLevel * delayedSample;
            output[i] = outputSample;
        }
    }
}

template <typename Type, size_t maxNumChannels>
void Delay<Type, maxNumChannels>::updateDelayLineSize()
{
    auto delayLineSizeSamples = (size_t)std::ceil (maxDelayTime * sampleRate);

    for (auto& dline : delayLines)
        dline.resize (delayLineSizeSamples);
}

//==============================================================================
template <typename Type, size_t maxNumChannels>
void Delay<Type, maxNumChannels>::updateDelayTime() noexcept
{
    for (size_t ch = 0; ch < maxNumChannels; ++ch)
        delayTimesSample[ch] = (size_t)juce::roundToInt (delayTimes[ch] * sampleRate);
}

template class DelayLine<float>;
template class Delay<float, 1>;
// template class Delay<float>;
template void Delay<float, 1>::process<juce::dsp::ProcessContextReplacing<float>> (
    const juce::dsp::ProcessContextReplacing<float>& context);
