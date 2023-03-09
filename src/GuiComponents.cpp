#include "GuiComponents.h"

//==============================================================================
void setComponentGraphics (juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::String& text)
{
    g.fillAll (juce::Colours::darkslategrey);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (gui_sizes.comp_title_font, juce::Font::FontStyleFlags::bold));
    g.drawText (text, bounds.reduced (5, 10), juce::Justification::topLeft);
    g.drawRect (bounds, 1);
}

//==============================================================================
ButtonsGui::ButtonsGui (const std::vector<std::function<void()>>& funcs)
{

    juce::StringArray str{"Magic", "Undo", "Redo", "!"};

    for (unsigned i = 0; i < comps.size(); ++i)
    {
        comps[i] = std::make_unique<juce::TextButton> (str[static_cast<int> (i)]);
        comps[i]->onClick = funcs[i];
        addAndMakeVisible (comps[i].get());
    }
}

void ButtonsGui::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void ButtonsGui::resized()
{
    auto bounds = getLocalBounds();

    for (auto&& c : comps)
    {
        if (c->getButtonText() == "!")
        {
            c->setSize (panic_btn_width, btn_height);
            c->setTopLeftPosition (bounds.removeFromLeft (panic_btn_width + btn_gap).getTopLeft());
            continue;
        }
        c->setSize (btn_width, btn_height);
        c->setTopLeftPosition (bounds.removeFromLeft (btn_width + btn_gap).getTopLeft());
    }
}

int ButtonsGui::getWidthNeeded()
{
    return (btn_width + btn_gap) * 3 + panic_btn_width;
}

int ButtonsGui::getHeightNeeded()
{
    return btn_height;
}

//==============================================================================
OutputGui::OutputGui (juce::ValueTree& v, juce::UndoManager* um)
{

    for (unsigned i = 0; i < comps.size(); ++i)
    {
        if (i != 0)
        {
            auto ch_node = v.getChildWithName (IDs::Group::CHAN[i - 1]);
            comps[i] = std::make_unique<SliderComp> (ch_node, um, IDs::gain, "Ch" + std::to_string (i),
                                                     juce::Range{param_limits.chan_min, param_limits.chan_max}, 0.001,
                                                     3, "dB");
        }
        else
            comps[i] = std::make_unique<SliderComp> (v, um, IDs::master, "Master",
                                                     juce::Range{param_limits.master_min, param_limits.master_max},
                                                     0.001, 1, "%");

        addAndMakeVisible (comps[i]->getComponent());
        addAndMakeVisible (comps[i]->label);
    }
}

void OutputGui::paint (juce::Graphics& g)
{
    setComponentGraphics (g, getLocalBounds(), "Output");
}

void OutputGui::resized()
{
    auto bounds = getLocalBounds().reduced (0, gui_sizes.comp_title_font);
    // auto bounds = getLocalBounds().withTrimmedLeft (gui_sizes.comp_title_font * 3);
    for (auto&& c : comps)
    {
        c->getComponent()->setSize (juce::jmin (bounds.getWidth(), c->getPreferredWidth()), c->getPreferredHeight());
        c->getComponent()->setTopLeftPosition (
            bounds.removeFromLeft (c->getPreferredWidth() + 2).getTopLeft().translated (0, 35));
    }
}

int OutputGui::getWidthNeeded()
{
    return 72 * 5;
}

int OutputGui::getHeightNeeded()
{
    return 70 + 60;
}

//==============================================================================
SequencerGui::SequencerGui (juce::ValueTree& v, juce::UndoManager* um)
{
    juce::ignoreUnused (um);
    onOff_btn.getToggleStateValue().referTo (v.getPropertyAsValue (IDs::enabled, nullptr));
    addAndMakeVisible (onOff_btn);

    slider = std::make_unique<SliderComp> (
        v, um, IDs::time, "Tempo", juce::Range{param_limits.seq_time_min, param_limits.seq_time_max}, 1, 0.4, "ms");

    addAndMakeVisible (slider->getComponent());
    addAndMakeVisible (slider->label);
}

void SequencerGui::paint (juce::Graphics& g)
{
    setComponentGraphics (g, getLocalBounds(), "Rand Seq");
}

void SequencerGui::resized()
{
    auto pos = getLocalBounds().withTrimmedTop (5).withTrimmedRight (5).getTopRight();
    onOff_btn.setSize (btn_width, btn_height);
    onOff_btn.setTopRightPosition (pos.x, pos.y);

    auto bounds = getLocalBounds().withTrimmedTop (38);
    slider->getComponent()->setSize (juce::jmin (bounds.getWidth(), slider->getPreferredWidth()),
                                     slider->getPreferredHeight());

    pos = bounds.removeFromLeft (getWidthNeeded()).getCentre();
    slider->getComponent()->setCentrePosition (pos.x, pos.y);
}

