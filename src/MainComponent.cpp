#include "MainComponent.h"

// the linker need an explicit definition for the static struct
MainComponent::ControlLimits MainComponent::ctl_limits;
//==============================================================================
MainComponent::MainComponent (juce::ValueTree root)
    : state (root), gui_state (root.createCopy()), def_state (root.createCopy())
// : adsc (deviceManager, 0, NUM_INPUT_CHANNELS, 0, NUM_OUTPUT_CHANNELS, false, false, true, false)
{
    // DBG ("begin constractor");
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
    initGuiControls (gui_state);

    startTimer (500);

    setSize (ctlComp.get()->getWidthNeeded() + 10, ctlComp.get()->getHeightNeeded() + 10 + 20);
    // DBG ("end constractor");
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // DBG ("begin prepareToPlay");
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
    // DBG ("end prepareToPlay");
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    juce::dsp::AudioBlock<float> audioBlock{*bufferToFill.buffer};

    for (int i = 0; i <= bufferToFill.buffer->getNumChannels() / 2; i += 2)
        audio_blocks.push_back (
            std::make_pair (audioBlock.getSingleChannelBlock (i), audioBlock.getSingleChannelBlock (i + 1)));

    float lfo1_range = max_osc_freq - static_cast<float> (getParamValue (IDs::LFO, IDs::LFO1, IDs::freq));
    float lfo2_range = max_osc_freq - static_cast<float> (getParamValue (IDs::LFO, IDs::LFO2, IDs::freq));

    // process by sample
    for (auto samp = 0; samp < bufferToFill.numSamples; ++samp)
    {
        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;

            auto lfoOut1 = lfo1.processSample (0.0f);
            auto osc1_freq = static_cast<double> (getParamValue (IDs::OSC, IDs::OSC1, IDs::freq));
            auto lfo1_gain = static_cast<double> (getParamValue (IDs::LFO, IDs::LFO1, IDs::gain));
            float mod1 = juce::jmap (lfoOut1, -1.0f, 1.0f, static_cast<float> (osc1_freq), lfo1_range);
            setChainParams (&chains[0], IDs::OSC, IDs::freq, osc1_freq + mod1 * lfo1_gain);

            auto lfoOut2 = lfo2.processSample (0.0f);
            auto osc2_freq = static_cast<double> (getParamValue (IDs::OSC, IDs::OSC2, IDs::freq));
            auto lfo2_gain = static_cast<double> (getParamValue (IDs::LFO, IDs::LFO2, IDs::gain));
            float mod2 = juce::jmap (lfoOut2, -1.0f, 1.0f, static_cast<float> (osc2_freq), lfo2_range);
            setChainParams (&chains[1], IDs::OSC, IDs::freq, osc2_freq + mod2 * lfo2_gain);
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

void MainComponent::timerCallback()
{
    undoManager.beginNewTransaction();
}

juce::var MainComponent::getParamValue (const juce::Identifier& parent, const juce::Identifier& node,
                                        const juce::Identifier& propertie)
{

    return state.getChildWithName (parent).getChildWithName (node).getProperty (propertie);
}

template <typename T>
void MainComponent::setParamValue (const juce::Identifier& parent, const juce::Identifier& node,
                                   const juce::Identifier& propertie, T val)
{
    state.getChildWithName (parent).getChildWithName (node).setProperty (propertie, val, nullptr);
}

juce::var MainComponent::getStateParamValue (const juce::ValueTree& v, const juce::Identifier& parent,
                                             const juce::Identifier& node, const juce::Identifier& propertie)
{
    return v.getChildWithName (parent).getChildWithName (node).getProperty (propertie);
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    // TODO: smooth osc freq change

    // DBG ("changeListenerCallback");
    auto comp = dynamic_cast<BaseComp*> (source);
    auto comp_state = comp->getState();
    auto comp_id = comp->id;
    auto prop = comp->prop;

    if (comp_id == IDs::MASTER)
    {
        setParamValue (IDs::OUTPUT_GAIN, comp_id, prop, comp_state[prop]);
        setChainParams (nullptr, comp_id, prop, getParamValue (IDs::OUTPUT_GAIN, comp_id, prop));
        return;
    }

    if (comp_id == IDs::CHAN1 || comp_id == IDs::CHAN2)
    {
        const auto& chain = comp_id == IDs::CHAN1 ? &chains[0] : &chains[1];
        setParamValue (IDs::OUTPUT_GAIN, comp_id, prop, comp_state[prop]);
        setChainParams (chain, comp_id, prop, getParamValue (IDs::OUTPUT_GAIN, comp_id, prop));
        return;
    }

    if (comp_id == IDs::OSC1 || comp_id == IDs::OSC2)
    {
        auto comp_type = IDs::OSC;
        const auto& chain = comp_id == IDs::OSC1 ? &chains[0] : &chains[1];

        if (prop == IDs::wavetype)
        {
            setParamValue (comp_type, comp_id, prop, comp_state[prop]);
            setChainParams (chain, comp_type, prop, getParamValue (comp_type, comp_id, prop));
            return;
        }

        if (prop == IDs::freq)
        {
            setParamValue (comp_type, comp_id, prop, comp_state[prop]);
            setChainParams (chain, comp_type, prop, getParamValue (comp_type, comp_id, prop));
            return;
        }

        if (prop == IDs::gain)
        {
            setParamValue (comp_type, comp_id, prop, comp_state[prop]);
            setChainParams (chain, comp_type, prop, getParamValue (comp_type, comp_id, prop));
            return;
        }

        if (prop == IDs::fm_freq)
        {
            setParamValue (comp_type, comp_id, prop, comp_state[prop]);
            setChainParams (chain, comp_type, prop, getParamValue (comp_type, comp_id, prop));
            return;
        }

        if (prop == IDs::fm_depth)
        {
            setParamValue (comp_type, comp_id, prop, comp_state[prop]);
            setChainParams (chain, comp_type, prop, getParamValue (comp_type, comp_id, prop));
            return;
        }
    }

    if (comp_id == IDs::LFO1 || comp_id == IDs::LFO2)
    {
        auto comp_type = IDs::LFO;
        auto& lfo = comp_id == IDs::LFO1 ? lfo1 : lfo2;

        if (prop == IDs::wavetype)
        {
            setParamValue (comp_type, comp_id, prop, comp_state[prop]);
            lfo.setWaveType (static_cast<WaveType> ((int)getParamValue (comp_type, comp_id, prop)));
            return;
        }

        if (prop == IDs::freq)
        {
            setParamValue (comp_type, comp_id, prop, comp_state[prop]);
            lfo.setFrequency (getParamValue (comp_type, comp_id, prop));
            return;
        }

        if (prop == IDs::gain)
        {
            setParamValue (comp_type, comp_id, prop, comp_state[prop]);
            lfo.setGainLinear (getParamValue (comp_type, comp_id, prop));
            return;
        }
    }

    if (comp_id == IDs::DELAY1 || comp_id == IDs::DELAY2)
    {
        auto comp_type = IDs::DELAY;
        const auto& chain = comp_id == IDs::DELAY1 ? &chains[0] : &chains[1];

        if (prop == IDs::mix)
        {
            setParamValue (comp_type, comp_id, prop, comp_state[prop]);
            setChainParams (chain, comp_type, prop, getParamValue (comp_type, comp_id, prop));
            return;
        }
    }
    // DBG ("end changeListenerCallback");
}

void MainComponent::initGuiControls (juce::ValueTree& v)
{

    // DBG ("begin initGuiControls");
    // DBG ("v validation: " << (v.isValid() == true ? "true" : "false"));
    std::vector<std::unique_ptr<BaseComp>> ctl;

    // master gain
    auto master_gain_node = v.getChildWithName (IDs::OUTPUT_GAIN).getChildWithName (IDs::MASTER);
    ctl.push_back (std::make_unique<SliderComp> (master_gain_node, IDs::gain, undoManager, "Master",
                                                 juce::Range{ctl_limits.master_min, ctl_limits.master_max}, 1,
                                                 master_gain_node[IDs::gain], "%"));

    // channels gain
    auto chan1_gain_node = v.getChildWithName (IDs::OUTPUT_GAIN).getChildWithName (IDs::CHAN1);
    ctl.push_back (std::make_unique<SliderComp> (chan1_gain_node, IDs::gain, undoManager, "ch1 gain",
                                                 juce::Range{ctl_limits.chan_min, ctl_limits.chan_max}, 1,
                                                 chan1_gain_node[IDs::gain], "dB"));

    auto chan2_gain_node = v.getChildWithName (IDs::OUTPUT_GAIN).getChildWithName (IDs::CHAN2);
    ctl.push_back (std::make_unique<SliderComp> (chan2_gain_node, IDs::gain, undoManager, "ch2 gain",
                                                 juce::Range{ctl_limits.chan_min, ctl_limits.chan_max}, 1,
                                                 chan2_gain_node[IDs::gain], "dB"));

    // OSC
    auto osc1_node = v.getChildWithName (IDs::OSC).getChildWithName (IDs::OSC1);
    ctl.push_back (std::make_unique<ChoiceComp> (osc1_node, IDs::wavetype, undoManager, "OSC1 Wave type",
                                                 juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                                 osc1_node[IDs::wavetype]));

    ctl.push_back (std::make_unique<SliderComp> (osc1_node, IDs::freq, undoManager, "Freq",
                                                 juce::Range{ctl_limits.osc_freq_min, ctl_limits.osc_freq_max}, 0.4,
                                                 osc1_node[IDs::freq], "Hz"));

    ctl.push_back (std::make_unique<SliderComp> (osc1_node, IDs::gain, undoManager, "Gain",
                                                 juce::Range{ctl_limits.osc_gain_min, ctl_limits.osc_gain_max}, 3.0,
                                                 osc1_node[IDs::gain], "dB"));

    ctl.push_back (std::make_unique<SliderComp> (osc1_node, IDs::fm_freq, undoManager, "FM Freq",
                                                 juce::Range{ctl_limits.osc_fm_freq_min, ctl_limits.osc_fm_freq_max}, 1,
                                                 osc1_node[IDs::fm_freq], "Hz"));

    ctl.push_back (std::make_unique<SliderComp> (osc1_node, IDs::fm_depth, undoManager, "FM Depth",
                                                 juce::Range{ctl_limits.osc_fm_depth_min, ctl_limits.osc_fm_depth_max},
                                                 0.4, osc1_node[IDs::fm_depth]));

    auto osc2_node = v.getChildWithName (IDs::OSC).getChildWithName (IDs::OSC2);
    ctl.push_back (std::make_unique<ChoiceComp> (osc2_node, IDs::wavetype, undoManager, "OSC2 Wave type",
                                                 juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                                 osc2_node[IDs::wavetype]));

    ctl.push_back (std::make_unique<SliderComp> (osc2_node, IDs::freq, undoManager, "Freq",
                                                 juce::Range{ctl_limits.osc_freq_min, ctl_limits.osc_freq_max}, 0.4,
                                                 osc2_node[IDs::freq], "Hz"));

    ctl.push_back (std::make_unique<SliderComp> (osc2_node, IDs::gain, undoManager, "Gain",
                                                 juce::Range{ctl_limits.osc_gain_min, ctl_limits.osc_gain_max}, 3.0,
                                                 osc2_node[IDs::gain], "dB"));

    ctl.push_back (std::make_unique<SliderComp> (osc2_node, IDs::fm_freq, undoManager, "FM Freq",
                                                 juce::Range{ctl_limits.osc_fm_freq_min, ctl_limits.osc_fm_freq_max}, 1,
                                                 osc2_node[IDs::fm_freq], "Hz"));

    ctl.push_back (std::make_unique<SliderComp> (osc2_node, IDs::fm_depth, undoManager, "FM Depth",
                                                 juce::Range{ctl_limits.osc_fm_depth_min, ctl_limits.osc_fm_depth_max},
                                                 0.4, osc2_node[IDs::fm_depth]));

    // LFO
    auto lfo1_node = v.getChildWithName (IDs::LFO).getChildWithName (IDs::LFO1);
    ctl.push_back (std::make_unique<ChoiceComp> (lfo1_node, IDs::wavetype, undoManager, "LFO1 Wave type",
                                                 juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                                 lfo1_node[IDs::wavetype]));

    ctl.push_back (std::make_unique<SliderComp> (lfo1_node, IDs::freq, undoManager, "Freq",
                                                 juce::Range{ctl_limits.lfo_freq_min, ctl_limits.lfo_freq_max}, 0.6,
                                                 lfo1_node[IDs::freq], "Hz"));

    ctl.push_back (std::make_unique<SliderComp> (lfo1_node, IDs::gain, undoManager, "Gain",
                                                 juce::Range{ctl_limits.lfo_gain_min, ctl_limits.lfo_gain_max}, 1,
                                                 lfo1_node[IDs::gain]));

    auto lfo2_node = v.getChildWithName (IDs::LFO).getChildWithName (IDs::LFO2);
    ctl.push_back (std::make_unique<ChoiceComp> (lfo2_node, IDs::wavetype, undoManager, "LFO2 Wave type",
                                                 juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"},
                                                 lfo2_node[IDs::wavetype]));

    ctl.push_back (std::make_unique<SliderComp> (lfo2_node, IDs::freq, undoManager, "Freq",
                                                 juce::Range{ctl_limits.lfo_freq_min, ctl_limits.lfo_freq_max}, 0.6,
                                                 lfo2_node[IDs::freq], "Hz"));

    ctl.push_back (std::make_unique<SliderComp> (lfo2_node, IDs::gain, undoManager, "Gain",
                                                 juce::Range{ctl_limits.lfo_gain_min, ctl_limits.lfo_gain_max}, 0.3,
                                                 lfo2_node[IDs::gain]));

    // delay
    auto delay1_node = v.getChildWithName (IDs::DELAY).getChildWithName (IDs::DELAY1);
    ctl.push_back (std::make_unique<SliderComp> (delay1_node, IDs::mix, undoManager, "Dry/wet",
                                                 juce::Range{ctl_limits.delay_mix_min, ctl_limits.delay_mix_max}, 1,
                                                 delay1_node[IDs::mix]));

    auto delay2_node = v.getChildWithName (IDs::DELAY).getChildWithName (IDs::DELAY2);
    ctl.push_back (std::make_unique<SliderComp> (delay2_node, IDs::mix, undoManager, "Dry/wet",
                                                 juce::Range{ctl_limits.delay_mix_min, ctl_limits.delay_mix_max}, 1,
                                                 delay2_node[IDs::mix]));

    for (auto& p : ctl)
        p->addChangeListener (this);

    ctlComp = std::make_unique<ControlComponent> (ctl);
    addAndMakeVisible (ctlComp.get());

    btns.push_back (std::make_unique<juce::TextButton> ("Magic"));
    btns.back()->onClick = [this] { generateRandomParameters(); };

    btns.push_back (std::make_unique<juce::TextButton> ("Undo"));
    btns.back()->onClick = [this] { undoManager.undo(); };

    btns.push_back (std::make_unique<juce::TextButton> ("Redo"));
    btns.back()->onClick = [this] { undoManager.redo(); };

    for (auto& b : btns)
        addAndMakeVisible (b.get());

    // resized();
    // DBG ("end initGuiControls");
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
void MainComponent::setChainParams (StereoChain* chain, juce::Identifier comp_id, juce::Identifier propertie, T val)
{
    if (comp_id == IDs::MASTER)
    {
        for (auto& c : chains)
        {
            c.first->get<ProcIdx::masterGainIdx>().setGainLinear (val);
            c.second->get<ProcIdx::masterGainIdx>().setGainLinear (val);
        }
        return;
    }

    if (comp_id == IDs::CHAN1 || comp_id == IDs::CHAN2)
    {
        chain->first->get<ProcIdx::chanGainIdx>().setGainDecibels (val);
        chain->second->get<ProcIdx::chanGainIdx>().setGainDecibels (val);
        return;
    }

    if (comp_id == IDs::OSC)
    {
        if (propertie == IDs::wavetype)
        {
            chain->first->get<ProcIdx::oscIdx>().setWaveType (static_cast<WaveType> ((int)val));
            chain->second->get<ProcIdx::oscIdx>().setWaveType (static_cast<WaveType> ((int)val));
            return;
        }

        if (propertie == IDs::freq)
        {
            chain->first->get<ProcIdx::oscIdx>().setFrequency (val);
            chain->second->get<ProcIdx::oscIdx>().setFrequency (val);
            return;
        }

        if (propertie == IDs::gain)
        {
            chain->first->get<ProcIdx::oscIdx>().setGainDecibels (val);
            chain->second->get<ProcIdx::oscIdx>().setGainDecibels (val);
            return;
        }

        if (propertie == IDs::fm_freq)
        {
            chain->first->get<ProcIdx::oscIdx>().setFmFreq (val);
            chain->second->get<ProcIdx::oscIdx>().setFmFreq (val);
            return;
        }

        if (propertie == IDs::fm_depth)
        {
            chain->first->get<ProcIdx::oscIdx>().setFmDepth (val);
            chain->second->get<ProcIdx::oscIdx>().setFmDepth (val);
            return;
        }
    }

    if (comp_id == IDs::DELAY)
    {
        chain->first->get<ProcIdx::delIdx>().setWetLevel (val);
        chain->second->get<ProcIdx::delIdx>().setWetLevel (val);
        return;
    }
}

void MainComponent::setDefaultParameterValues()
{
    // DBG ("begin setDefaultParameterValues");
    float master_gain = static_cast<float> (getStateParamValue (def_state, IDs::OUTPUT_GAIN, IDs::MASTER, IDs::gain));
    float chan_gain = static_cast<float> (getStateParamValue (def_state, IDs::OUTPUT_GAIN, IDs::CHAN1, IDs::gain));

    WaveType osc_wave = static_cast<WaveType> ((int)getStateParamValue (def_state, IDs::OSC, IDs::OSC1, IDs::wavetype));
    double osc_freq = getStateParamValue (def_state, IDs::OSC, IDs::OSC1, IDs::freq);
    float osc_gain = getStateParamValue (def_state, IDs::OSC, IDs::OSC1, IDs::gain);
    double osc_fm_freq = getStateParamValue (def_state, IDs::OSC, IDs::OSC1, IDs::fm_freq);
    float osc_fm_depth = getStateParamValue (def_state, IDs::OSC, IDs::OSC1, IDs::fm_depth);

    WaveType lfo_wave = static_cast<WaveType> ((int)getStateParamValue (def_state, IDs::LFO, IDs::LFO1, IDs::wavetype));
    double lfo_freq = getStateParamValue (def_state, IDs::LFO, IDs::LFO1, IDs::freq);
    float lfo_gain = getStateParamValue (def_state, IDs::LFO, IDs::LFO1, IDs::gain);

    float del_mix = getStateParamValue (def_state, IDs::DELAY, IDs::DELAY1, IDs::gain);

    // master gain
    setChainParams (nullptr, IDs::MASTER, juce::Identifier(), master_gain);
    // channels gain
    setChainParams (&chains[0], IDs::CHAN1, juce::Identifier(), chan_gain);
    setChainParams (&chains[1], IDs::CHAN2, juce::Identifier(), chan_gain);

    for (size_t i = 0; i < chains.size(); i++)
    {
        // OSC
        setChainParams (&chains[i], IDs::OSC, IDs::wavetype, osc_wave);
        setChainParams (&chains[i], IDs::OSC, IDs::freq, osc_freq);
        setChainParams (&chains[i], IDs::OSC, IDs::gain, osc_gain);
        setChainParams (&chains[i], IDs::OSC, IDs::fm_freq, osc_fm_freq);
        setChainParams (&chains[i], IDs::OSC, IDs::fm_depth, osc_fm_depth);
        // LFO
        setChainParams (&chains[i], IDs::LFO, IDs::wavetype, lfo_wave);
        setChainParams (&chains[i], IDs::LFO, IDs::freq, lfo_freq);
        setChainParams (&chains[i], IDs::LFO, IDs::gain, lfo_gain);
        // delay
        setChainParams (&chains[i], IDs::DELAY, IDs::mix, del_mix);
    }
    // DBG ("end setDefaultParameterValues");
}

void MainComponent::generateRandomParameters()
{
    // std::random_device rd;
    // std::mt19937 gen (rd());

    // std::uniform_int_distribution<> osc_type (ctl_limits.osc_waveType_min, 3);
    // std::uniform_real_distribution<> osc_freq (ctl_limits.osc_freq_min, ctl_limits.osc_freq_max);
    // std::uniform_real_distribution<> osc_fm_freq (ctl_limits.osc_fm_freq_min, ctl_limits.osc_fm_freq_max);
    // std::uniform_real_distribution<> osc_fm_depth (ctl_limits.osc_fm_depth_min, ctl_limits.osc_fm_depth_max);

    // std::uniform_int_distribution<> lfo_type (ctl_limits.lfo_waveType_min, 3);
    // std::uniform_real_distribution<> lfo_freq (ctl_limits.lfo_freq_min, ctl_limits.lfo_freq_max);
    // std::uniform_real_distribution<> lfo_gain (ctl_limits.lfo_gain_min, ctl_limits.lfo_gain_max);

    // ChainsParameters rand;

    // for (int i = 0; i < chains.size(); i++)
    // {
    //     rand[i].osc_wavetype = static_cast<WaveType> (osc_type (gen));
    //     rand[i].osc_freq = osc_freq (gen);
    //     rand[i].osc_fm_freq = osc_fm_freq (gen);
    //     rand[i].osc_fm_depth = osc_fm_depth (gen);

    //     rand[i].lfo_wavetype = static_cast<WaveType> (lfo_type (gen));

    //     rand[i].lfo_freq = lfo_freq (gen);

    //     rand[i].lfo_gain = osc_freq (gen);

    //     setChainParams (&chains[i], ParamId::OSC1_WAVETYPE, rand[i].osc_wavetype);
    //     setChainParams (&chains[i], ParamId::OSC1_FREQ, rand[i].osc_freq);
    //     setChainParams (&chains[i], ParamId::OSC1_FM_FREQ, rand[i].osc_fm_freq);
    //     setChainParams (&chains[i], ParamId::OSC1_FM_DEPTH, rand[i].osc_fm_depth);

    //     setChainParams (&chains[i], ParamId::LFO1_WAVETYPE, rand[i].lfo_wavetype);
    //     setChainParams (&chains[i], ParamId::LFO1_FREQ, rand[i].lfo_freq);
    //     setChainParams (&chains[i], ParamId::LFO1_GAIN, rand[i].lfo_gain);
    // }
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
    // DBG (bounds.getX() << " " << bounds.getY() << " " << bounds.getWidth() << " " << bounds.getHeight());
    // DBG (ctlComp->getBounds().getX() << " " << ctlComp->getBounds().getY());
    int i = bounds.getX();
    for (auto& b : btns)
    {
        b->setBounds (i + 5, bounds.getY() + 5, b->getBestWidthForHeight (20), 20);
        i += b->getBestWidthForHeight (20) + 6;
    }

    if (ctlComp.get() != nullptr)
        ctlComp->setBounds (5, 25, ctlComp->getWidthNeeded(), ctlComp->getHeightNeeded());
    // ctlComp->setBounds (bounds.removeFromTop (ctlComp->getHeightNeeded()).reduced (20, 0));
}
