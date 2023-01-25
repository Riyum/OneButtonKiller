#pragma once

#include "Osc.h"
#include "Utils.h"
#include <JuceHeader.h>
#include <array>
#include <memory>
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

    // GUI
    std::unique_ptr<ParametersComponent> paramsComp;

    static constexpr int NUM_INPUT_CHANNELS = 0;
    static constexpr int NUM_OUTPUT_CHANNELS = 4;

    struct Parameter
    {
        float master_gain = 0;  // linear
        float chan_gain = -100; // dB

        WaveType osc_wavetype = WaveType::SIN;
        double osc_freq = 0;
        float osc_gain = -100;

        WaveType lfo_wavetype = WaveType::SIN;
        double lfo_freq = 0;
        float lfo_gain = 0; // linear

        void setDefaultParams()
        {
            master_gain = 0.7;
            chan_gain = 0;

            osc_wavetype = WaveType::SIN;
            osc_freq = 440;
            osc_gain = -25;

            lfo_wavetype = WaveType::SIN;
            lfo_freq = 0;
            lfo_gain = 0;
        }
    };

    std::array<Parameter, static_cast<int> (NUM_OUTPUT_CHANNELS / 2)> chain_param;

    Parameter def_param;

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
