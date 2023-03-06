#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent (juce::ValueTree st, juce::ValueTree selectors_st)
    : seq ([this]() { generateRandomParameters(); }), state (st), selectors_state (selectors_st), gen (rd()), rand (gen)
// adsc (deviceManager, 0, NUM_INPUT_CHANNELS, 0, NUM_OUTPUT_CHANNELS, false, false, true, false)
{
    for (size_t i = 0; i < chains.size(); i++)
    {
        chains[i] = std::make_pair (std::make_unique<Chain>(), std::make_unique<Chain>());
        lfo[i] = std::make_unique<Lfo<float>> (chains[i], i, st);
    }

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

    jassert (NUM_OUTPUT_CHANNELS == deviceManager.getAudioDeviceSetup().outputChannels.countNumberOfSetBits());

    addKeyListener (this);
    initGuiComponents (state, selectors_st);
    initBroadcasters (state, selectors_st);

    int main_comp_width = 0, main_comp_height = 0;
    for (size_t i = 0; i < osc_comp.size(); i++)
    {
        main_comp_width +=
            std::max ({getComponentWidth (osc_comp[i]), getComponentWidth (lfo_comp[i]), getComponentWidth (del_comp[i])});
    }

    main_comp_height += getComponentHeight (btn_comp) + gui_sizes.yGap_top;
    main_comp_height += getComponentHeight (output_comp) + gui_sizes.yGap_top;
    main_comp_height += getComponentHeight (osc_comp.back()) + gui_sizes.yGap_between_components;
    main_comp_height += getComponentHeight (filt_comp.back()) + gui_sizes.yGap_between_components;
    main_comp_height += getComponentHeight (lfo_comp.back()) + gui_sizes.yGap_between_components;
    main_comp_height += getComponentHeight (del_comp.back()) + gui_sizes.yGap_between_components;

    setSize (main_comp_width + 30, main_comp_height + 15);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    removeKeyListener (this);
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::dsp::ProcessSpec spec;

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlockExpected);
    spec.numChannels = 1;

    jassert (chains.size() == lfo.size());
    for (size_t i = 0; i < chains.size(); i++)
    {
        chains[i].first->prepare (spec);
        chains[i].second->prepare (spec);
        lfo[i]->prepare ({spec.sampleRate / def_params.lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});
    }

    oscOff();
    setDefaultParameterValues();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{

    juce::dsp::AudioBlock<float> audioBlock{*bufferToFill.buffer};

    for (size_t i = 0, j = 0; i < audio_blocks.size(); ++i, j += 2)
        audio_blocks[i] = std::make_pair (audioBlock.getSingleChannelBlock (j), audioBlock.getSingleChannelBlock (j + 1));

    // process by sample
    for (auto samp = 0; samp < bufferToFill.numSamples; ++samp)
    {
        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = def_params.lfoUpdateRate;
            for (size_t i = 0; i < lfo.size(); i++)
                lfo[i]->process();
        }
        --lfoUpdateCounter;
    }

    jassert (chains.size() == audio_blocks.size());
    // process by blocks
    for (size_t i = 0; i < chains.size(); i++)
    {
        chains[i].first->process (juce::dsp::ProcessContextReplacing<float> (audio_blocks[i].first));
        chains[i].second->process (juce::dsp::ProcessContextReplacing<float> (audio_blocks[i].second));
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
    for (auto& chain : chains)
    {
        chain.first->reset();
        chain.second->reset();
    }
}

bool MainComponent::keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent)
{
    juce::ignoreUnused (key, originatingComponent);
    // if (key.getKeyCode() == juce::KeyPress::returnKey)
    oscOn();
    return true;
}

