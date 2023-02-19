#pragma once

#include "Osc.h"
#include <JuceHeader.h>
#include <array>

// Constatns
//==============================================================================
inline constexpr int NUM_INPUT_CHANNELS = 0;
inline constexpr int NUM_OUTPUT_CHANNELS = 8;

inline constexpr struct _Default_Parameters
{
    float master_gain = 0.7; // linear
    float chan_gain = -10;   // dB

    WaveType osc_wavetype = WaveType::SIN;
    double osc_freq = 440;
    float osc_gain = -25;
    float osc_fm_freq = 0;
    float osc_fm_depth = 0;

    WaveType lfo_wavetype = WaveType::SIN;
    double lfo_freq = 0;
    float lfo_gain = 0;         // linear
    size_t lfoUpdateRate = 100; // every 100 samples

    float del_mix = 0;
    float del_time = 0.7;
    float del_feedback = 0.5;

} def_params;

inline constexpr struct _Gui_Parameters
{
    // JUCE slider setter/getters are expecting double types
    // combobox expecting int's

    int selector[NUM_OUTPUT_CHANNELS / 2]{1, 2, 3, 4};

    double master_min = 0, master_max = 1;
    double chan_min = -100.0, chan_max = 0;

    int osc_waveType_min = 1, osc_waveType_max = 6;
    double osc_freq_min = 0, osc_freq_max = 24000;
    double osc_gain_min = -100, osc_gain_max = 0;
    double osc_fm_freq_min = 0, osc_fm_freq_max = 20;
    double osc_fm_depth_min = 0, osc_fm_depth_max = 10;

    int lfo_waveType_min = 1, lfo_waveType_max = 6;
    double lfo_freq_min = 0, lfo_freq_max = 70;
    double lfo_gain_min = 0, lfo_gain_max = 1;

    double delay_mix_min = 0, delay_mix_max = 1;
    double delay_time_min = 0, delay_time_max = 4.79;
    double delay_feedback_min = 0, delay_feedback_max = 1;

} gui_params;

inline constexpr struct _Gui_Sizes
{
    int yGap_between_components = 10;
    int yGap_top = 4;

    int comp_title_font = 15;
    int selector_box_width = 50;

} gui_sizes;

//==============================================================================
namespace IDs
{
#define DECLARE_ID(name) const juce::Identifier name (#name);

DECLARE_ID (ROOT)

DECLARE_ID (OUTPUT_GAIN)
DECLARE_ID (master)
DECLARE_ID (CHAN1)
DECLARE_ID (CHAN2)
DECLARE_ID (CHAN3)
DECLARE_ID (CHAN4)

DECLARE_ID (OSC_GUI)
DECLARE_ID (selector)
DECLARE_ID (OSC)
DECLARE_ID (OSC1)
DECLARE_ID (OSC2)
DECLARE_ID (OSC3)
DECLARE_ID (OSC4)

DECLARE_ID (LFO_GUI)
DECLARE_ID (LFO)
DECLARE_ID (LFO1)
DECLARE_ID (LFO2)
DECLARE_ID (LFO3)
DECLARE_ID (LFO4)

DECLARE_ID (wavetype)
DECLARE_ID (freq)
DECLARE_ID (gain)
DECLARE_ID (fm_freq)
DECLARE_ID (fm_depth)

DECLARE_ID (DELAY_GUI)
DECLARE_ID (DELAY)
DECLARE_ID (DELAY1)
DECLARE_ID (DELAY2)
DECLARE_ID (DELAY3)
DECLARE_ID (DELAY4)
DECLARE_ID (mix)
DECLARE_ID (time)
DECLARE_ID (feedback)

#undef DECLARE_ID

namespace Group
{
    using IdsArray = std::array<std::reference_wrapper<const juce::Identifier>, NUM_OUTPUT_CHANNELS / 2>;

    const IdsArray CHAN = {IDs::CHAN1, IDs::CHAN2, IDs::CHAN3, IDs::CHAN4};
    /* const IdsArray CHAN = {IDs::CHAN1, IDs::CHAN2, IDs::CHAN3, IDs::CHAN4, */
    /*                        IDs::CHAN5, IDs::CHAN6, IDs::CHAN7, IDs::CHAN8}; */

    const IdsArray OSC = {IDs::OSC1, IDs::OSC2, IDs::OSC3, IDs::OSC4};
    /* const IdsArray OSC = {IDs::OSC1, IDs::OSC2, IDs::OSC3, IDs::OSC4, IDs::OSC5, IDs::OSC6, IDs::OSC7, IDs::OSC8}; */

    const IdsArray LFO = {IDs::LFO1, IDs::LFO2, IDs::LFO3, IDs::LFO4};
    /* const IdsArray LFO = {IDs::LFO1, IDs::LFO2, IDs::LFO3, IDs::LFO4, IDs::LFO5, IDs::LFO6, IDs::LFO7, IDs::LFO8}; */

    const IdsArray DELAY = {IDs::DELAY1, IDs::DELAY2, IDs::DELAY3, IDs::DELAY4};
    /* const IdsArray DELAY = {IDs::DELAY1, IDs::DELAY2, IDs::DELAY3, IDs::DELAY4, */
    /*                         IDs::DELAY5, IDs::DELAY6, IDs::DELAY7, IDs::DELAY8}; */
}; // namespace Group

}; // namespace IDs

// TMP declarations
//==============================================================================