int SequencerGui::getWidthNeeded()
{
    return 100;
}

int SequencerGui::getHeightNeeded()
{
    return 70 + 60;
}

//==============================================================================
OscGui::OscGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um)
{
    unsigned i = 0;

    comps[i++] = std::make_unique<ComboComp> (vs, um, IDs::selector, "", juce::StringArray{"1", "2", "3", "4"});

    comps[i++] = std::make_unique<ComboComp> (
        v, um, IDs::waveType, "", juce::StringArray{"sine", "saw", "square", "rand", "wsine", "wsaw", "wsqr"});

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::freq, "Freq", juce::Range{param_limits.osc_freq_min, param_limits.osc_freq_max}, 0.001, 0.4, "Hz");

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::gain, "Gain", juce::Range{param_limits.osc_gain_min, param_limits.osc_gain_max}, 0.001, 3.0, "dB");

    comps[i++] = std::make_unique<SliderComp> (v, um, IDs::fm_freq, "FM freq",
                                               juce::Range{param_limits.osc_fm_freq_min, param_limits.osc_fm_freq_max},
                                               0.001, 1, "Hz");

    comps[i++] = std::make_unique<SliderComp> (v, um, IDs::fm_depth, "FM depth",
                                               juce::Range{param_limits.osc_fm_depth_min, param_limits.osc_fm_depth_max},
                                               0.001, 0.3);

    comps[i] = std::make_unique<SliderComp> (v, um, IDs::pan, "Pan",
                                             juce::Range{param_limits.osc_pan_min, param_limits.osc_pan_max}, 0.001, 1);

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        addAndMakeVisible (c->label);
        addAndMakeVisible (c->getComponent());
    }
}

void OscGui::paint (juce::Graphics& g)
{
    setComponentGraphics (g, getLocalBounds(), "OSC");
}

void OscGui::resized()
{
    // juce::Component a;

    auto boxes_bounds = getLocalBounds().withTrimmedLeft (gui_sizes.comp_title_font * 3);
    boxes_bounds.removeFromTop (5);
    auto slider_bounds = getLocalBounds().removeFromTop (5);

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        if (c->propertie == IDs::selector)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), gui_sizes.selector_box_width),
                                        c->getPreferredHeight());
            c->getComponent()->setTopLeftPosition (boxes_bounds.removeFromLeft (gui_sizes.selector_box_width).getTopLeft());
            continue;
        }

        if (c->propertie == IDs::waveType)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), c->getPreferredWidth()),
                                        c->getPreferredHeight());
            c->getComponent()->setTopLeftPosition (boxes_bounds.removeFromLeft (c->getPreferredWidth()).getTopLeft());
            continue;
        }
        c->getComponent()->setSize (juce::jmin (slider_bounds.getWidth(), c->getPreferredWidth()), c->getPreferredHeight());
        c->getComponent()->setTopLeftPosition (
            slider_bounds.removeFromLeft (c->getPreferredWidth() + 2).getTopLeft().translated (0, 35 + 30));
    }
}

void OscGui::setSelector (juce::ValueTree v, juce::UndoManager* um)
{
    for (auto& c : comps)
    {
        if (c->propertie == IDs::selector)
            continue;

        if (auto slider = dynamic_cast<juce::Slider*> (c->getComponent()))
            slider->getValueObject().referTo (v.getPropertyAsValue (c->propertie, um));

        else if (auto comboBox = dynamic_cast<juce::ComboBox*> (c->getComponent()))
            comboBox->getSelectedIdAsValue().referTo (v.getPropertyAsValue (c->propertie, um));
    }
}

int OscGui::getWidthNeeded()
{
    return 72 * 5;
}

int OscGui::getHeightNeeded()
{
    return 70 + 60 + 15;
}

//==============================================================================
LfoGui::LfoGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um)
{
    size_t i = 0;

    int itemId = 1;
    PopMenuParameters routing_options{
        {"Ch", {{itemId++, "Gain"}}},
        {"Osc", {{itemId++, "Freq"}, {itemId++, "Gain"}, {itemId++, "FM Freq"}, {itemId++, "FM Depth"}}},
        {"Filter", {{itemId++, "cutoff"}, {itemId++, "Reso"}, {itemId++, "Drive"}}},
    };

    comps[i++] = std::make_unique<ComboComp> (vs, um, IDs::selector, "", juce::StringArray{"1", "2", "3", "4"});

    comps[i++] =
        std::make_unique<ComboComp> (v, um, IDs::waveType, "", juce::StringArray{"sine", "saw", "square", "rand"});

    comps[i++] = std::make_unique<PopupComp> (v, um, IDs::route, "", routing_options);

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::freq, "Freq", juce::Range{param_limits.lfo_freq_min, param_limits.lfo_freq_max}, 0.001, 0.6, "Hz");

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::gain, "Gain", juce::Range{param_limits.lfo_gain_min, param_limits.lfo_gain_max}, 0.001, 0.3);

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        addAndMakeVisible (c->label);
        addAndMakeVisible (c->getComponent());
    }
}