bool MainComponent::keyStateChanged (bool isKeyDown, juce::Component* originatingComponent)
{
    juce::ignoreUnused (originatingComponent);
    if (!isKeyDown)
    {
        oscOff();
    }
    else
    {
        // Handle the key up event
    }
    return true;
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    auto comp = dynamic_cast<Broadcaster*> (source);
    auto comp_state = comp->getState();
    auto comp_type = comp_state.getParent().getType();
    auto comp_id = comp_state.getType();
    auto prop = comp->propertie;

    if (prop == IDs::master)
    {
        setParam (0, IDs::OUTPUT_GAIN, prop, comp_state[prop]);
        return;
    }

    if (comp_type == IDs::SEQUENCER)
    {
        setParam (0, comp_type, prop, comp_state[prop]);
        return;
    }

    size_t idx = static_cast<size_t> (comp_state.getParent().indexOf (comp_state));

    if (comp_type == IDs::OUTPUT_GAIN || comp_type == IDs::OSC || comp_type == IDs::LFO || comp_type == IDs::FILT
        || comp_type == IDs::DELAY)
    {
        setParam (idx, comp_type, prop, comp_state[prop]);
        return;
    }

    if (prop == IDs::selector)
    {
        int state_idx = static_cast<int> (comp_state[prop]) - 1;

        if (comp_type == IDs::OSC_GUI)
        {
            osc_comp[idx]->setSelector (state.getChildWithName (IDs::OSC).getChild (state_idx),
                                        undoManager.getManagerPtr());
            return;
        }

        if (comp_type == IDs::LFO_GUI)
        {
            lfo_comp[idx]->setSelector (state.getChildWithName (IDs::LFO).getChild (state_idx),
                                        undoManager.getManagerPtr());
            return;
        }

        if (comp_type == IDs::FILT_GUI)
        {
            filt_comp[idx]->setSelector (state.getChildWithName (IDs::FILT).getChild (state_idx),
                                         undoManager.getManagerPtr());
            return;
        }

        if (comp_type == IDs::DELAY_GUI)
        {
            del_comp[idx]->setSelector (state.getChildWithName (IDs::DELAY).getChild (state_idx),
                                        undoManager.getManagerPtr());
            return;
        }
    }
}

void MainComponent::initGuiComponents (const juce::ValueTree& v, const juce::ValueTree& vs)
{
    // clang-format off
    std::vector<std::function<void()>> btn_funcs { [this] { generateRandomParameters(); },
                                                   [this] { undoManager.undo(); },
                                                   [this] { undoManager.redo(); },
                                                   [this] { releaseResources(); }
    };
    // clang-format on

    btn_comp = std::make_unique<ButtonsGui> (btn_funcs);
    if (btn_comp.get() != nullptr)
        addAndMakeVisible (btn_comp.get());

    auto output_state = v.getChildWithName (IDs::OUTPUT_GAIN);
    output_comp = std::make_unique<OutputGui> (output_state, undoManager.getManagerPtr());
    if (output_comp.get() != nullptr)
        addAndMakeVisible (output_comp.get());

    auto seq_state = v.getChildWithName (IDs::SEQUENCER).getChildWithName (IDs::SEQ1);
    seq_comp = std::make_unique<SequencerGui> (seq_state, undoManager.getManagerPtr());
    if (seq_comp.get() != nullptr)
        addAndMakeVisible (seq_comp.get());

    for (size_t i = 0; i < osc_comp.size(); i++)
    {
        auto osc_state = v.getChildWithName (IDs::OSC).getChildWithName (IDs::Group::OSC[i]);
        auto osc_selector_state = vs.getChildWithName (IDs::OSC_GUI).getChildWithName (IDs::Group::OSC[i]);

        osc_comp[i] = std::make_unique<OscGui> (osc_state, osc_selector_state, undoManager.getManagerPtr());
        if (osc_comp[i].get() != nullptr)
            addAndMakeVisible (osc_comp[i].get());

        auto lfo_state = v.getChildWithName (IDs::LFO).getChildWithName (IDs::Group::LFO[i]);
        auto lfo_selector_state = vs.getChildWithName (IDs::LFO_GUI).getChildWithName (IDs::Group::LFO[i]);

        lfo_comp[i] = std::make_unique<LfoGui> (lfo_state, lfo_selector_state, undoManager.getManagerPtr());
        if (lfo_comp[i].get() != nullptr)
            addAndMakeVisible (lfo_comp[i].get());

        auto filt_state = v.getChildWithName (IDs::FILT).getChildWithName (IDs::Group::FILT[i]);
        auto filt_selector_state = vs.getChildWithName (IDs::FILT_GUI).getChildWithName (IDs::Group::FILT[i]);

        filt_comp[i] = std::make_unique<FiltGui> (filt_state, filt_selector_state, undoManager.getManagerPtr());
        if (filt_comp[i].get() != nullptr)
            addAndMakeVisible (filt_comp[i].get());

        auto del_state = v.getChildWithName (IDs::DELAY).getChildWithName (IDs::Group::DELAY[i]);
        auto del_selector_state = vs.getChildWithName (IDs::DELAY_GUI).getChildWithName (IDs::Group::DELAY[i]);

        del_comp[i] = std::make_unique<DelayGui> (del_state, del_selector_state, undoManager.getManagerPtr());
        if (del_comp[i].get() != nullptr)
            addAndMakeVisible (del_comp[i].get());
    }
}

