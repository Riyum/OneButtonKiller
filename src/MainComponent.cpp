#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent (const int num_input_channels, const int num_output_channels)
// : adsc (deviceManager, 0, num_input_channels, 0, num_output_channels, false, false, true, false)
{
    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && !juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        setAudioChannels (num_input_channels, num_output_channels);
    }

    // addAndMakeVisible (adsc);
    initParameters();

    setSize (params.get()->getWidthNeeded() + 10, params.get()->getHeightNeeded() + 10);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();

    /* for now, removing listeners from the parameters is done in ParametersComponent destructor */
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::dsp::ProcessSpec spec;

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlockExpected;
    spec.numChannels = 1;

    for (int i = 0; i < 2; i++)
    {
        chain1[i].prepare (spec);
        chain2[i].prepare (spec);
    }

    lfo1.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});
    lfo2.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});

    // Master gain
    setChainParams (0.8, &juce::dsp::Gain<float>::setGainLinear, &chain1[0].get<masterGainIdx>(),
                    &chain1[1].get<masterGainIdx>(), &chain2[0].get<masterGainIdx>(), &chain2[1].get<masterGainIdx>());

    // Channels gain
    setChainParams (0, &juce::dsp::Gain<float>::setGainDecibels, &chain1[0].get<chanGainIdx>(),
                    &chain1[1].get<chanGainIdx>(), &chain2[0].get<chanGainIdx>(), &chain2[1].get<chanGainIdx>());

    // OSC
    // setChainParams (WaveType::SAW, &Osci<float>::setWaveType, &chain1[0].get<oscIdx>(), &chain1[1].get<oscIdx>());
    // setChainParams (WaveType::SQR, &Osci<float>::setWaveType, &chain2[0].get<oscIdx>(), &chain2[1].get<oscIdx>());
    // setChainParams (-30.0, &Osci<float>::setLevel, &chain1[0].get<oscIdx>(), &chain1[1].get<oscIdx>(),
    //                 &chain2[0].get<oscIdx>(), &chain2[1].get<oscIdx>());
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    juce::dsp::AudioBlock<float> audioBlock{*bufferToFill.buffer};

    auto chan1_block = audioBlock.getSubsetChannelBlock (0, 2);
    auto chan2_block = audioBlock.getSubsetChannelBlock (2, 2);

    float lfo1_range = max_osc_freq - osc1_freq;
    float lfo2_range = max_osc_freq - osc2_freq;

    // process by sample
    for (auto samp = 0; samp < bufferToFill.numSamples; ++samp)
    {
        // auto noise = random.nextFloat() * 2.0f - 1.0f;

        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;

            auto lfoOut1 = lfo1.processSample (0.0f);
            float mod1 = juce::jmap (lfoOut1, -1.0f, 1.0f, static_cast<float> (osc1_freq), lfo1_range);
            setChainParams (osc1_freq + mod1 * lfo1_gain, &Osci<float>::setFrequency, &chain1[0].get<oscIdx>(),
                            &chain1[1].get<oscIdx>());

            auto lfoOut2 = lfo2.processSample (0.0f);
            float mod2 = juce::jmap (lfoOut2, -1.0f, 1.0f, static_cast<float> (osc2_freq), lfo2_range);
            setChainParams (osc2_freq + mod2 * lfo2_gain, &Osci<float>::setFrequency, &chain2[0].get<oscIdx>(),
                            &chain2[1].get<oscIdx>());
        }
        --lfoUpdateCounter;
    }

    // process by block, pass audio blocks for DPS processing
    for (unsigned i = 0; i < 2; i++)
    {
        auto block1 = chan1_block.getSingleChannelBlock (i);
        auto block2 = chan2_block.getSingleChannelBlock (i);

        chain1[i].process (juce::dsp::ProcessContextReplacing<float> (block1));
        chain2[i].process (juce::dsp::ProcessContextReplacing<float> (block2));
    }
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    ParamId id = static_cast<ParamId> (dynamic_cast<ParameterBase*> (source)->id);

    switch (id)
    {
    // Master Gain
    case ParamId::MASTER:
        master_gain = static_cast<float> (params->getParam<double> (ParamId::MASTER));
        setChainParams (master_gain, &juce::dsp::Gain<float>::setGainLinear, &chain1[0].get<masterGainIdx>(),
                        &chain1[1].get<masterGainIdx>(), &chain2[0].get<masterGainIdx>(),
                        &chain2[1].get<masterGainIdx>());
        return;

    // Channels Gain
    case ParamId::CHAN1_GAIN:
        setChainParams (static_cast<float> (params->getParam<double> (ParamId::CHAN1_GAIN)),
                        &juce::dsp::Gain<float>::setGainDecibels, &chain1[0].get<chanGainIdx>(),
                        &chain1[1].get<chanGainIdx>());
        return;

    case ParamId::CHAN2_GAIN:
        setChainParams (static_cast<float> (params->getParam<double> (ParamId::CHAN2_GAIN)),
                        &juce::dsp::Gain<float>::setGainDecibels, &chain2[0].get<chanGainIdx>(),
                        &chain2[1].get<chanGainIdx>());
        return;

    // OSC
    case ParamId::OSC1_WAVETYPE:
        setChainParams (static_cast<WaveType> (params->getParam<int> (ParamId::OSC1_WAVETYPE)),
                        &Osci<float>::setWaveType, &chain1[0].get<oscIdx>(), &chain1[1].get<oscIdx>());
        return;
    case ParamId::OSC1_FREQ:
        osc1_freq = params->getParam<double> (ParamId::OSC1_FREQ);
        setChainParams (osc1_freq, &Osci<float>::setFrequency, &chain1[0].get<oscIdx>(), &chain1[1].get<oscIdx>());
        return;
    case ParamId::OSC1_GAIN:
        setChainParams (static_cast<float> (params->getParam<double> (ParamId::OSC1_GAIN)), &Osci<float>::setLevel,
                        &chain1[0].get<oscIdx>(), &chain1[1].get<oscIdx>());
        return;

    case ParamId::OSC2_WAVETYPE:
        setChainParams (static_cast<WaveType> (params->getParam<int> (ParamId::OSC2_WAVETYPE)),
                        &Osci<float>::setWaveType, &chain2[0].get<oscIdx>(), &chain2[1].get<oscIdx>());
        return;
    case ParamId::OSC2_FREQ:
        osc2_freq = params->getParam<double> (ParamId::OSC2_FREQ);
        setChainParams (osc2_freq, &Osci<float>::setFrequency, &chain2[0].get<oscIdx>(), &chain2[1].get<oscIdx>());
        return;
    case ParamId::OSC2_GAIN:
        setChainParams (static_cast<float> (params->getParam<double> (ParamId::OSC2_GAIN)), &Osci<float>::setLevel,
                        &chain2[0].get<oscIdx>(), &chain2[1].get<oscIdx>());
        return;

    // LFO
    case ParamId::LFO1_WAVETYPE:
        lfo1.setWaveType (static_cast<WaveType> (params->getParam<int> (ParamId::LFO1_WAVETYPE)));
        return;
    case ParamId::LFO1_FREQ:
        lfo1_freq = params->getParam<double> (ParamId::LFO1_FREQ);
        lfo1.setFrequency (lfo1_freq);
        return;
    case ParamId::LFO1_GAIN:
        lfo1_gain = static_cast<float> (params->getParam<double> (ParamId::LFO1_GAIN));
        lfo1.setLevel (lfo1_gain);
        return;

    case ParamId::LFO2_WAVETYPE:
        lfo2.setWaveType (static_cast<WaveType> (params->getParam<int> (ParamId::LFO2_WAVETYPE)));
        return;
    case ParamId::LFO2_FREQ:
        lfo2_freq = params->getParam<double> (ParamId::LFO2_FREQ);
        lfo2.setFrequency (lfo2_freq);
        return;
    case ParamId::LFO2_GAIN:
        lfo2_gain = static_cast<float> (params->getParam<double> (ParamId::LFO2_GAIN));
        lfo2.setLevel (lfo2_gain);
        return;
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

void MainComponent::initParameters()
{

    std::vector<std::unique_ptr<ParameterBase>> parameters;

    // Master Gain
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{0.0, 1.0}, 1, 0.8, ParamId::MASTER, "Master", "%"));

    // Channels Gain
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{-100.0, 0.0}, 1, 0.0, ParamId::CHAN1_GAIN, "ch1 Gain", "dB"));
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{-100.0, 0.0}, 1, 0.0, ParamId::CHAN2_GAIN, "ch2 Gain", "dB"));

    // OSC
    parameters.push_back (
        std::make_unique<ChoiceParameter> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"}, 3,
                                           ParamId::OSC1_WAVETYPE, "OSC1 Wave type"));
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{0.0, 22000.0}, 0.4, 442.0, ParamId::OSC1_FREQ, "Freq", "Hz"));
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{-100.0, 10.0}, 3.0, -20.0, ParamId::OSC1_GAIN, "Gain", "dB"));

    parameters.push_back (
        std::make_unique<ChoiceParameter> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"}, 2,
                                           ParamId::OSC2_WAVETYPE, "OSC2 Wave type"));
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{0.0, 22000.0}, 0.4, 440.0, ParamId::OSC2_FREQ, "Freq", "Hz"));
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{-100.0, 10.0}, 3.0, -20.0, ParamId::OSC2_GAIN, "Gain", "dB"));

    // LFO
    parameters.push_back (
        std::make_unique<ChoiceParameter> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"}, 3,
                                           ParamId::LFO1_WAVETYPE, "LFO1 Wave type"));
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{0.0, 70.0}, 0.6, 0.0, ParamId::LFO1_FREQ, "Freq", "Hz"));
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{0.0, 1.0}, 0.3, 0.0, ParamId::LFO1_GAIN, "Gain"));

    parameters.push_back (
        std::make_unique<ChoiceParameter> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"}, 3,
                                           ParamId::LFO2_WAVETYPE, "LFO2 Wave type"));
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{0.0, 70.0}, 0.6, 0.0, ParamId::LFO2_FREQ, "Freq", "Hz"));
    parameters.push_back (
        std::make_unique<SliderParameter> (juce::Range{0.0, 1.0}, 0.3, 0.0, ParamId::LFO2_GAIN, "Gain"));

    for (auto& p : parameters)
        p->addChangeListener (this);

    params = std::make_unique<ParametersComponent> (parameters);
    addAndMakeVisible (params.get());
    resized();
}

template <typename T, typename Func, typename... O>
void MainComponent::setChainParams (T val, Func f, O*... obj)
{
    (..., (obj->*f) (val));
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    if (params.get() != nullptr)
        params->setBounds (bounds.removeFromTop (params->getHeightNeeded()).reduced (20, 0));
}