void LfoGui::paint (juce::Graphics& g)
{
    setComponentGraphics (g, getLocalBounds(), "LFO");
}

void LfoGui::resized()
{
    auto boxes_bounds = getLocalBounds().withTrimmedLeft (gui_sizes.comp_title_font * 3);
    boxes_bounds.removeFromTop (5);
    auto slider_bounds = getLocalBounds().removeFromTop (5);
    int slider_xGap = 50;

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        if (c->propertie == IDs::selector)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), gui_sizes.selector_box_width),
                                        c->getPreferredHeight());
            c->getComponent()->setTopLeftPosition (boxes_bounds.removeFromLeft (gui_sizes.selector_box_width).getTopLeft());
            continue;
        }

        if (c->propertie == IDs::waveType)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), c->getPreferredWidth()),
                                        c->getPreferredHeight());
            c->getComponent()->setTopLeftPosition (boxes_bounds.removeFromLeft (c->getPreferredWidth()).getTopLeft());
            continue;
        }

        if (c->propertie == IDs::route)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), gui_sizes.route_box_width),
                                        c->getPreferredHeight());
            auto b = boxes_bounds.removeFromRight (gui_sizes.route_box_width).getTopRight();
            c->getComponent()->setTopRightPosition (b.getX() - 5, b.getY());
            continue;
        }

        c->getComponent()->setSize (juce::jmin (slider_bounds.getWidth(), c->getPreferredWidth()), c->getPreferredHeight());
        slider_bounds.removeFromLeft (slider_xGap);
        c->getComponent()->setCentrePosition (
            slider_bounds.removeFromLeft (c->getPreferredWidth()).getCentre().translated (0, 98));
    }
}

void LfoGui::setSelector (juce::ValueTree v, juce::UndoManager* um)
{
    for (auto& c : comps)
    {
        if (c->propertie == IDs::selector)
            continue;

        if (auto slider = dynamic_cast<juce::Slider*> (c->getComponent()))
            slider->getValueObject().referTo (v.getPropertyAsValue (c->propertie, um));

        else if (auto comboBox = dynamic_cast<juce::ComboBox*> (c->getComponent()))
            comboBox->getSelectedIdAsValue().referTo (v.getPropertyAsValue (c->propertie, um));
    }
}

int LfoGui::getWidthNeeded()
{
    return 300;
}

int LfoGui::getHeightNeeded()
{
    return 70 + 60 + 15;
}

//==============================================================================
FiltGui::FiltGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um)
{
    size_t i = 0;

    int itemID = 1;
    PopMenuParameters popParams{
        {"LP", {{itemID++, "LP12"}, {itemID++, "LP24"}}},
        {"BP", {{itemID++, "BP12"}, {itemID++, "BP24"}}},
        {"HP", {{itemID++, "HP12"}, {itemID++, "HP24"}}},
    };

    comps[i++] = std::make_unique<ComboComp> (vs, um, IDs::selector, "", juce::StringArray{"1", "2", "3", "4"});

    comps[i++] = std::make_unique<PopupComp> (v, um, IDs::filtType, "", popParams);

    comps[i++] = std::make_unique<SliderComp> (v, um, IDs::cutOff, "Cutoff",
                                               juce::Range{param_limits.filt_cutoff_min, param_limits.filt_cutoff_max},
                                               0.001, 0.4, "Hz");

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::reso, "Reso", juce::Range{param_limits.filt_reso_min, param_limits.filt_reso_max}, 0.001, 1);

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::drive, "Drive", juce::Range{param_limits.filt_drive_min, param_limits.filt_drive_max}, 0.001, 0.7);

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        addAndMakeVisible (c->label);
        addAndMakeVisible (c->getComponent());
    }

    onOff_btn.getToggleStateValue().referTo (v.getPropertyAsValue (IDs::enabled, um));
    addAndMakeVisible (onOff_btn);
}

void FiltGui::paint (juce::Graphics& g)
{
    setComponentGraphics (g, getLocalBounds(), "Filter");
}