void MainComponent::setLfoRoute (const size_t lfo_idx, const size_t val)
{
    struct RouteParameters
    {
        const juce::Identifier& comp;
        const juce::Identifier& prop;
        const double limit;
    };

    size_t itemId = 1;
    static const std::map<size_t, RouteParameters> rp{
        // ch
        {itemId++, {IDs::OUTPUT_GAIN, IDs::gain, param_limits.chan_min}},
        // osc
        {itemId++, {IDs::OSC, IDs::freq, param_limits.osc_freq_max}},
        {itemId++, {IDs::OSC, IDs::gain, param_limits.osc_gain_min}},
        {itemId++, {IDs::OSC, IDs::fm_freq, param_limits.osc_fm_freq_max}},
        {itemId++, {IDs::OSC, IDs::fm_depth, param_limits.osc_fm_depth_max}},
        // filter
        {itemId++, {IDs::FILT, IDs::cutOff, param_limits.filt_cutoff_max}},
        {itemId++, {IDs::FILT, IDs::reso, param_limits.filt_reso_max}},
        {itemId++, {IDs::FILT, IDs::drive, param_limits.filt_drive_max}}
        //
    };

    if (val > rp.size() || lfo_idx > lfo.size())
        return;

    const auto& opt = rp.at (val);

    lfo[lfo_idx]->setLfoRoute (opt.comp, opt.prop, opt.limit);
}

void MainComponent::initBroadcasters (const juce::ValueTree& v, const juce::ValueTree& vs)
{

    broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::OUTPUT_GAIN), IDs::master));

    broadcasters.push_back (
        std::make_unique<Broadcaster> (v.getChildWithName (IDs::SEQUENCER).getChildWithName (IDs::SEQ1), IDs::enabled));
    broadcasters.push_back (
        std::make_unique<Broadcaster> (v.getChildWithName (IDs::SEQUENCER).getChildWithName (IDs::SEQ1), IDs::time));

    for (size_t i = 0; i < NUM_OUTPUT_CHANNELS / 2; i++)
    {
        broadcasters.push_back (
            std::make_unique<Broadcaster> (v.getChildWithName (IDs::OUTPUT_GAIN).getChild (i), IDs::gain));

        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::OSC).getChild (i), IDs::wavetype));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::OSC).getChild (i), IDs::freq));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::OSC).getChild (i), IDs::gain));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::OSC).getChild (i), IDs::fm_freq));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::OSC).getChild (i), IDs::fm_depth));

        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::LFO).getChild (i), IDs::wavetype));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::LFO).getChild (i), IDs::freq));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::LFO).getChild (i), IDs::gain));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::LFO).getChild (i), IDs::route));

        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::FILT).getChild (i), IDs::enabled));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::FILT).getChild (i), IDs::filtType));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::FILT).getChild (i), IDs::cutOff));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::FILT).getChild (i), IDs::reso));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::FILT).getChild (i), IDs::drive));

        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::DELAY).getChild (i), IDs::mix));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::DELAY).getChild (i), IDs::time));
        broadcasters.push_back (std::make_unique<Broadcaster> (v.getChildWithName (IDs::DELAY).getChild (i), IDs::feedback));
    }

    // selectros broadcasters
    for (size_t i = 0; i < NUM_OUTPUT_CHANNELS / 4; ++i)
    {
        broadcasters.push_back (std::make_unique<Broadcaster> (
            vs.getChildWithName (IDs::OSC_GUI).getChildWithName (IDs::Group::OSC[i]), IDs::selector));
        broadcasters.push_back (std::make_unique<Broadcaster> (
            vs.getChildWithName (IDs::LFO_GUI).getChildWithName (IDs::Group::LFO[i]), IDs::selector));
        broadcasters.push_back (std::make_unique<Broadcaster> (
            vs.getChildWithName (IDs::FILT_GUI).getChildWithName (IDs::Group::FILT[i]), IDs::selector));
        broadcasters.push_back (std::make_unique<Broadcaster> (
            vs.getChildWithName (IDs::DELAY_GUI).getChildWithName (IDs::Group::DELAY[i]), IDs::selector));
    }

    for (auto& b : broadcasters)
        if (b.get() != nullptr)
            b->addChangeListener (this);
}

