#include <JuceHeader.h>

template <typename Type>
class DelayLine
{
public:
    void clear() noexcept;
    size_t size() const noexcept;
    void resize (size_t newValue);
    Type back() const noexcept;
    Type get (size_t delayInSamples) const noexcept;
    void set (size_t delayInSamples, Type newValue) noexcept;
    void push (Type valueToAdd) noexcept;

private:
    std::vector<Type> rawData;
    size_t leastRecentIndex = 0;
};

//==============================================================================
template <typename Type, size_t maxNumChannels = 2>
class Delay
{
public:
    //==============================================================================
    Delay();
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    size_t getNumChannels() const noexcept;
    void setMaxDelayTime (Type newValue);
    void setFeedback (Type newValue) noexcept;
    void setWetLevel (Type newValue) noexcept;
    void setDelayTime (size_t channel, Type newValue);
    void setDelayTime (Type newValue);
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept;

private:
    //==============================================================================
    std::array<DelayLine<Type>, maxNumChannels> delayLines;
    std::array<size_t, maxNumChannels> delayTimesSample;
    std::array<Type, maxNumChannels> delayTimes;
    Type feedback{Type (0)};
    Type wetLevel{Type (0)};

    std::array<juce::dsp::IIR::Filter<Type>, maxNumChannels> filters;
    typename juce::dsp::IIR::Coefficients<Type>::Ptr filterCoefs;

    Type sampleRate{Type (44.1e3)};
    Type maxDelayTime{Type (2)};

    //==============================================================================
    void updateDelayLineSize();
    void updateDelayTime() noexcept;
};
