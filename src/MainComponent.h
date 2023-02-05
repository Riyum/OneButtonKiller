#pragma once

#include "Delay.h"
#include "Osc.h"
#include "Utils.h"
#include <JuceHeader.h>
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
    using DEL = Delay<float, 1>;

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

    juce::UndoManager undoManager;

    // chain parameters values
    juce::ValueTree state;
    // GUI parameters values
    juce::ValueTree gui_state;

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
    void setChainParams (StereoChain* chain, const juce::Identifier& comp_id, const juce::Identifier& propertie, T val);
    void setDefaultParameterValues();
    void generateRandomParameters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