juce::var MainComponent::getStateParamValue (const juce::ValueTree& v, const juce::Identifier& parent,
                                             const juce::Identifier& node, const juce::Identifier& propertie)
{
    return v.getChildWithName (parent).getChildWithName (node).getProperty (propertie);
}

template <typename T>
void MainComponent::setParam (const size_t idx, const juce::Identifier& comp_type, const juce::Identifier& propertie, T val)
{

    if (idx > chains.size())
        return;

    if (propertie == IDs::master)
    {
        for (auto& [left, right] : chains)
        {
            left->template get<ProcIdx::MASTER_GAIN>().setGainLinear (val);
            right->template get<ProcIdx::MASTER_GAIN>().setGainLinear (val);
        }
        return;
    }

    if (comp_type == IDs::OUTPUT_GAIN)
    {
        chains[idx].first->get<ProcIdx::CHAN_GAIN>().setGainDecibels (val);
        chains[idx].second->get<ProcIdx::CHAN_GAIN>().setGainDecibels (val);
        return;
    }

    if (comp_type == IDs::SEQUENCER)
    {
        if (propertie == IDs::enabled)
        {
            seq.setEnabled (val);
            return;
        }

        if (propertie == IDs::time)
        {
            seq.setTime (val);
            return;
        }
    }

    if (comp_type == IDs::OSC)
    {

        if (propertie == IDs::wavetype)
        {
            chains[idx].first->get<ProcIdx::OSC>().setWaveType (static_cast<WaveType> ((int)val));
            chains[idx].second->get<ProcIdx::OSC>().setWaveType (static_cast<WaveType> ((int)val));
            return;
        }

        if (propertie == IDs::freq)
        {
            chains[idx].first->get<ProcIdx::OSC>().setBaseFrequency (val);
            chains[idx].second->get<ProcIdx::OSC>().setBaseFrequency (val);
            return;
        }

        if (propertie == IDs::gain)
        {
            chains[idx].first->get<ProcIdx::OSC>().setGainDecibels (val);
            chains[idx].second->get<ProcIdx::OSC>().setGainDecibels (val);
            return;
        }

        if (propertie == IDs::fm_freq)
        {
            chains[idx].first->get<ProcIdx::OSC>().setFmFreq (val);
            chains[idx].second->get<ProcIdx::OSC>().setFmFreq (val);
            return;
        }

        if (propertie == IDs::fm_depth)
        {
            chains[idx].first->get<ProcIdx::OSC>().setFmDepth (val);
            chains[idx].second->get<ProcIdx::OSC>().setFmDepth (val);
            return;
        }
    }

    if (comp_type == IDs::FILT)
    {

        if (propertie == IDs::enabled)
        {
            chains[idx].first->get<ProcIdx::FILT>().setEnabled (val);
            chains[idx].second->get<ProcIdx::FILT>().setEnabled (val);
            return;
        }

        if (propertie == IDs::filtType)
        {
            static std::map<int, juce::dsp::LadderFilterMode> types{
                {1, juce::dsp::LadderFilterMode::LPF12}, {2, juce::dsp::LadderFilterMode::LPF24},
                {3, juce::dsp::LadderFilterMode::BPF12}, {4, juce::dsp::LadderFilterMode::BPF24},
                {5, juce::dsp::LadderFilterMode::HPF12}, {6, juce::dsp::LadderFilterMode::HPF24}};

            size_t k = static_cast<size_t> ((int)val);

            chains[idx].first->get<ProcIdx::FILT>().setMode (types.at (k));
            chains[idx].second->get<ProcIdx::FILT>().setMode (types.at (k));
            return;
        }

        if (propertie == IDs::cutOff)
        {
            chains[idx].first->get<ProcIdx::FILT>().setCutoffFrequencyHz (val);
            chains[idx].second->get<ProcIdx::FILT>().setCutoffFrequencyHz (val);
            return;
        }

        if (propertie == IDs::reso)
        {
            chains[idx].first->get<ProcIdx::FILT>().setResonance (val);
            chains[idx].second->get<ProcIdx::FILT>().setResonance (val);
            return;
        }

        if (propertie == IDs::drive)
        {
            chains[idx].first->get<ProcIdx::FILT>().setDrive (val);
            chains[idx].second->get<ProcIdx::FILT>().setDrive (val);
            return;
        }
    }

    if (comp_type == IDs::LFO)
    {
        if (propertie == IDs::route)
        {
            setLfoRoute (idx, static_cast<size_t> ((int)val));
            return;
        }

        if (propertie == IDs::wavetype)
        {
            lfo[idx]->setWaveType (static_cast<WaveType> ((int)val));
            return;
        }

        if (propertie == IDs::freq)
        {
            lfo[idx]->setFrequency (val);
            return;
        }

        if (propertie == IDs::gain)
        {
            lfo[idx]->setGain (val);
            return;
        }
    }

    if (comp_type == IDs::DELAY)
    {
        if (propertie == IDs::mix)
        {
            chains[idx].first->get<ProcIdx::DEL>().setWetLevel (val);
            chains[idx].second->get<ProcIdx::DEL>().setWetLevel (val);
            return;
        }
        if (propertie == IDs::time)
        {
            chains[idx].first->get<ProcIdx::DEL>().setDelayTime (val);
            chains[idx].second->get<ProcIdx::DEL>().setDelayTime (static_cast<double> (val) + 0.2);
            return;
        }
        if (propertie == IDs::feedback)
        {
            chains[idx].first->get<ProcIdx::DEL>().setFeedback (val);
            chains[idx].second->get<ProcIdx::DEL>().setFeedback (val);
            return;
        }
    }
}

