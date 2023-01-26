#include "MainComponent.h"

// the linker need an explicit definition for the static struct
MainComponent::Control_limits MainComponent::ctl_limits;
//==============================================================================
MainComponent::MainComponent()
// : adsc (deviceManager, 0, NUM_INPUT_CHANNELS, 0, NUM_OUTPUT_CHANNELS, false, false, true, false)
{
    for (int i = 0; i < NUM_OUTPUT_CHANNELS / 2; i++)
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
        setAudioChannels (NUM_INPUT_CHANNELS, NUM_OUTPUT_CHANNELS);
    }

    // addAndMakeVisible (adsc);

    // initialise Parameters GUI component
    initParameters();

    setSize (ctlComp.get()->getWidthNeeded() + 10, ctlComp.get()->getHeightNeeded() + 10);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();

    /* since the parameters vector is private in parametersComponent
     * removing listeners from the parameters is done in the ControlComponent destructor
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

    float lfo1_range = max_osc_freq - chain_param[0].osc_freq;
    float lfo2_range = max_osc_freq - chain_param[1].osc_freq;

    // process by sample
    for (auto samp = 0; samp < bufferToFill.numSamples; ++samp)
    {
        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;

            auto lfoOut1 = lfo1.processSample (0.0f);
            float mod1 = juce::jmap (lfoOut1, -1.0f, 1.0f, static_cast<float> (chain_param[0].osc_freq), lfo1_range);
            setChainParams (&chains[0], ParamId::OSC1_FREQ, chain_param[0].osc_freq + mod1 * chain_param[0].lfo_gain);

            auto lfoOut2 = lfo2.processSample (0.0f);
            float mod2 = juce::jmap (lfoOut2, -1.0f, 1.0f, static_cast<float> (chain_param[1].osc_freq), lfo2_range);
            setChainParams (&chains[1], ParamId::OSC2_FREQ, chain_param[1].osc_freq + mod2 * chain_param[1].lfo_gain);
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

    ParamId id = static_cast<ParamId> (dynamic_cast<BaseComp*> (source)->id);

    switch (id)
    {
    // Master Gain
    case ParamId::MASTER:
        setChainParams (nullptr, ParamId::MASTER, static_cast<float> (ctlComp->getValue<double> (ParamId::MASTER)));
        return;

    // Channels Gain
    case ParamId::CHAN1_GAIN:
        setChainParams (&chains[0], ParamId::CHAN1_GAIN,
                        static_cast<float> (ctlComp->getValue<double> (ParamId::CHAN1_GAIN)));
        return;

    case ParamId::CHAN2_GAIN:
        setChainParams (&chains[1], ParamId::CHAN2_GAIN,
                        static_cast<float> (ctlComp->getValue<double> (ParamId::CHAN2_GAIN)));
        return;

    // // OSC
    case ParamId::OSC1_WAVETYPE:
        setChainParams (&chains[0], ParamId::OSC1_WAVETYPE,
                        static_cast<WaveType> (ctlComp->getValue<int> (ParamId::OSC1_WAVETYPE)));
        return;
    case ParamId::OSC1_FREQ:
        chain_param[0].osc_freq = ctlComp->getValue<double> (ParamId::OSC1_FREQ);
        setChainParams (&chains[0], ParamId::OSC1_FREQ, chain_param[0].osc_freq);
        return;
    case ParamId::OSC1_GAIN:
        setChainParams (&chains[0], ParamId::OSC1_GAIN,
                        static_cast<float> (ctlComp->getValue<double> (ParamId::OSC1_GAIN)));
        return;
    case ParamId::OSC1_FM_FREQ:
        setChainParams (&chains[0], ParamId::OSC1_FM_FREQ,
                        static_cast<float> (ctlComp->getValue<double> (ParamId::OSC1_FM_FREQ)));
        return;
    case ParamId::OSC1_FM_DEPTH:
        setChainParams (&chains[0], ParamId::OSC1_FM_DEPTH,
                        static_cast<float> (ctlComp->getValue<double> (ParamId::OSC1_FM_DEPTH)));
        return;

    case ParamId::OSC2_WAVETYPE:
        setChainParams (&chains[1], ParamId::OSC2_WAVETYPE,
                        static_cast<WaveType> (ctlComp->getValue<int> (ParamId::OSC2_WAVETYPE)));
        return;
    case ParamId::OSC2_FREQ:
        chain_param[1].osc_freq = ctlComp->getValue<double> (ParamId::OSC2_FREQ);
        setChainParams (&chains[1], ParamId::OSC2_FREQ, chain_param[1].osc_freq);
        return;
    case ParamId::OSC2_GAIN:
        setChainParams (&chains[1], ParamId::OSC2_GAIN,
                        static_cast<float> (ctlComp->getValue<double> (ParamId::OSC2_GAIN)));
        return;
    case ParamId::OSC2_FM_FREQ:
        setChainParams (&chains[1], ParamId::OSC2_FM_FREQ,
                        static_cast<float> (ctlComp->getValue<double> (ParamId::OSC2_FM_FREQ)));
        return;
    case ParamId::OSC2_FM_DEPTH:
        setChainParams (&chains[1], ParamId::OSC2_FM_DEPTH,
                        static_cast<float> (ctlComp->getValue<double> (ParamId::OSC2_FM_DEPTH)));
        return;

    // // LFO
    case ParamId::LFO1_WAVETYPE:
        lfo1.setWaveType (static_cast<WaveType> (ctlComp->getValue<int> (ParamId::LFO1_WAVETYPE)));
        return;
    case ParamId::LFO1_FREQ:
        chain_param[0].lfo_freq = ctlComp->getValue<double> (ParamId::LFO1_FREQ);
        lfo1.setFrequency (chain_param[0].lfo_freq);
        return;
    case ParamId::LFO1_GAIN:
        chain_param[0].lfo_gain = static_cast<float> (ctlComp->getValue<double> (ParamId::LFO1_GAIN));
        lfo1.setGainLinear (chain_param[0].lfo_gain);
        return;

    case ParamId::LFO2_WAVETYPE:
        lfo2.setWaveType (static_cast<WaveType> (ctlComp->getValue<int> (ParamId::LFO2_WAVETYPE)));
        return;
    case ParamId::LFO2_FREQ:
        chain_param[1].lfo_freq = ctlComp->getValue<double> (ParamId::LFO2_FREQ);
        lfo2.setFrequency (chain_param[1].lfo_freq);
        return;
    case ParamId::LFO2_GAIN:
        chain_param[1].lfo_gain = static_cast<float> (ctlComp->getValue<double> (ParamId::LFO2_GAIN));
        lfo2.setGainLinear (chain_param[1].lfo_gain);
        return;

    // Delay
    case ParamId::DEL_MIX:
        setChainParams (&chains[0], ParamId::DEL_MIX,
                        static_cast<float> (ctlComp->getValue<double> (ParamId::DEL_MIX)));
        return;
    }
}

void MainComponent::initParameters()
{

    std::vector<std::unique_ptr<BaseComp>> ctl;

    // button
    ctl.push_back (std::make_unique<BtnComp> (
        "TOUCH ME", [this] { generateRandomParameters(); }, ParamId::BTN, ""));

    // master gain
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.master_min, ctl_limits.master_max}, 1,
                                                 def_param.master_gain, ParamId::MASTER, "Master", "%"));

    // channels gain
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.chan_min, ctl_limits.chan_max}, 1,
                                                 def_param.chan_gain, ParamId::CHAN1_GAIN, "ch1 Gain", "dB"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.chan_min, ctl_limits.chan_max}, 1,
                                                 def_param.chan_gain, ParamId::CHAN2_GAIN, "ch2 Gain", "dB"));

    // OSC
    ctl.push_back (std::make_unique<ChoiceComp> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                                 def_param.osc_wavetype, ParamId::OSC1_WAVETYPE, "OSC1 Wave type"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.osc_freq_min, ctl_limits.osc_freq_max}, 0.4,
                                                 def_param.osc_freq, ParamId::OSC1_FREQ, "Freq", "Hz"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.osc_gain_min, ctl_limits.osc_gain_max}, 3.0,
                                                 def_param.osc_gain, ParamId::OSC1_GAIN, "Gain", "dB"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.osc_fm_freq_min, ctl_limits.osc_fm_freq_max}, 1,
                                                 def_param.osc_fm_freq, ParamId::OSC1_FM_FREQ, "FM Freq", "Hz"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.osc_fm_depth_min, ctl_limits.osc_fm_depth_max},
                                                 1, def_param.osc_fm_depth, ParamId::OSC1_FM_DEPTH, "Fm Depth"));

    ctl.push_back (std::make_unique<ChoiceComp> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                                 WaveType::SQR, ParamId::OSC2_WAVETYPE, "OSC2 Wave type"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.osc_freq_min, ctl_limits.osc_freq_max}, 0.4,
                                                 def_param.osc_freq, ParamId::OSC2_FREQ, "Freq", "Hz"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.osc_gain_min, ctl_limits.osc_gain_max}, 3.0,
                                                 def_param.osc_gain, ParamId::OSC2_GAIN, "Gain", "dB"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.osc_fm_freq_min, ctl_limits.osc_fm_freq_max}, 1,
                                                 def_param.osc_fm_freq, ParamId::OSC2_FM_FREQ, "FM Freq", "Hz"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.osc_fm_depth_min, ctl_limits.osc_fm_depth_max},
                                                 1, def_param.osc_fm_depth, ParamId::OSC2_FM_DEPTH, "Fm Depth"));

    // LFO
    ctl.push_back (std::make_unique<ChoiceComp> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                                 def_param.lfo_wavetype, ParamId::LFO1_WAVETYPE, "LFO1 Wave type"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.lfo_freq_min, ctl_limits.lfo_freq_max}, 0.6,
                                                 def_param.lfo_freq, ParamId::LFO1_FREQ, "Freq", "Hz"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.lfo_gain_min, ctl_limits.lfo_gain_max}, 0.3,
                                                 def_param.lfo_gain, ParamId::LFO1_GAIN, "Gain"));

    ctl.push_back (std::make_unique<ChoiceComp> (juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                                 def_param.lfo_wavetype, ParamId::LFO2_WAVETYPE, "LFO2 Wave type"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.lfo_freq_min, ctl_limits.lfo_freq_max}, 0.6,
                                                 def_param.lfo_freq, ParamId::LFO2_FREQ, "Freq", "Hz"));
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.lfo_gain_min, ctl_limits.lfo_gain_max}, 0.3,
                                                 def_param.lfo_gain, ParamId::LFO2_GAIN, "Gain"));

    // delay
    ctl.push_back (std::make_unique<SliderComp> (juce::Range{ctl_limits.delay_mix_min, ctl_limits.delay_mix_max}, 1,
                                                 def_param.lfo_freq, ParamId::DEL_MIX, "Delay"));

    for (auto& p : ctl)
        p->addChangeListener (this);

    ctlComp = std::make_unique<ControlComponent> (ctl);
    addAndMakeVisible (ctlComp.get());
    resized();
}

void MainComponent::setDefaultParameterValues()
{
    // master gain
    setChainParams (nullptr, ParamId::MASTER, def_param.master_gain);

    // channels gain
    setChainParams (&chains[0], ParamId::CHAN1_GAIN, def_param.chan_gain);
    setChainParams (&chains[1], ParamId::CHAN2_GAIN, def_param.chan_gain);

    // OSC
    setChainParams (&chains[0], ParamId::OSC1_WAVETYPE, def_param.osc_wavetype);
    setChainParams (&chains[0], ParamId::OSC1_FREQ, def_param.osc_freq);
    setChainParams (&chains[0], ParamId::OSC1_GAIN, def_param.osc_gain);
    setChainParams (&chains[0], ParamId::OSC1_FM_FREQ, def_param.osc_fm_freq);
    setChainParams (&chains[0], ParamId::OSC1_FM_DEPTH, def_param.osc_fm_depth);

    setChainParams (&chains[1], ParamId::OSC2_WAVETYPE, WaveType::SQR);
    setChainParams (&chains[1], ParamId::OSC2_FREQ, def_param.osc_freq);
    setChainParams (&chains[1], ParamId::OSC2_GAIN, def_param.osc_gain);
    setChainParams (&chains[1], ParamId::OSC2_FM_FREQ, def_param.osc_fm_freq);
    setChainParams (&chains[1], ParamId::OSC2_FM_DEPTH, def_param.osc_fm_depth);

    // LFO
    setChainParams (&chains[0], ParamId::LFO1_WAVETYPE, def_param.lfo_wavetype);
    setChainParams (&chains[0], ParamId::LFO1_FREQ, def_param.lfo_freq);
    setChainParams (&chains[0], ParamId::LFO1_GAIN, def_param.lfo_gain);

    setChainParams (&chains[1], ParamId::LFO2_WAVETYPE, def_param.lfo_wavetype);
    setChainParams (&chains[1], ParamId::LFO2_FREQ, def_param.lfo_freq);
    setChainParams (&chains[1], ParamId::LFO2_GAIN, def_param.lfo_gain);

    // delay
    setChainParams (&chains[0], ParamId::DEL_MIX, def_param.del_mix);
    setChainParams (&chains[1], ParamId::DEL_MIX, def_param.del_mix);
}

void MainComponent::generateRandomParameters()
{
    std::random_device rd;
    std::mt19937 gen (rd());

    std::uniform_int_distribution<> osc_type (ctl_limits.osc_waveType_min, 3);
    std::uniform_real_distribution<> osc_freq (ctl_limits.osc_freq_min, ctl_limits.osc_freq_max);
    std::uniform_real_distribution<> osc_fm_freq (ctl_limits.osc_fm_freq_min, ctl_limits.osc_fm_freq_max);
    std::uniform_real_distribution<> osc_fm_depth (ctl_limits.osc_fm_depth_min, ctl_limits.osc_fm_depth_max);

    std::uniform_int_distribution<> lfo_type (ctl_limits.lfo_waveType_min, 3);
    std::uniform_real_distribution<> lfo_freq (ctl_limits.lfo_freq_min, ctl_limits.lfo_freq_max);
    std::uniform_real_distribution<> lfo_gain (ctl_limits.lfo_gain_min, ctl_limits.lfo_gain_max);

    ChainsParameters rand;

    for (int i = 0; i < chains.size(); i++)
    {
        rand[i].osc_wavetype = static_cast<WaveType> (osc_type (gen));
        rand[i].osc_freq = osc_freq (gen);
        rand[i].osc_fm_freq = osc_fm_freq (gen);
        rand[i].osc_fm_depth = osc_fm_depth (gen);

        rand[i].lfo_wavetype = static_cast<WaveType> (lfo_type (gen));

        rand[i].lfo_freq = lfo_freq (gen);

        rand[i].lfo_gain = osc_freq (gen);

        setChainParams (&chains[i], ParamId::OSC1_WAVETYPE, rand[i].osc_wavetype);
        setChainParams (&chains[i], ParamId::OSC1_FREQ, rand[i].osc_freq);
        setChainParams (&chains[i], ParamId::OSC1_FM_FREQ, rand[i].osc_fm_freq);
        setChainParams (&chains[i], ParamId::OSC1_FM_DEPTH, rand[i].osc_fm_depth);

        setChainParams (&chains[i], ParamId::LFO1_WAVETYPE, rand[i].lfo_wavetype);
        setChainParams (&chains[i], ParamId::LFO1_FREQ, rand[i].lfo_freq);
        setChainParams (&chains[i], ParamId::LFO1_GAIN, rand[i].lfo_gain);
    }
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
        chain->first->get<ProcIdx::oscIdx>().setGainDecibels (val);
        chain->second->get<ProcIdx::oscIdx>().setGainDecibels (val);
        break;

    case ParamId::OSC1_FM_FREQ:
    case ParamId::OSC2_FM_FREQ:
        chain->first->get<ProcIdx::oscIdx>().setFmFreq (val);
        chain->second->get<ProcIdx::oscIdx>().setFmFreq (val);
        break;

    case ParamId::OSC1_FM_DEPTH:
    case ParamId::OSC2_FM_DEPTH:
        chain->first->get<ProcIdx::oscIdx>().setFmDepth (val);
        chain->second->get<ProcIdx::oscIdx>().setFmDepth (val);
        break;

    case ParamId::DEL_MIX:
        chain->first->get<ProcIdx::delIdx>().setWetLevel (val);
        chain->second->get<ProcIdx::delIdx>().setWetLevel (val);
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

    if (ctlComp.get() != nullptr)
        ctlComp->setBounds (bounds.removeFromTop (ctlComp->getHeightNeeded()).reduced (20, 0));
}
