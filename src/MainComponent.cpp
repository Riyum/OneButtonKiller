#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent (juce::ValueTree root) : state (root), gui_state (root.createCopy())
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

    initGuiControls (gui_state);

    startTimer (500);

    setSize (ctlComp.get()->getWidthNeeded() + 60, ctlComp.get()->getHeightNeeded() + 60);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
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
    auto comp = dynamic_cast<BaseComp*> (source);
    auto comp_state = comp->getState();
    auto comp_type = comp_state.getParent().getType();
    auto comp_id = comp_state.getType();
    auto prop = comp->prop;

    if (comp_type == IDs::OUTPUT_GAIN)
    {
        setParamValue (comp_type, comp_id, prop, comp_state[prop]);
        if (comp_id == IDs::MASTER)
        {
            setChainParams (nullptr, comp_id, prop, getParamValue (comp_type, comp_id, prop));
            return;
        }
        const auto& chain = comp_id == IDs::CHAN1 ? &chains[0] : &chains[1];
        setChainParams (chain, comp_id, prop, getParamValue (comp_type, comp_id, prop));
        return;
    }

    if (comp_type == IDs::OSC)
    {
        setParamValue (comp_type, comp_id, prop, comp_state[prop]);
        const auto& chain = comp_id == IDs::OSC1 ? &chains[0] : &chains[1];
        setChainParams (chain, comp_type, prop, getParamValue (comp_type, comp_id, prop));
        return;
    }

    if (comp_type == IDs::LFO)
    {
        setParamValue (comp_type, comp_id, prop, comp_state[prop]);
        auto& lfo = comp_id == IDs::LFO1 ? lfo1 : lfo2;

        if (prop == IDs::wavetype)
        {
            lfo.setWaveType (static_cast<WaveType> ((int)getParamValue (comp_type, comp_id, prop)));
            return;
        }

        if (prop == IDs::freq)
        {
            lfo.setFrequency (getParamValue (comp_type, comp_id, prop));
            return;
        }

        if (prop == IDs::gain)
        {
            lfo.setGainLinear (getParamValue (comp_type, comp_id, prop));
            return;
        }
    }

    if (comp_type == IDs::DELAY)
    {
        setParamValue (comp_type, comp_id, prop, comp_state[prop]);
        const auto& chain = comp_id == IDs::DELAY1 ? &chains[0] : &chains[1];
        setChainParams (chain, comp_type, prop, getParamValue (comp_type, comp_id, prop));
    }
}

void MainComponent::initGuiControls (juce::ValueTree& v)
{

    std::vector<std::unique_ptr<BaseComp>> ctl;

    // master gain
    auto master_gain_node = v.getChildWithName (IDs::OUTPUT_GAIN).getChildWithName (IDs::MASTER);
    ctl.push_back (std::make_unique<LabelComp> (master_gain_node, IDs::ROOT, undoManager, "Volume"));
    ctl.push_back (std::make_unique<SliderComp> (master_gain_node, IDs::gain, undoManager, "Master",
                                                 juce::Range{ctl_limits.master_min, ctl_limits.master_max}, 1,
                                                 master_gain_node[IDs::gain], "%"));

    // channels gain
    auto chan1_gain_node = v.getChildWithName (IDs::OUTPUT_GAIN).getChildWithName (IDs::CHAN1);
    ctl.push_back (std::make_unique<SliderComp> (chan1_gain_node, IDs::gain, undoManager, "Ch1",
                                                 juce::Range{ctl_limits.chan_min, ctl_limits.chan_max}, 1,
                                                 chan1_gain_node[IDs::gain], "dB"));

    auto chan2_gain_node = v.getChildWithName (IDs::OUTPUT_GAIN).getChildWithName (IDs::CHAN2);
    ctl.push_back (std::make_unique<SliderComp> (chan2_gain_node, IDs::gain, undoManager, "Ch2",
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
                                                 juce::Range{ctl_limits.lfo_gain_min, ctl_limits.lfo_gain_max}, 1,
                                                 lfo2_node[IDs::gain]));

    // delay
    auto delay1_node = v.getChildWithName (IDs::DELAY).getChildWithName (IDs::DELAY1);
    ctl.push_back (std::make_unique<LabelComp> (delay1_node, IDs::ROOT, undoManager, "Delay 1"));

    ctl.push_back (std::make_unique<SliderComp> (delay1_node, IDs::mix, undoManager, "Dry/wet",
                                                 juce::Range{ctl_limits.delay_mix_min, ctl_limits.delay_mix_max}, 1,
                                                 delay1_node[IDs::mix]));

    ctl.push_back (std::make_unique<SliderComp> (delay1_node, IDs::time, undoManager, "Time",
                                                 juce::Range{ctl_limits.delay_time_min, ctl_limits.delay_time_max}, 1,
                                                 delay1_node[IDs::time]));

    ctl.push_back (std::make_unique<SliderComp> (
        delay1_node, IDs::feedback, undoManager, "Feedback",
        juce::Range{ctl_limits.delay_feedback_min, ctl_limits.delay_feedback_max}, 1, delay1_node[IDs::feedback]));

    auto delay2_node = v.getChildWithName (IDs::DELAY).getChildWithName (IDs::DELAY2);
    ctl.push_back (std::make_unique<LabelComp> (delay2_node, IDs::ROOT, undoManager, "Delay 2"));
    ctl.push_back (std::make_unique<SliderComp> (delay2_node, IDs::mix, undoManager, "Dry/wet",
                                                 juce::Range{ctl_limits.delay_mix_min, ctl_limits.delay_mix_max}, 1,
                                                 delay2_node[IDs::mix]));

    ctl.push_back (std::make_unique<SliderComp> (delay2_node, IDs::time, undoManager, "Time",
                                                 juce::Range{ctl_limits.delay_time_min, ctl_limits.delay_time_max}, 1,
                                                 delay2_node[IDs::time]));

    ctl.push_back (std::make_unique<SliderComp> (
        delay2_node, IDs::feedback, undoManager, "Feedback",
        juce::Range{ctl_limits.delay_feedback_min, ctl_limits.delay_feedback_max}, 1, delay2_node[IDs::feedback]));

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
}