void MainComponent::setDefaultParameterValues()
{
    // master gain
    setParam (0, IDs::OUTPUT_GAIN, IDs::master, def_params.master_gain);

    // seq
    setParam (0, IDs::SEQUENCER, IDs::enabled, false);
    setParam (0, IDs::SEQUENCER, IDs::time, def_params.seq_time);

    for (size_t i = 0; i < chains.size(); i++)
    {
        // channels gain
        setParam (i, IDs::OUTPUT_GAIN, IDs::gain, def_params.chan_gain);
        // OSC
        setParam (i, IDs::OSC, IDs::wavetype, def_params.osc_wavetype);
        setParam (i, IDs::OSC, IDs::freq, def_params.osc_freq + 10 * i);
        setParam (i, IDs::OSC, IDs::gain, def_params.osc_gain);
        setParam (i, IDs::OSC, IDs::fm_freq, def_params.osc_fm_freq);
        setParam (i, IDs::OSC, IDs::fm_depth, def_params.osc_fm_depth);
        // LFO
        setParam (i, IDs::LFO, IDs::wavetype, def_params.lfo_wavetype);
        setParam (i, IDs::LFO, IDs::freq, def_params.lfo_freq);
        setParam (i, IDs::LFO, IDs::gain, def_params.lfo_gain);
        setParam (i, IDs::LFO, IDs::route, i + 2);
        // filter
        setParam (i, IDs::FILT, IDs::enabled, false);
        setParam (i, IDs::FILT, IDs::filtType, def_params.filt_type);
        setParam (i, IDs::FILT, IDs::cutOff, def_params.filt_cutoff);
        setParam (i, IDs::FILT, IDs::reso, def_params.filt_reso);
        setParam (i, IDs::FILT, IDs::drive, def_params.filt_drive);
        // delay
        setParam (i, IDs::DELAY, IDs::mix, def_params.del_mix);
        setParam (i, IDs::DELAY, IDs::time, def_params.del_time);
        setParam (i, IDs::DELAY, IDs::feedback, def_params.del_feedback);
    }
}

