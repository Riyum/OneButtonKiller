#include "MainComponent.h"

// the linker need and explicit definition for the static struct
MainComponent::Default_parameters_values MainComponent::Def_param_val;

//==============================================================================
MainComponent::MainComponent (const int num_input_channels, const int num_output_channels)
// : adsc (deviceManager, 0, num_input_channels, 0, num_output_channels, false, false, true, false)
{
    for (int i = 0; i < num_output_channels / 2; i++)
        chains.push_back (std::make_pair (std::make_unique<Chain>(), std::make_unique<Chain>()));

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

    /* since the parameters vector is private in parametersComponent
     * removing listeners from the parameters is done in the ParametersComponent destructor
     */
}
//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::dsp::ProcessSpec spec;

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlockExpected;
    spec.numChannels = 1;

    for (auto& chain : chains)
    {
        chain.first->prepare (spec);
        chain.second->prepare (spec);
    }

    lfo1.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});
    lfo2.prepare ({spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});

    setDefaultParameterValues();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    juce::dsp::AudioBlock<float> audioBlock{*bufferToFill.buffer};

    for (int i = 0; i <= bufferToFill.buffer->getNumChannels() / 2; i += 2)
        audio_blocks.push_back (
            std::make_pair (audioBlock.getSingleChannelBlock (i), audioBlock.getSingleChannelBlock (i + 1)));

    float lfo1_range = max_osc_freq - osc1_freq;
    float lfo2_range = max_osc_freq - osc2_freq;

    // process by sample
    for (auto samp = 0; samp < bufferToFill.numSamples; ++samp)
    {
        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;

            auto lfoOut1 = lfo1.processSample (0.0f);
            float mod1 = juce::jmap (lfoOut1, -1.0f, 1.0f, static_cast<float> (osc1_freq), lfo1_range);
            setChainParams (&chains[0], ParamId::OSC1_FREQ, osc1_freq + mod1 * lfo1_gain);

            auto lfoOut2 = lfo2.processSample (0.0f);
            float mod2 = juce::jmap (lfoOut2, -1.0f, 1.0f, static_cast<float> (osc2_freq), lfo2_range);
            setChainParams (&chains[1], ParamId::OSC2_FREQ, osc2_freq + mod2 * lfo2_gain);
        }
        --lfoUpdateCounter;
    }

    jassert (chains.size() == audio_blocks.size());
    // process by blocks
    for (int i = 0; i < chains.size(); i++)
    {
        chains[i].first->process (juce::dsp::ProcessContextReplacing<float> (audio_blocks[i].first));
        chains[i].second->process (juce::dsp::ProcessContextReplacing<float> (audio_blocks[i].second));
    }

    audio_blocks.clear();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{

    ParamId id = static_cast<ParamId> (dynamic_cast<ParameterBase*> (source)->id);

    switch (id)
    {
    // Master Gain
    case ParamId::MASTER:
        master_gain = static_cast<float> (params->getParam<double> (ParamId::MASTER));
        setChainParams (nullptr, ParamId::MASTER, master_gain);
        return;

    // Channels Gain
    case ParamId::CHAN1_GAIN:
        setChainParams (&chains[0], ParamId::CHAN1_GAIN,
                        static_cast<float> (params->getParam<double> (ParamId::CHAN1_GAIN)));
        return;

    case ParamId::CHAN2_GAIN:
        setChainParams (&chains[1], ParamId::CHAN2_GAIN,
                        static_cast<float> (params->getParam<double> (ParamId::CHAN2_GAIN)));
        return;

    // // OSC
    case ParamId::OSC1_WAVETYPE:
        setChainParams (&chains[0], ParamId::OSC1_WAVETYPE,
                        static_cast<WaveType> (params->getParam<int> (ParamId::OSC1_WAVETYPE)));
        return;
    case ParamId::OSC1_FREQ:
        osc1_freq = params->getParam<double> (ParamId::OSC1_FREQ);
        setChainParams (&chains[0], ParamId::OSC1_FREQ, osc1_freq);
        return;
    case ParamId::OSC1_GAIN:
        setChainParams (&chains[0], ParamId::OSC1_GAIN,
                        static_cast<float> (params->getParam<double> (ParamId::OSC1_GAIN)));
        return;

    case ParamId::OSC2_WAVETYPE:
        setChainParams (&chains[1], ParamId::OSC2_WAVETYPE,
                        static_cast<WaveType> (params->getParam<int> (ParamId::OSC2_WAVETYPE)));
        return;
    case ParamId::OSC2_FREQ:
        osc2_freq = params->getParam<double> (ParamId::OSC2_FREQ);
        setChainParams (&chains[1], ParamId::OSC2_FREQ, osc2_freq);
        return;
    case ParamId::OSC2_GAIN:
        setChainParams (&chains[1], ParamId::OSC2_GAIN,
                        static_cast<float> (params->getParam<double> (ParamId::OSC2_GAIN)));
        return;

    // // LFO
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

void MainComponent::initParameters()
{

    std::vector<std::unique_ptr<ParameterBase>> parameters;

    // Master Gain
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{0.0, 1.0}, 1, Def_param_val.master_gain,
                                                             ParamId::MASTER, "Master", "%"));

    // Channels Gain
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{-100.0, 0.0}, 1, Def_param_val.chans_gain,
                                                             ParamId::CHAN1_GAIN, "ch1 Gain", "dB"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{-100.0, 0.0}, 1, Def_param_val.chans_gain,
                                                             ParamId::CHAN2_GAIN, "ch2 Gain", "dB"));

    // OSC
    parameters.push_back (
        std::make_unique<ChoiceParameter> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                           Def_param_val.osc_wavetype, ParamId::OSC1_WAVETYPE, "OSC1 Wave type"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{0.0, 22000.0}, 0.4, Def_param_val.osc_freq,
                                                             ParamId::OSC1_FREQ, "Freq", "Hz"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{-100.0, 10.0}, 3.0, Def_param_val.osc_gain,
                                                             ParamId::OSC1_GAIN, "Gain", "dB"));

    parameters.push_back (
        std::make_unique<ChoiceParameter> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                           WaveType::SQR, ParamId::OSC2_WAVETYPE, "OSC2 Wave type"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{0.0, 22000.0}, 0.4, Def_param_val.osc_freq,
                                                             ParamId::OSC2_FREQ, "Freq", "Hz"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{-100.0, 10.0}, 3.0, Def_param_val.osc_gain,
                                                             ParamId::OSC2_GAIN, "Gain", "dB"));

    // LFO
    parameters.push_back (
        std::make_unique<ChoiceParameter> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                           Def_param_val.lfo_wavetype, ParamId::LFO1_WAVETYPE, "LFO1 Wave type"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{0.0, 70.0}, 0.6, Def_param_val.lfo_freq,
                                                             ParamId::LFO1_FREQ, "Freq", "Hz"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{0.0, 1.0}, 0.3, Def_param_val.lfo_gain,
                                                             ParamId::LFO1_GAIN, "Gain"));

    parameters.push_back (
        std::make_unique<ChoiceParameter> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                           Def_param_val.lfo_wavetype, ParamId::LFO2_WAVETYPE, "LFO2 Wave type"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{0.0, 70.0}, 0.6, Def_param_val.lfo_freq,
                                                             ParamId::LFO2_FREQ, "Freq", "Hz"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range{0.0, 1.0}, 0.3, Def_param_val.lfo_gain,
                                                             ParamId::LFO2_GAIN, "Gain"));

    for (auto& p : parameters)
        p->addChangeListener (this);

    params = std::make_unique<ParametersComponent> (parameters);
    addAndMakeVisible (params.get());
    resized();
}