template <typename T, typename Func, typename... O>
void MainComponent::setChainParams (T val, Func f, O*... obj)
{
    (..., (obj->*f) (val));
}

template <typename T>
void MainComponent::setChainParams (StereoChain* chain, const juce::Identifier& comp_id,
                                    const juce::Identifier& propertie, T val)
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
        if (propertie == IDs::mix)
        {
            chain->first->get<ProcIdx::delIdx>().setWetLevel (val);
            chain->second->get<ProcIdx::delIdx>().setWetLevel (val);
            return;
        }
        if (propertie == IDs::time)
        {
            chain->first->get<ProcIdx::delIdx>().setDelayTime (val);
            chain->second->get<ProcIdx::delIdx>().setDelayTime (static_cast<double> (val) + 0.2);
            return;
        }
        if (propertie == IDs::feedback)
        {
            chain->first->get<ProcIdx::delIdx>().setFeedback (val);
            chain->second->get<ProcIdx::delIdx>().setFeedback (val);
            return;
        }
    }
}

void MainComponent::setDefaultParameterValues()
{
    // master gain
    setChainParams (nullptr, IDs::MASTER, juce::Identifier(), def_param.master_gain);
    // channels gain
    setChainParams (&chains[0], IDs::CHAN1, juce::Identifier(), def_param.chan_gain);
    setChainParams (&chains[1], IDs::CHAN2, juce::Identifier(), def_param.chan_gain);

    for (size_t i = 0; i < chains.size(); i++)
    {
        // OSC
        setChainParams (&chains[i], IDs::OSC, IDs::wavetype, def_param.osc_wavetype);
        setChainParams (&chains[i], IDs::OSC, IDs::freq, def_param.osc_freq);
        setChainParams (&chains[i], IDs::OSC, IDs::gain, def_param.osc_gain);
        setChainParams (&chains[i], IDs::OSC, IDs::fm_freq, def_param.osc_fm_freq);
        setChainParams (&chains[i], IDs::OSC, IDs::fm_depth, def_param.osc_fm_depth);
        // LFO
        setChainParams (&chains[i], IDs::LFO, IDs::wavetype, def_param.lfo_wavetype);
        setChainParams (&chains[i], IDs::LFO, IDs::freq, def_param.lfo_freq);
        setChainParams (&chains[i], IDs::LFO, IDs::gain, def_param.lfo_gain);
        // delay
        setChainParams (&chains[i], IDs::DELAY, IDs::mix, def_param.del_mix);
        setChainParams (&chains[i], IDs::DELAY, IDs::time, def_param.del_time);
        setChainParams (&chains[i], IDs::DELAY, IDs::feedback, def_param.del_feedback);
    }
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
    std::uniform_real_distribution<> lfo_gain (ctl_limits.lfo_gain_min, percentageFrom (30, ctl_limits.lfo_gain_max));

    for (auto osc_parameters : gui_state.getChildWithName (IDs::OSC))
    {
        osc_parameters.setProperty (IDs::wavetype, osc_type (gen), &undoManager);
        osc_parameters.setProperty (IDs::freq, osc_freq (gen), &undoManager);
        osc_parameters.setProperty (IDs::fm_freq, osc_fm_freq (gen), &undoManager);
        osc_parameters.setProperty (IDs::fm_depth, osc_fm_depth (gen), &undoManager);
    }

    for (auto lfo_parameters : gui_state.getChildWithName (IDs::LFO))
    {
        lfo_parameters.setProperty (IDs::wavetype, lfo_type (gen), &undoManager);
        lfo_parameters.setProperty (IDs::freq, lfo_freq (gen), &undoManager);
        lfo_parameters.setProperty (IDs::gain, lfo_gain (gen), &undoManager);
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
    auto btnBounds = bounds.removeFromTop (30);

    const int btn_height = 20;
    const int gap = 3;

    for (auto&& b : btns)
    {
        b->setSize (b->getBestWidthForHeight (btn_height), btn_height);
        auto pos = btnBounds.removeFromLeft (b->getBestWidthForHeight (btn_height + gap)).getCentre();
        b->setCentrePosition (pos);
    }

    if (ctlComp.get() != nullptr)
        ctlComp->setBounds (bounds.removeFromTop (ctlComp->getHeightNeeded()).reduced (20, 5));
}