void MainComponent::generateRandomParameters()
{
    std::array<int, NUM_OUTPUT_CHANNELS / 2> indexs_range;
    std::iota (indexs_range.begin(), indexs_range.end(), 0);
    std::shuffle (indexs_range.begin(), indexs_range.end(), gen);
    std::array<int, NUM_OUTPUT_CHANNELS / 4> indexes;
    std::copy (indexs_range.begin(), indexs_range.begin() + NUM_OUTPUT_CHANNELS / 4, indexes.begin());

    for (size_t i = 0; i < NUM_OUTPUT_CHANNELS / 2; i++)
    {
        if (std::find (indexes.begin(), indexes.end(), i) != indexes.end())
        {
            generateRandomOscParameters (i, true);
            generateRandomLfoParameters (i, true);
        }
        else
        {
            generateRandomOscParameters (i);
            generateRandomLfoParameters (i);
        }
        // generateRandomFilterParameters (i);
        generateRandomDelayParameters (i);
    }
}

void MainComponent::generateRandomOscParameters (const int index, const bool suppressed)
{
    static std::uniform_int_distribution<> osc_type (param_limits.osc_waveType_min, 3);
    static std::uniform_real_distribution<> osc_freq (param_limits.osc_freq_min, param_limits.C8);
    static std::uniform_real_distribution<> osc_fm_freq (param_limits.osc_fm_freq_min, param_limits.osc_fm_freq_max);
    static std::uniform_real_distribution<> osc_fm_depth (param_limits.osc_fm_depth_min, param_limits.osc_fm_depth_max);

    juce::ValueTree osc_state = state.getChildWithName (IDs::OSC).getChild (index);

    osc_state.setProperty (IDs::wavetype, osc_type (gen), undoManager.getManagerPtr());

    if (suppressed)
    {
        osc_state.setProperty (IDs::freq, rand.getSup (param_limits.C8, 5), undoManager.getManagerPtr());
        osc_state.setProperty (IDs::fm_freq, rand.getSup (param_limits.osc_fm_freq_max, 50), undoManager.getManagerPtr());
        osc_state.setProperty (IDs::fm_depth, rand.getSup (param_limits.osc_fm_depth_max, 5), undoManager.getManagerPtr());
    }
    else
    {
        osc_state.setProperty (IDs::freq, osc_freq (gen), undoManager.getManagerPtr());
        osc_state.setProperty (IDs::fm_freq, osc_fm_freq (gen), undoManager.getManagerPtr());
        osc_state.setProperty (IDs::fm_depth, rand.getSup (param_limits.osc_fm_depth_max, 25), undoManager.getManagerPtr());
    }
}

