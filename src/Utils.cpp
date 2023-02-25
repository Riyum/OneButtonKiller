#include "Utils.h"

Broadcaster::Broadcaster (const juce::ValueTree& v, const juce::Identifier& prop) : propertie (prop), state (v)

{
    state.addListener (this);
}

juce::ValueTree Broadcaster::getState() const
{
    return state;
}

void Broadcaster::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& p)
{
    juce::ignoreUnused (v);
    if (propertie == p)
    {
        sendChangeMessage();
    }
}

// clang-format off

//==============================================================================
juce::ValueTree createSelectorsTree()
{
    juce::ValueTree oscs{IDs::OSC_GUI, {}};
    juce::ValueTree lfos{IDs::LFO_GUI, {}};
    juce::ValueTree dels{IDs::DELAY_GUI, {}};

    for (size_t i = 0; i < NUM_OUTPUT_CHANNELS / 4; ++i)
    {
        juce::ValueTree osc{IDs::Group::OSC[i], {{IDs::selector, param_limits.selector[i]}}};
        juce::ValueTree lfo{IDs::Group::LFO[i], {{IDs::selector, param_limits.selector[i]}}};
        juce::ValueTree del{IDs::Group::DELAY[i], {{IDs::selector, param_limits.selector[i]}}};

        oscs.addChild (osc, -1, nullptr);
        lfos.addChild (lfo, -1, nullptr);
        dels.addChild (del, -1, nullptr);
    }

    juce::ValueTree root (IDs::ROOT);
    root.addChild (oscs, -1, nullptr);
    root.addChild (lfos, -1, nullptr);
    root.addChild (dels, -1, nullptr);

    return root;
}

juce::ValueTree createDefaultTree()
{
    juce::ValueTree outputs{IDs::OUTPUT_GAIN, {{IDs::master, def_params.master_gain}}};
    juce::ValueTree oscs{IDs::OSC, {}};
    juce::ValueTree lfos{IDs::LFO, {}};
    juce::ValueTree dels{IDs::DELAY, {}};

    for (size_t i = 0; i < NUM_OUTPUT_CHANNELS / 2; ++i)
    {
        juce::ValueTree chan{IDs::Group::CHAN[i], {{IDs::gain, def_params.chan_gain}}};

        juce::ValueTree osc{IDs::Group::OSC[i],
                            {{IDs::wavetype, def_params.osc_wavetype},
                             {IDs::freq, def_params.osc_freq + i*10},
                             {IDs::gain, def_params.osc_gain},
                             {IDs::fm_freq, def_params.osc_fm_freq},
                             {IDs::fm_depth, def_params.osc_fm_depth}}};

        juce::ValueTree lfo{IDs::Group::LFO[i],
                            {{IDs::wavetype, def_params.lfo_wavetype},
                             {IDs::freq, def_params.lfo_freq},
                             {IDs::gain, def_params.lfo_gain}}};

        juce::ValueTree del{IDs::Group::DELAY[i],
                            {{IDs::mix, def_params.del_mix},
                             {IDs::time, def_params.del_time},
                             {IDs::feedback, def_params.del_feedback}}};

        outputs.addChild (chan, -1, nullptr);
        oscs.addChild (osc, -1, nullptr);
        lfos.addChild (lfo, -1, nullptr);
        dels.addChild (del, -1, nullptr);
    }

    juce::ValueTree root (IDs::ROOT);
    root.addChild (outputs, -1, nullptr);
    root.addChild (oscs, -1, nullptr);
    root.addChild (lfos, -1, nullptr);
    root.addChild (dels, -1, nullptr);

    return root;
}

// clang-format on
