#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent (juce::ValueTree st, juce::ValueTree selectors_st) : state (st)
// adsc (deviceManager, 0, NUM_INPUT_CHANNELS, 0, NUM_OUTPUT_CHANNELS, false, false, true, false)
{
    for (int i = 0; i < NUM_OUTPUT_CHANNELS / 2; i++)
    {
        chains.push_back (std::make_pair (std::make_unique<Chain>(), std::make_unique<Chain>()));
        lfo.push_back (Osc<float>());
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

    // addAndMakeVisible (adsc);

    initGuiComponents (state, selectors_st);

    startTimer (500);

    int main_comp_width = 0, main_comp_height = 0;
    for (unsigned i = 0; i < osc_comp.size(); i++)
    {
        main_comp_width += std::max (
            {getComponentWidth (osc_comp[i]), getComponentWidth (lfo_comp[i]), getComponentWidth (del_comp[i])});
    }

    main_comp_height += getComponentHeight (btn_comp) + gui_sizes.yGap_top;
    main_comp_height += getComponentHeight (output_comp) + gui_sizes.yGap_top;
    main_comp_height += getComponentHeight (osc_comp.back()) + gui_sizes.yGap_between_components;
    main_comp_height += getComponentHeight (lfo_comp.back()) + gui_sizes.yGap_between_components;
    main_comp_height += getComponentHeight (del_comp.back()) + gui_sizes.yGap_between_components;

    setSize (main_comp_width + 30, main_comp_height + 30);
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
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlockExpected);
    spec.numChannels = 1;

    jassert (chains.size() == lfo.size());
    for (size_t i = 0; i < chains.size(); i++)
    {
        chains[i].first->prepare (spec);
        chains[i].second->prepare (spec);
        lfo[i].prepare ({spec.sampleRate / def_params.lfoUpdateRate, spec.maximumBlockSize, spec.numChannels});
    }

    setDefaultParameterValues();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    juce::dsp::AudioBlock<float> audioBlock{*bufferToFill.buffer};

    for (size_t i = 0; static_cast<int> (i) < bufferToFill.buffer->getNumChannels(); i += 2)
        audio_blocks.push_back (
            std::make_pair (audioBlock.getSingleChannelBlock (i), audioBlock.getSingleChannelBlock (i + 1)));

    // process by sample
    for (auto samp = 0; samp < bufferToFill.numSamples; ++samp)
    {
        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = def_params.lfoUpdateRate;
            for (size_t i = 0; i < lfo.size(); i++)
            {
                float lfo_range =
                    gui_params.osc_freq_max
                    - static_cast<float> (getStateParamValue (state, IDs::LFO, IDs::Group::LFO[i], IDs::freq));
                double lfo_gain =
                    static_cast<double> (getStateParamValue (state, IDs::LFO, IDs::Group::LFO[i], IDs::gain));
                double osc_freq =
                    static_cast<double> (getStateParamValue (state, IDs::OSC, IDs::Group::OSC[i], IDs::freq));
                auto lfoOut = lfo[i].processSample (0.0f);
                float mod = juce::jmap (lfoOut, -1.0f, 1.0f, static_cast<float> (osc_freq), lfo_range);
                setChainParams (&chains[i], IDs::OSC, IDs::freq, osc_freq + mod * lfo_gain);
            }
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

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    auto comp = dynamic_cast<BaseComp*> (source);
    auto comp_state = comp->getState();
    auto comp_type = comp_state.getParent().getType();
    auto comp_id = comp_state.getType();
    auto prop = comp->propertie;

    if (prop == IDs::master)
    {
        setChainParams (nullptr, IDs::OUTPUT_GAIN, prop, comp_state[prop]);
        return;
    }

    if (comp_type == IDs::OUTPUT_GAIN || comp_type == IDs::OSC || comp_type == IDs::LFO || comp_type == IDs::DELAY)
    {
        unsigned idx = static_cast<unsigned> (comp_state.getParent().indexOf (comp_state));
        const auto& chain = &chains[idx];
        setChainParams (chain, comp_type, prop, comp_state[prop]);
        return;
    }

    if (comp_type == IDs::OSC_GUI)
    {
        unsigned comp_idx = static_cast<unsigned> (comp_state.getParent().indexOf (comp_state));
        osc_comp[comp_idx]->setSelector (
            state.getChildWithName (IDs::OSC).getChild (static_cast<int> (comp_state[prop]) - 1));
        return;
    }

    if (comp_type == IDs::LFO_GUI)
    {
        unsigned comp_idx = static_cast<unsigned> (comp_state.getParent().indexOf (comp_state));
        lfo_comp[comp_idx]->setSelector (
            state.getChildWithName (IDs::LFO).getChild (static_cast<int> (comp_state[prop]) - 1));
        return;
    }

    if (comp_type == IDs::DELAY_GUI)
    {
        unsigned comp_idx = static_cast<unsigned> (comp_state.getParent().indexOf (comp_state));
        del_comp[comp_idx]->setSelector (
            state.getChildWithName (IDs::DELAY).getChild (static_cast<int> (comp_state[prop]) - 1));
        return;
    }
}

void MainComponent::initGuiComponents (const juce::ValueTree& v, const juce::ValueTree& vs)
{

    std::vector<std::function<void()>> funcs;
    funcs.push_back ([this] { generateRandomParameters(); });
    funcs.push_back ([this] { undoManager.undo(); });
    funcs.push_back ([this] { undoManager.redo(); });

    btn_comp = std::make_unique<ButtonsGui> (funcs);
    addAndMakeVisible (btn_comp.get());

    auto const& output_state = v.getChildWithName (IDs::OUTPUT_GAIN);
    output_comp = std::make_unique<OutputGui> (output_state, undoManager, this);
    addAndMakeVisible (output_comp.get());

    for (unsigned i = 0; i < osc_comp.size(); i++)
    {
        auto const& osc_state = v.getChildWithName (IDs::OSC).getChildWithName (IDs::Group::OSC[i]);
        auto const& osc_selector_state = vs.getChildWithName (IDs::OSC_GUI).getChildWithName (IDs::Group::OSC[i]);

        osc_comp[i] = std::make_unique<OscGui> (osc_state, osc_selector_state, undoManager, this);
        if (osc_comp[i].get() != nullptr)
            addAndMakeVisible (osc_comp[i].get());

        auto const& lfo_state = v.getChildWithName (IDs::LFO).getChildWithName (IDs::Group::LFO[i]);
        auto const& lfo_selector_state = vs.getChildWithName (IDs::LFO_GUI).getChildWithName (IDs::Group::LFO[i]);

        lfo_comp[i] = std::make_unique<LfoGui> (lfo_state, lfo_selector_state, undoManager, this);
        if (lfo_comp[i].get() != nullptr)
            addAndMakeVisible (lfo_comp[i].get());

        auto const& del_state = v.getChildWithName (IDs::DELAY).getChildWithName (IDs::Group::DELAY[i]);
        auto const& del_selector_state = vs.getChildWithName (IDs::DELAY_GUI).getChildWithName (IDs::Group::DELAY[i]);

        del_comp[i] = std::make_unique<DelayGui> (del_state, del_selector_state, undoManager, this);
        if (del_comp[i].get() != nullptr)
            addAndMakeVisible (del_comp[i].get());
    }
}

// juce::var MainComponent::getParamValue (const juce::Identifier& parent, const juce::Identifier& node,
//                                         const juce::Identifier& propertie)
// {
//     return state.getChildWithName (parent).getChildWithName (node).getProperty (propertie);
// }

// template <typename T>
// void MainComponent::setParamValue (const juce::Identifier& parent, const juce::Identifier& node,
//                                    const juce::Identifier& propertie, T val)
// {
//     state.getChildWithName (parent).getChildWithName (node).setProperty (propertie, val, nullptr);
// }

juce::var MainComponent::getStateParamValue (const juce::ValueTree& v, const juce::Identifier& parent,
                                             const juce::Identifier& node, const juce::Identifier& propertie)
{
    return v.getChildWithName (parent).getChildWithName (node).getProperty (propertie);
}

template <typename T, typename Func, typename... O>
void MainComponent::setChainParams (T val, Func f, O*... obj)
{
    (..., (obj->*f) (val));
}

template <typename T>
void MainComponent::setChainParams (StereoChain* chain, const juce::Identifier& comp_type,
                                    const juce::Identifier& propertie, T val)
{
    if (propertie == IDs::master)
    {
        for (auto& c : chains)
        {
            c.first->get<ProcIdx::masterGain>().setGainLinear (val);
            c.second->get<ProcIdx::masterGain>().setGainLinear (val);
        }
        return;
    }

    if (comp_type == IDs::OUTPUT_GAIN)
    {
        chain->first->get<ProcIdx::chanGain>().setGainDecibels (val);
        chain->second->get<ProcIdx::chanGain>().setGainDecibels (val);
        return;
    }

    if (comp_type == IDs::OSC)
    {

        if (propertie == IDs::wavetype)
        {
            chain->first->get<ProcIdx::osc>().setWaveType (static_cast<WaveType> ((int)val));
            chain->second->get<ProcIdx::osc>().setWaveType (static_cast<WaveType> ((int)val));
            return;
        }

        if (propertie == IDs::freq)
        {
            chain->first->get<ProcIdx::osc>().setFrequency (val);
            chain->second->get<ProcIdx::osc>().setFrequency (val);
            return;
        }

        if (propertie == IDs::gain)
        {
            chain->first->get<ProcIdx::osc>().setGainDecibels (val);
            chain->second->get<ProcIdx::osc>().setGainDecibels (val);
            return;
        }

        if (propertie == IDs::fm_freq)
        {
            chain->first->get<ProcIdx::osc>().setFmFreq (val);
            chain->second->get<ProcIdx::osc>().setFmFreq (val);
            return;
        }

        if (propertie == IDs::fm_depth)
        {
            chain->first->get<ProcIdx::osc>().setFmDepth (val);
            chain->second->get<ProcIdx::osc>().setFmDepth (val);
            return;
        }
    }

    if (comp_type == IDs::LFO)
    {
        // TODO: temporary hack
        size_t idx = 0;
        for (auto& c : chains)
        {
            if (&c == chain)
                break;
            ++idx;
        }

        if (propertie == IDs::wavetype)
        {
            lfo[idx].setWaveType (static_cast<WaveType> ((int)val));
            return;
        }

        if (propertie == IDs::freq)
        {
            lfo[idx].setFrequency (val);
            return;
        }

        if (propertie == IDs::gain)
        {
            lfo[idx].setGainLinear (val);
            return;
        }
    }

    if (comp_type == IDs::DELAY)
    {
        if (propertie == IDs::mix)
        {
            chain->first->get<ProcIdx::del>().setWetLevel (val);
            chain->second->get<ProcIdx::del>().setWetLevel (val);
            return;
        }
        if (propertie == IDs::time)
        {
            chain->first->get<ProcIdx::del>().setDelayTime (val);
            chain->second->get<ProcIdx::del>().setDelayTime (static_cast<double> (val) + 0.2);
            return;
        }
        if (propertie == IDs::feedback)
        {
            chain->first->get<ProcIdx::del>().setFeedback (val);
            chain->second->get<ProcIdx::del>().setFeedback (val);
            return;
        }
    }
}

void MainComponent::setDefaultParameterValues()
{
    // master gain
    setChainParams (nullptr, IDs::OUTPUT_GAIN, IDs::master, def_params.master_gain);

    for (size_t i = 0; i < chains.size(); i++)
    {
        // channels gain
        setChainParams (&chains[i], IDs::OUTPUT_GAIN, IDs::gain, def_params.chan_gain);
        // OSC
        setChainParams (&chains[i], IDs::OSC, IDs::wavetype, def_params.osc_wavetype);
        setChainParams (&chains[i], IDs::OSC, IDs::freq, def_params.osc_freq);
        setChainParams (&chains[i], IDs::OSC, IDs::gain, def_params.osc_gain);
        setChainParams (&chains[i], IDs::OSC, IDs::fm_freq, def_params.osc_fm_freq);
        setChainParams (&chains[i], IDs::OSC, IDs::fm_depth, def_params.osc_fm_depth);
        // LFO
        setChainParams (&chains[i], IDs::LFO, IDs::wavetype, def_params.lfo_wavetype);
        setChainParams (&chains[i], IDs::LFO, IDs::freq, def_params.lfo_freq);
        setChainParams (&chains[i], IDs::LFO, IDs::gain, def_params.lfo_gain);
        // delay
        setChainParams (&chains[i], IDs::DELAY, IDs::mix, def_params.del_mix);
        setChainParams (&chains[i], IDs::DELAY, IDs::time, def_params.del_time);
        setChainParams (&chains[i], IDs::DELAY, IDs::feedback, def_params.del_feedback);
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

void MainComponent::generateRandomParameters()
{
    std::random_device rd;
    std::mt19937 gen (rd());

    std::uniform_int_distribution<> osc_type (gui_params.osc_waveType_min, 3);
    std::uniform_real_distribution<> osc_freq (gui_params.osc_freq_min, gui_params.osc_freq_max);
    std::uniform_real_distribution<> osc_fm_freq (gui_params.osc_fm_freq_min, gui_params.osc_fm_freq_max);
    std::uniform_real_distribution<> osc_fm_depth (gui_params.osc_fm_depth_min, gui_params.osc_fm_depth_max);

    std::uniform_int_distribution<> lfo_type (gui_params.lfo_waveType_min, 3);
    std::uniform_real_distribution<> lfo_freq (gui_params.lfo_freq_min, gui_params.lfo_freq_max);
    std::uniform_real_distribution<> lfo_gain (gui_params.lfo_gain_min, percentageFrom (30, gui_params.lfo_gain_max));

    std::uniform_real_distribution<> del_mix (gui_params.delay_mix_min, gui_params.delay_mix_max);
    std::uniform_real_distribution<> del_time (gui_params.delay_time_min, gui_params.delay_time_max);
    std::uniform_real_distribution<> del_feedback (gui_params.delay_feedback_min, gui_params.delay_feedback_max);

    for (auto osc_parameters : state.getChildWithName (IDs::OSC))
    {
        osc_parameters.setProperty (IDs::wavetype, osc_type (gen), &undoManager);
        osc_parameters.setProperty (IDs::freq, osc_freq (gen), &undoManager);
        osc_parameters.setProperty (IDs::fm_freq, osc_fm_freq (gen), &undoManager);
        osc_parameters.setProperty (IDs::fm_depth, osc_fm_depth (gen), &undoManager);
    }

    for (auto lfo_parameters : state.getChildWithName (IDs::LFO))
    {
        lfo_parameters.setProperty (IDs::wavetype, lfo_type (gen), &undoManager);
        lfo_parameters.setProperty (IDs::freq, lfo_freq (gen), &undoManager);
        lfo_parameters.setProperty (IDs::gain, lfo_gain (gen), &undoManager);
    }

    for (auto del_parameters : state.getChildWithName (IDs::DELAY))
    {
        del_parameters.setProperty (IDs::mix, del_mix (gen), &undoManager);
        del_parameters.setProperty (IDs::time, del_time (gen), &undoManager);
        del_parameters.setProperty (IDs::feedback, del_feedback (gen), &undoManager);
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
    int xPadding = 5;
    auto bounds = getLocalBounds();

    bounds.removeFromTop (gui_sizes.yGap_top);

    if (btn_comp.get() != nullptr)
    {
        auto bound = bounds.removeFromTop (btn_comp->getHeightNeeded()).reduced (xPadding, 0);
        bound.setWidth (btn_comp->getWidthNeeded());
        btn_comp->setBounds (bound);
    }

    bounds.removeFromTop (gui_sizes.yGap_top);

    if (output_comp.get() != nullptr)
    {
        auto bound = bounds.removeFromTop (output_comp->getHeightNeeded()).reduced (xPadding, 0);
        bound.setWidth (output_comp->getWidthNeeded());
        output_comp->setBounds (bound);
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