void MainComponent::generateRandomLfoParameters (const int index, const bool suppressed)
{
    static std::uniform_int_distribution<> lfo_type (param_limits.lfo_waveType_min, 4);
    static std::uniform_real_distribution<> lfo_freq (param_limits.lfo_freq_min, param_limits.lfo_freq_max);
    static std::uniform_real_distribution<> lfo_gain (param_limits.lfo_gain_min, param_limits.lfo_gain_max);
    static std::uniform_int_distribution<> lfo_route (2, 5);

    juce::ValueTree lfo_state = state.getChildWithName (IDs::LFO).getChild (index);

    int route = lfo_route (gen);
    lfo_state.setProperty (IDs::route, route, undoManager.getManagerPtr());

    float freq = 0, gain = 0;
    if (suppressed)
    {
        int lfo_t = lfo_type (gen);
        lfo_t = lfo_t <= 2 ? lfo_t : 4; // avoid squear on low freqs
        lfo_state.setProperty (IDs::wavetype, lfo_t, undoManager.getManagerPtr());

        freq = rand.getSup (param_limits.lfo_freq_max, 1);
        if (route == 3) // osc gain
            gain = rand.getVal (param_limits.lfo_gain_max);
        else
            gain = rand.getSup (param_limits.lfo_gain_max, 50);

        lfo_state.setProperty (IDs::freq, freq, undoManager.getManagerPtr());
        lfo_state.setProperty (IDs::gain, gain, undoManager.getManagerPtr());
    }
    else
    {
        freq = rand.getSup (param_limits.lfo_freq_max, 50);
        if (route == 3) // osc gain
            gain = rand.getVal (param_limits.lfo_gain_max);
        else
            gain = rand.getSup (param_limits.lfo_gain_max, 5);

        lfo_state.setProperty (IDs::wavetype, lfo_type (gen), undoManager.getManagerPtr());
        lfo_state.setProperty (IDs::freq, freq, undoManager.getManagerPtr());
        lfo_state.setProperty (IDs::gain, gain, undoManager.getManagerPtr());
    }
}

void MainComponent::generateRandomFilterParameters (const int index, const bool suppressed)
{
    // static std::uniform_int_distribution<> filt_enabled (0, 1);
    static std::uniform_int_distribution<> filt_type (param_limits.filt_filtType_min, param_limits.filt_filtType_max);
    static std::uniform_real_distribution<> filt_cutOff (param_limits.filt_cutoff_min, param_limits.filt_cutoff_max);
    static std::uniform_real_distribution<> filt_reso (param_limits.filt_reso_min, param_limits.filt_reso_max - 0.35);
    static std::uniform_real_distribution<> filt_drive (param_limits.filt_drive_min, param_limits.filt_drive_max - 8);

    juce::ValueTree filt_state = state.getChildWithName (IDs::FILT).getChild (index);

    if (suppressed)
    {
    }
    else
    {
        // filt_state.setProperty (IDs::enabled, (filt_enabled (gen) != 0 ? true : false), undoManager.getManagerPtr());
        filt_state.setProperty (IDs::filtType, filt_type (gen), undoManager.getManagerPtr());
        filt_state.setProperty (IDs::cutOff, filt_cutOff (gen), undoManager.getManagerPtr());
        filt_state.setProperty (IDs::reso, filt_reso (gen), undoManager.getManagerPtr());
        filt_state.setProperty (IDs::drive, filt_drive (gen), undoManager.getManagerPtr());
    }
}

void MainComponent::generateRandomDelayParameters (const int index, const bool suppressed)
{
    static std::uniform_real_distribution<> del_mix (param_limits.delay_mix_min, param_limits.delay_mix_max);
    static std::uniform_real_distribution<> del_time (param_limits.delay_time_min, param_limits.delay_time_max);
    static std::uniform_real_distribution<> del_feedback (param_limits.delay_feedback_min,
                                                          param_limits.delay_feedback_max - 0.1);

    juce::ValueTree del_state = state.getChildWithName (IDs::DELAY).getChild (index);

    if (suppressed)
    {
    }
    else
    {
        del_state.setProperty (IDs::mix, del_mix (gen), undoManager.getManagerPtr());
        del_state.setProperty (IDs::time, del_time (gen), undoManager.getManagerPtr());
        del_state.setProperty (IDs::feedback, del_feedback (gen), undoManager.getManagerPtr());
    }
}

void MainComponent::oscOn()
{
    for (auto&& [left, right] : chains)
    {
        left->get<ProcIdx::OSC>().setBypass (false);
        right->get<ProcIdx::OSC>().setBypass (false);
    }
}

void MainComponent::oscOff()
{
    for (auto&& [left, right] : chains)
    {
        left->get<ProcIdx::OSC>().setBypass (true);
        right->get<ProcIdx::OSC>().setBypass (true);
    }
}