void FiltGui::resized()
{
    auto boxes_bounds = getLocalBounds().withTrimmedLeft (gui_sizes.comp_title_font * 3);
    boxes_bounds.removeFromTop (5);
    auto slider_bounds = getLocalBounds().removeFromTop (5);

    auto btn_pos = getLocalBounds().withTrimmedTop (5).withTrimmedRight (5).getTopRight();
    onOff_btn.setSize (btn_width, btn_height);
    onOff_btn.setTopRightPosition (btn_pos.x, btn_pos.y);

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        if (c->propertie == IDs::selector)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), gui_sizes.selector_box_width),
                                        c->getPreferredHeight());
            c->getComponent()->setTopLeftPosition (boxes_bounds.removeFromLeft (gui_sizes.selector_box_width).getTopLeft());
            continue;
        }

        if (c->propertie == IDs::filtType)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), c->getPreferredWidth()),
                                        c->getPreferredHeight());
            c->getComponent()->setTopLeftPosition (boxes_bounds.removeFromLeft (c->getPreferredWidth()).getTopLeft());
            continue;
        }

        c->getComponent()->setSize (juce::jmin (slider_bounds.getWidth(), c->getPreferredWidth()), c->getPreferredHeight());
        c->getComponent()->setTopLeftPosition (
            slider_bounds.removeFromLeft (c->getPreferredWidth() + 2).getTopLeft().translated (0, 35 + 30));
    }
}

void FiltGui::setSelector (juce::ValueTree v, juce::UndoManager* um)
{

    onOff_btn.getToggleStateValue().referTo (v.getPropertyAsValue (IDs::enabled, um));

    for (auto& c : comps)
    {
        if (c->propertie == IDs::selector)
            continue;

        if (auto slider = dynamic_cast<juce::Slider*> (c->getComponent()))
            slider->getValueObject().referTo (v.getPropertyAsValue (c->propertie, um));

        else if (auto comboBox = dynamic_cast<juce::ComboBox*> (c->getComponent()))
            comboBox->getSelectedIdAsValue().referTo (v.getPropertyAsValue (c->propertie, um));
    }
}

int FiltGui::getWidthNeeded()
{
    return 72 * 3;
}

int FiltGui::getHeightNeeded()
{
    return 70 + 60 + 15;
}

//==============================================================================
DelayGui::DelayGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um)
{
    unsigned i = 0;

    comps[i++] = std::make_unique<ComboComp> (vs, um, IDs::selector, "", juce::StringArray{"1", "2", "3", "4"});

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::mix, "Dry/wet", juce::Range{param_limits.delay_mix_min, param_limits.delay_mix_max}, 0.001, 1);

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::time, "Time", juce::Range{param_limits.delay_time_min, param_limits.delay_time_max}, 0.001, 1);

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::feedback, "Feedback", juce::Range{param_limits.delay_feedback_min, param_limits.delay_feedback_max},
        0.001, 1);

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        addAndMakeVisible (c->label);
        addAndMakeVisible (c->getComponent());
    }
}

void DelayGui::paint (juce::Graphics& g)
{
    setComponentGraphics (g, getLocalBounds(), "DELAY");
}

void DelayGui::resized()
{
    auto boxes_bounds = getLocalBounds().withTrimmedLeft (gui_sizes.comp_title_font * 4);
    boxes_bounds.removeFromTop (5);
    auto slider_bounds = getLocalBounds().removeFromTop (5);
    int slider_xGap = 10;

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        if (c->propertie == IDs::selector)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), gui_sizes.selector_box_width),
                                        c->getPreferredHeight());
            c->getComponent()->setTopLeftPosition (boxes_bounds.removeFromLeft (gui_sizes.selector_box_width).getTopLeft());
            continue;
        }

        c->getComponent()->setSize (juce::jmin (slider_bounds.getWidth(), c->getPreferredWidth()), c->getPreferredHeight());
        slider_bounds.removeFromLeft (slider_xGap);
        c->getComponent()->setTopLeftPosition (
            slider_bounds.removeFromLeft (c->getPreferredWidth()).getTopLeft().translated (0, 35 + 30));
    }
}

void DelayGui::setSelector (juce::ValueTree v, juce::UndoManager* um)
{
    for (auto& c : comps)
    {
        if (c->propertie == IDs::selector)
            continue;

        if (auto slider = dynamic_cast<juce::Slider*> (c->getComponent()))
            slider->getValueObject().referTo (v.getPropertyAsValue (c->propertie, um));
    }
}

int DelayGui::getWidthNeeded()
{
    return 72 * 3 + 30;
}

int DelayGui::getHeightNeeded()
{
    return 70 + 60 + 15;
}
