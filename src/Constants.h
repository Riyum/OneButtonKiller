#pragma once

#include <JuceHeader.h>
#include <array>

// Constatns
//==============================================================================

inline constexpr int NUM_INPUT_CHANNELS = 0;
inline constexpr int NUM_OUTPUT_CHANNELS = 8;

enum WaveType
{
    SIN = 1,
    SAW,
    SQR,
    RAND,
    WSIN,
    WSAW,
    WSQR
};

inline constexpr struct _Default_Parameters
{
    double seq_time = 500;

    float master_gain = 0.5; // linear
    float chan_gain = 0;     // dB

    WaveType osc_wavetype = WaveType::SIN;
    float osc_freq = 440;
    float osc_gain = -25;
    float osc_fm_freq = 0;
    float osc_fm_depth = 0;

    WaveType lfo_wavetype = WaveType::SIN;
    float lfo_freq = 0;
    float lfo_gain = 0;         // linear
    size_t lfoUpdateRate = 100; // samples

    int filt_type = 1; // LPF12;
    bool filt_enabled = false;
    float filt_cutoff = 0.1;
    float filt_reso = 0;
    float filt_drive = 1;

    float del_mix = 0;
    float del_time = 0.7;
    float del_feedback = 0.5;

} def_params;

inline constexpr struct _Parameter_Limits
{
    // JUCE slider setter/getters are expecting double types
    // combobox expecting int's

    int selector[NUM_OUTPUT_CHANNELS / 2]{1, 2, 3, 4};

    double seq_time_min = 50, seq_time_max = 2000;

    double master_min = 0, master_max = 1;
    double chan_min = -100.0, chan_max = 0;

    int C8 = 4186; // highest note on a standard 88-key piano
    int osc_waveType_min = 1, osc_waveType_max = 7;
    double osc_freq_min = 0, osc_freq_max = 22000;
    double osc_gain_min = -100, osc_gain_max = 0;
    double osc_fm_freq_min = 0, osc_fm_freq_max = 150;
    double osc_fm_depth_min = 0, osc_fm_depth_max = 1;

    int lfo_waveType_min = 1, lfo_waveType_max = 4;
    double lfo_freq_min = 0, lfo_freq_max = 30;
    double lfo_gain_min = 0, lfo_gain_max = 1;

    int filt_filtType_min = 1, filt_filtType_max = 6;
    double filt_cutoff_min = 0.1, filt_cutoff_max = 22000;
    double filt_reso_min = 0, filt_reso_max = 1;
    double filt_drive_min = 1, filt_drive_max = 10;

    double delay_mix_min = 0, delay_mix_max = 1;
    double delay_time_min = 0, delay_time_max = 1.79;
    double delay_feedback_min = 0, delay_feedback_max = 1;

} param_limits;

inline constexpr struct _Gui_Sizes
{
    int yGap_between_components = 10;
    int yGap_top = 4;

    int comp_title_font = 15;
    int selector_box_width = 50;
    int route_box_width = 100;

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

DECLARE_ID (SEQUENCER)
DECLARE_ID (SEQ1)

DECLARE_ID (OSC_GUI)
DECLARE_ID (selector)
DECLARE_ID (OSC)
DECLARE_ID (OSC1)
DECLARE_ID (OSC2)
DECLARE_ID (OSC3)
DECLARE_ID (OSC4)

DECLARE_ID (LFO_GUI)
DECLARE_ID (route)
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

DECLARE_ID (FILT_GUI)
DECLARE_ID (FILT)
DECLARE_ID (FILT1)
DECLARE_ID (FILT2)
DECLARE_ID (FILT3)
DECLARE_ID (FILT4)
DECLARE_ID (enabled)
DECLARE_ID (filtType)
DECLARE_ID (cutOff)
DECLARE_ID (reso)
DECLARE_ID (drive)

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

    const IdsArray OSC = {IDs::OSC1, IDs::OSC2, IDs::OSC3, IDs::OSC4};

    const IdsArray LFO = {IDs::LFO1, IDs::LFO2, IDs::LFO3, IDs::LFO4};

    const IdsArray FILT = {IDs::FILT1, IDs::FILT2, IDs::FILT3, IDs::FILT4};

    const IdsArray DELAY = {IDs::DELAY1, IDs::DELAY2, IDs::DELAY3, IDs::DELAY4};

}; // namespace Group

}; // namespace IDs
