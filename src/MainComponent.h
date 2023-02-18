#pragma once

#include "ComponentWrappers.h"
#include "Constants.h"
#include "Delay.h"
#include "GuiComponents.h"
#include "Osc.h"
#include "Utils.h"
#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include <memory>
#include <random>
#include <vector>

class MainComponent : public juce::AudioAppComponent, public juce::ChangeListener, private juce::Timer
{
public:
    //==============================================================================
    MainComponent (juce::ValueTree st, juce::ValueTree selectors_st);
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
    using Gain = juce::dsp::Gain<float>;
    using OSC = Osc<float>;
    using DEL = Delay<float, 1>;

    enum ProcIdx
    {
        osc,
        del,
        chanGain,
        masterGain
    };

    // Chain definition
    // signal flow: ... ---> Gain (channel) ---> Gain (master) ----> out
    using Chain = juce::dsp::ProcessorChain<OSC, DEL, Gain, Gain>;

    // we need two chains for each stereo output channel
    using StereoChain = std::pair<std::unique_ptr<Chain>, std::unique_ptr<Chain>>;
    std::vector<StereoChain> chains;
    // std::array<StereoChain, NUM_OUTPUT_CHANNELS / 2> chains;

    // each (stereo)chain need its own (stereo)block
    using StereoBlock = std::pair<juce::dsp::AudioBlock<float>, juce::dsp::AudioBlock<float>>;
    std::vector<StereoBlock> audio_blocks;
    // std::array<StereoBlock, NUM_OUTPUT_CHANNELS / 2> audio_blocks;

    size_t lfoUpdateCounter = def_params.lfoUpdateRate;
    std::vector<Osc<float>> lfo;

    juce::ValueTree state;
    juce::UndoManager undoManager;

    // GUI controllers
    std::unique_ptr<ButtonsGui> btn_comp;
    std::unique_ptr<OutputGui> output_comp;
    std::array<std::unique_ptr<OscGui>, NUM_OUTPUT_CHANNELS / 4> osc_comp;
    std::array<std::unique_ptr<LfoGui>, NUM_OUTPUT_CHANNELS / 4> lfo_comp;
    std::array<std::unique_ptr<DelayGui>, NUM_OUTPUT_CHANNELS / 4> del_comp;

    // juce::AudioDeviceSelectorComponent adsc;

    //==============================================================================
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void initGuiComponents (const juce::ValueTree& v, const juce::ValueTree& vs);

    juce::var getStateParamValue (const juce::ValueTree& v, const juce::Identifier& parent,
                                  const juce::Identifier& node, const juce::Identifier& propertie);

    template <typename T, typename Func, typename... O>
    void setChainParams (T val, Func f, O*... obj);
    template <typename T>
    void setChainParams (StereoChain* chain, const juce::Identifier& comp_type, const juce::Identifier& propertie,
                         T val);

    void setDefaultParameterValues();

    template <typename T>
    int getComponentWidth (const std::unique_ptr<T>& comp) const;

    template <typename T>
    int getComponentHeight (const std::unique_ptr<T>& comp) const;

    void generateRandomParameters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
