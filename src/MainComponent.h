#pragma once

#include "Osci.h"
#include "Utils.h"
#include <JuceHeader.h>

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
    static juce::String getListOfActiveBits (const juce::BigInteger& b);

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // juce::AudioDeviceSelectorComponent adsc;
    std::unique_ptr<ParametersComponent> params;

    juce::Random random;

    // params recived from listners
    float master_gain = 0;

    float chan1_gain = 0;
    float chan2_gain = 0;

    double osc1_freq = 0;
    double osc2_freq = 0;

    double lfo1_freq = 0;
    float lfo1_gain = 0;

    double lfo2_freq = 0;
    float lfo2_gain = 0;
    // ========================

    using Gain = juce::dsp::Gain<float>;
    using Osc = Osci<float>;

    enum
    {
        oscIdx,
        gainIdx
    };

    using Chain = juce::dsp::ProcessorChain<Osc, Gain>;

    Chain chain1[2];
    Chain chain2[2];

    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;

    Osci<float> lfo1;
    Osci<float> lfo2;

    //==============================================================================
    void initParameters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
