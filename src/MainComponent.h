#pragma once

#include "Chain.h"
#include "ComponentWrappers.h"
#include "Constants.h"
#include "GuiComponents.h"
#include "Lfo.h"
#include "RandSequencer.h"
#include "Utils.h"

#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <random>
#include <vector>

// TODO: add reset to defaults button

class MainComponent : public juce::AudioAppComponent,
                      public juce::ChangeListener, // listening to the state envtes
                      public juce::KeyListener     // add keyboard events to the app
{
public:
    //==============================================================================
    MainComponent (juce::ValueTree st, juce::ValueTree selectors_st);
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    using juce::AudioAppComponent::keyPressed;
    using juce::AudioAppComponent::keyStateChanged;
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged (bool isKeyDown, juce::Component* originatingComponent) override;
    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    std::array<StereoChain, NUM_OUTPUT_CHANNELS / 2> chains;
    std::array<StereoBlock, NUM_OUTPUT_CHANNELS / 2> audio_blocks;

    // LFO
    size_t lfoUpdateCounter = def_params.lfoUpdateRate;
    std::array<std::unique_ptr<Lfo<float>>, NUM_OUTPUT_CHANNELS / 2> lfo;

    // sequencer
    RandSequencer seq;

    std::vector<std::unique_ptr<Broadcaster>> broadcasters;
    juce::ValueTree state;
    juce::ValueTree selectors_state;
    undoMan undoManager;

    std::random_device rd;
    std::mt19937 gen;
    RAND_HELPER rand;

    // GUI controllers
    std::unique_ptr<ButtonsGui> btn_comp;
    std::unique_ptr<OutputGui> output_comp;
    std::unique_ptr<SequencerGui> seq_comp;
    std::array<std::unique_ptr<OscGui>, NUM_OUTPUT_CHANNELS / 4> osc_comp;
    std::array<std::unique_ptr<LfoGui>, NUM_OUTPUT_CHANNELS / 4> lfo_comp;
    std::array<std::unique_ptr<FiltGui>, NUM_OUTPUT_CHANNELS / 4> filt_comp;
    std::array<std::unique_ptr<DelayGui>, NUM_OUTPUT_CHANNELS / 4> del_comp;
    // juce::AudioDeviceSelectorComponent adsc;

    //==============================================================================
    // void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    void initGuiComponents (const juce::ValueTree& v, const juce::ValueTree& vs);
    void initBroadcasters (const juce::ValueTree& v, const juce::ValueTree& vs);
    void setLfoRoute (const size_t lfo_idx, const size_t val);

    juce::var getStateParamValue (const juce::ValueTree& v, const juce::Identifier& parent,
                                  const juce::Identifier& node, const juce::Identifier& propertie);

    template <typename T>
    void setParam (const size_t idx, const juce::Identifier& comp_type, const juce::Identifier& propertie, T val);

    void setDefaultParameterValues();

    template <typename T>
    int getComponentWidth (const std::unique_ptr<T>& comp) const;

    template <typename T>
    int getComponentHeight (const std::unique_ptr<T>& comp) const;

    void generateRandomParameters();
    void generateRandomOscParameters (const int index, const bool suppressed = false);
    void generateRandomLfoParameters (const int index, const bool suppressed = false);
    void generateRandomFilterParameters (const int index, const bool suppressed = false);
    void generateRandomDelayParameters (const int index, const bool suppressed = false);

    void oscOn();
    void oscOff();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
