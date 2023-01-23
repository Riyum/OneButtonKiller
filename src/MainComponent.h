#pragma once

#include "Osci.h"
#include "Utils.h"
#include <JuceHeader.h>
#include <functional>

class MainComponent : public juce::AudioAppComponent, public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent (const int num_input_channels, const int num_output_channels);
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // juce::AudioDeviceSelectorComponent adsc;

    // DSP processors
    using Gain = juce::dsp::Gain<float>;
    using Osc = Osci<float>;

    enum ProcIdx
    {
        oscIdx,
        chanGainIdx,
        masterGainIdx
    };

    // Chain definition
    using Chain = juce::dsp::ProcessorChain<Osc, Gain, Gain>;

    // we need two chains for each stereo output channel
    Chain chain1[2];
    Chain chain2[2];

    // LFO's
    static constexpr float max_osc_freq = 22000.f;
    static constexpr size_t lfoUpdateRate = 100; // every 100 samples
    size_t lfoUpdateCounter = lfoUpdateRate;

    Osci<float> lfo1;
    Osci<float> lfo2;

    std::unique_ptr<ParametersComponent> params;

    // juce::Random random;

    // params received from listeners
    float master_gain = 0;

    float chan1_gain = 0;
    float chan2_gain = 0;

    double osc1_freq = 0;
    double osc2_freq = 0;

    double lfo1_freq = 0;
    float lfo1_gain = 0;

    double lfo2_freq = 0;
    float lfo2_gain = 0;

    //==============================================================================
    void initParameters();

    template <typename T, typename Func, typename... O>
    void setChainParams (T val, Func f, O*... obj);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
