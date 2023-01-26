#pragma once

#include "Delay.h"
#include "Osc.h"
#include "Utils.h"
#include <JuceHeader.h>
#include <array>
#include <memory>
#include <random>
#include <vector>

class MainComponent : public juce::AudioAppComponent, public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent();
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
    static constexpr int NUM_INPUT_CHANNELS = 0;
    static constexpr int NUM_OUTPUT_CHANNELS = 4;
    // juce::AudioDeviceSelectorComponent adsc;

    // DSP processors
    using Gain = juce::dsp::Gain<float>;
    using OSC = Osc<float>;
    using DEL = Delay<float>;

    enum ProcIdx
    {
        oscIdx,
        delIdx,
        chanGainIdx,
        masterGainIdx
    };

    // Chain definition
    // signal flow: Osc ---> ... ---> Gain (channel) ---> Gain (master) ----> out
    using Chain = juce::dsp::ProcessorChain<OSC, DEL, Gain, Gain>;

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

    struct Parameter
    {
        float master_gain = 0.7; // linear
        float chan_gain = -10;   // dB

        WaveType osc_wavetype = WaveType::SIN;
        double osc_freq = 440;
        float osc_gain = -25;
        float osc_fm_freq = 0;
        float osc_fm_depth = 0;

        WaveType lfo_wavetype = WaveType::SIN;
        double lfo_freq = 0;
        float lfo_gain = 0; // linear

        float del_mix = 0;
    };

    using ChainsParameters = std::array<Parameter, static_cast<int> (NUM_OUTPUT_CHANNELS / 2)>;

    ChainsParameters chain_param;

    static struct Control_limits
    {
        // in JUCE slider setter/getters are expecting double types
        // choicebox expecting int's

        double master_min = 0, master_max = 1;
        double chan_min = -100.0, chan_max = 0;

        int osc_waveType_min = 1, osc_waveType_max = 6;
        double osc_freq_min = 0, osc_freq_max = 24000;
        double osc_gain_min = -100, osc_gain_max = 5;
        double osc_fm_freq_min = 0, osc_fm_freq_max = 10;
        double osc_fm_depth_min = 0, osc_fm_depth_max = 20;

        int lfo_waveType_min = 1, lfo_waveType_max = 6;
        double lfo_freq_min = 0, lfo_freq_max = 70;
        double lfo_gain_min = 0, lfo_gain_max = 1;

        double delay_mix_min = 0, delay_mix_max = 1;

    } ctl_limits;

    Parameter def_param;

    // GUI controllers
    std::unique_ptr<ControlComponent> ctlComp;

    //==============================================================================
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void initParameters();
    void setDefaultParameterValues();
    void generateRandomParameters();

    template <typename T, typename Func, typename... O>
    void setChainParams (T val, Func f, O*... obj);

    template <typename T>
    void setChainParams (StereoChain* chain, ParamId id, T val);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