template <typename T>
int MainComponent::getComponentWidth (const std::unique_ptr<T>& comp) const
{
    if (comp.get() != nullptr)
        return comp->getWidthNeeded();

    return 0;
}

template <typename T>
int MainComponent::getComponentHeight (const std::unique_ptr<T>& comp) const
{
    if (comp.get() != nullptr)
        return comp->getHeightNeeded();

    return 0;
}

void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid color)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    // You can add your drawing code here!
}

void MainComponent::resized()
{
    static int xPadding = 5;
    auto bounds = getLocalBounds();

    bounds.removeFromTop (gui_sizes.yGap_top);

    if (btn_comp.get() != nullptr)
    {
        auto bound = bounds.removeFromTop (btn_comp->getHeightNeeded()).reduced (xPadding, 0);
        bound.setWidth (btn_comp->getWidthNeeded());
        btn_comp->setBounds (bound);
    }

    bounds.removeFromTop (gui_sizes.yGap_top);
    auto output_bound = bounds.removeFromTop (getComponentHeight (output_comp)).reduced (xPadding, 0);
    auto seq_bound = output_bound.withTrimmedLeft (getComponentWidth (output_comp) + gui_sizes.yGap_between_components);

    if (output_comp.get() != nullptr)
    {
        output_bound.setWidth (output_comp->getWidthNeeded());
        output_comp->setBounds (output_bound);
    }

    if (seq_comp.get() != nullptr)
    {
        seq_bound.setWidth (seq_comp->getWidthNeeded());
        seq_comp->setBounds (seq_bound);
    }

    bounds.removeFromTop (gui_sizes.yGap_between_components);
    auto osc_bound = bounds.removeFromTop (getComponentHeight (osc_comp.back())).reduced (xPadding, 0);

    for (auto& c : osc_comp)
    {
        if (c.get() == nullptr)
            continue;

        c->setSize (c->getWidthNeeded(), c->getHeightNeeded());
        auto pos = osc_bound.getTopLeft();
        c->setTopLeftPosition (pos);
        osc_bound.removeFromLeft (c->getWidthNeeded() + gui_sizes.yGap_between_components);
    }

    bounds.removeFromTop (gui_sizes.yGap_between_components);
    auto filt_bound = bounds.removeFromTop (getComponentHeight (filt_comp.back())).reduced (xPadding, 0);

    for (auto& c : filt_comp)
    {
        if (c.get() == nullptr)
            continue;

        c->setSize (c->getWidthNeeded(), c->getHeightNeeded());
        auto pos = filt_bound.getTopLeft();
        c->setTopLeftPosition (pos);
        filt_bound.removeFromLeft (c->getWidthNeeded() + gui_sizes.yGap_between_components);
    }

    bounds.removeFromTop (gui_sizes.yGap_between_components);
    auto lfo_bound = bounds.removeFromTop (getComponentHeight (lfo_comp.back())).reduced (xPadding, 0);

    for (auto& c : lfo_comp)
    {
        if (c.get() == nullptr)
            continue;

        c->setSize (c->getWidthNeeded(), c->getHeightNeeded());
        auto pos = lfo_bound.getTopLeft();
        c->setTopLeftPosition (pos);
        lfo_bound.removeFromLeft (c->getWidthNeeded() + gui_sizes.yGap_between_components);
    }

    bounds.removeFromTop (gui_sizes.yGap_between_components);
    auto del_bound = bounds.removeFromTop (getComponentHeight (del_comp.back())).reduced (xPadding, 0);

    for (auto& c : del_comp)
    {
        if (c.get() == nullptr)
            continue;

        c->setSize (c->getWidthNeeded(), c->getHeightNeeded());
        auto pos = del_bound.getTopLeft();
        c->setTopLeftPosition (pos);
        del_bound.removeFromLeft (c->getWidthNeeded() + gui_sizes.yGap_between_components);
    }

    // auto adsc_bounds = getLocalBounds().removeFromLeft (70 * 4);
    // adsc.setBounds (adsc_bounds);
    // adsc.setCentrePosition (adsc_bounds.getCentre().translated (70 * 4 + 20, 0));
}
