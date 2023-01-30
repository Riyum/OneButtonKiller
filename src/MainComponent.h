#pragma once

#include "Delay.h"
#include "Osc.h"
#include "Utils.h"
#include <JuceHeader.h>
#include <array>
#include <memory>
#include <random>
#include <vector>

class MainComponent : public juce::AudioAppComponent, public juce::ChangeListener, private juce::Timer
{
public:
    //==============================================================================
    MainComponent (juce::ValueTree root);
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
    // signal flow: ... ---> Gain (channel) ---> Gain (master) ----> out
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

    static struct ControlLimits
    {
        // in JUCE slider setter/getters are expecting double types
        // choicebox expecting int's

        double master_min = 0, master_max = 1;
        double chan_min = -100.0, chan_max = 0;

        int osc_waveType_min = 1, osc_waveType_max = 6;
        double osc_freq_min = 0, osc_freq_max = 24000;
        double osc_gain_min = -100, osc_gain_max = 5;
        double osc_fm_freq_min = 0, osc_fm_freq_max = 20;
        double osc_fm_depth_min = 0, osc_fm_depth_max = 10;

        int lfo_waveType_min = 1, lfo_waveType_max = 6;
        double lfo_freq_min = 0, lfo_freq_max = 70;
        double lfo_gain_min = 0, lfo_gain_max = 1;

        double delay_mix_min = 0, delay_mix_max = 1;

    } ctl_limits;

    juce::UndoManager undoManager;
    // parameters values
    juce::ValueTree state;
    // GUI parameters values
    juce::ValueTree gui_state;
    // default parameter values
    juce::ValueTree def_state;

    // GUI controllers
    std::vector<std::unique_ptr<juce::TextButton>> btns;
    std::unique_ptr<ControlComponent> ctlComp;

    //==============================================================================
    void timerCallback() override;
    juce::var getParamValue (const juce::Identifier& parent, const juce::Identifier& node,
                             const juce::Identifier& propertie);
    template <typename T>
    void setParamValue (const juce::Identifier& parent, const juce::Identifier& node, const juce::Identifier& propertie,
                        T val);
    juce::var getStateParamValue (const juce::ValueTree& v, const juce::Identifier& parent,
                                  const juce::Identifier& node, const juce::Identifier& propertie);
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void initGuiControls (juce::ValueTree& v);
    template <typename T, typename Func, typename... O>
    void setChainParams (T val, Func f, O*... obj);
    template <typename T>
    void setChainParams (StereoChain* chain, juce::Identifier comp_id, juce::Identifier propertie, T val);
    void setDefaultParameterValues();
    void generateRandomParameters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