void MainComponent::setDefaultParameterValues()
{
    // Master gain
    setChainParams (nullptr, ParamId::MASTER, Def_param_val.master_gain);

    // Channels gain
    setChainParams (&chains[0], ParamId::CHAN1_GAIN, Def_param_val.chans_gain);
    setChainParams (&chains[1], ParamId::CHAN2_GAIN, Def_param_val.chans_gain);

    // OSC
    setChainParams (&chains[0], ParamId::OSC1_WAVETYPE, Def_param_val.osc_wavetype);
    setChainParams (&chains[0], ParamId::OSC1_FREQ, Def_param_val.osc_freq);
    setChainParams (&chains[0], ParamId::OSC1_GAIN, Def_param_val.osc_gain);

    setChainParams (&chains[1], ParamId::OSC2_WAVETYPE, WaveType::SQR);
    setChainParams (&chains[1], ParamId::OSC2_FREQ, Def_param_val.osc_freq);
    setChainParams (&chains[1], ParamId::OSC2_GAIN, Def_param_val.osc_gain);

    // LFO
    setChainParams (&chains[0], ParamId::LFO1_WAVETYPE, Def_param_val.lfo_wavetype);
    setChainParams (&chains[0], ParamId::LFO1_FREQ, Def_param_val.lfo_freq);
    setChainParams (&chains[0], ParamId::LFO1_GAIN, Def_param_val.lfo_gain);

    setChainParams (&chains[1], ParamId::LFO2_WAVETYPE, Def_param_val.lfo_wavetype);
    setChainParams (&chains[1], ParamId::LFO2_FREQ, Def_param_val.lfo_freq);
    setChainParams (&chains[1], ParamId::LFO2_GAIN, Def_param_val.lfo_gain);
}

template <typename T, typename Func, typename... O>
void MainComponent::setChainParams (T val, Func f, O*... obj)
{
    /*
    execute f on the obj with the argument val
        Args:
        val = argument for f
        f = function reference
        obj = an instance of the object to perform the function on
    */
    (..., (obj->*f) (val));
}

template <typename T>
void MainComponent::setChainParams (StereoChain* chain, ParamId idx, T val)
{
    switch (idx)
    {
    case ParamId::MASTER:
        for (auto& c : chains)
        {
            c.first->get<ProcIdx::masterGainIdx>().setGainLinear (val);
            c.second->get<ProcIdx::masterGainIdx>().setGainLinear (val);
        }
        break;

    case ParamId::CHAN1_GAIN:
    case ParamId::CHAN2_GAIN:
        chain->first->get<ProcIdx::chanGainIdx>().setGainDecibels (val);
        chain->second->get<ProcIdx::chanGainIdx>().setGainDecibels (val);
        break;

    case ParamId::OSC1_WAVETYPE:
    case ParamId::OSC2_WAVETYPE:
        chain->first->get<ProcIdx::oscIdx>().setWaveType (static_cast<WaveType> (val));
        chain->second->get<ProcIdx::oscIdx>().setWaveType (static_cast<WaveType> (val));
        break;

    case ParamId::OSC1_FREQ:
    case ParamId::OSC2_FREQ:
        chain->first->get<ProcIdx::oscIdx>().setFrequency (val);
        chain->second->get<ProcIdx::oscIdx>().setFrequency (val);
        break;

    case ParamId::OSC1_GAIN:
    case ParamId::OSC2_GAIN:
        chain->first->get<ProcIdx::oscIdx>().setLevel (val);
        chain->second->get<ProcIdx::oscIdx>().setLevel (val);
        break;
    }
}

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
