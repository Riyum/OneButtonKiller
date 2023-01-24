#pragma once

#include "Osc.h"
#include "Utils.h"
#include <JuceHeader.h>
#include <memory>

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

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // juce::AudioDeviceSelectorComponent adsc;

    // DSP processors
    using Gain = juce::dsp::Gain<float>;
    using OSC = Osc<float>;

    enum ProcIdx
    {
        oscIdx,
        chanGainIdx,
        masterGainIdx
    };

    // Chain definition
    // signal flow: ... Osc ---> Gain (channel) ---> Gain (master) ----> out
    using Chain = juce::dsp::ProcessorChain<OSC, Gain, Gain>;

    // we need two chains for each stereo output channel
    using StereoChain = std::pair<std::unique_ptr<Chain>, std::unique_ptr<Chain>>;
    std::vector<StereoChain> chains;

    // each (stereo)chain need its own (stereo)block
    using StereoBlock = std::pair<juce::dsp::AudioBlock<float>, juce::dsp::AudioBlock<float>>;
    std::vector<StereoBlock> audio_blocks;

    // LFO's, declared outside of the chain processor
    static constexpr float max_osc_freq = 22000.f;
    static constexpr size_t lfoUpdateRate = 100; // every 100 samples
    size_t lfoUpdateCounter = lfoUpdateRate;

    Osc<float> lfo1;
    Osc<float> lfo2;

    std::unique_ptr<ParametersComponent> params;

    static struct Default_parameters_values
    {
        float master_gain = 0.8; // linear
        float chans_gain = 0.0;  // dB

        WaveType osc_wavetype = WaveType::SAW;
        float osc_freq = 220.0;
        float osc_gain = -20.0;

        WaveType lfo_wavetype = WaveType::SIN;
        float lfo_freq = 0.0;
        float lfo_gain = -100.0;

    } Def_param_val;

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
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void initParameters();
    void setDefaultParameterValues();

    template <typename T, typename Func, typename... O>
    void setChainParams (T val, Func f, O*... obj);

    template <typename T>
    void setChainParams (StereoChain* chain, ParamId id, T val);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
