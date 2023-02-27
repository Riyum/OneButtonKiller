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

    juce::StringArray str{"Magic", "Undo", "Redo"};

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
        c->setSize (btn_width, btn_height);
        c->setTopLeftPosition (bounds.removeFromLeft (btn_width + btn_gap).getTopLeft());
    }
}

int ButtonsGui::getWidthNeeded()
{
    return (btn_width + btn_gap) * NUM_OF_COMPONENTS;
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
                                                     juce::Range{param_limits.chan_min, param_limits.chan_max}, 3, "dB");
        }
        else
            comps[i] = std::make_unique<SliderComp> (
                v, um, IDs::master, "Master", juce::Range{param_limits.master_min, param_limits.master_max}, 1, "%");

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
OscGui::OscGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um)
{
    unsigned i = 0;

    comps[i++] = std::make_unique<ComboComp> (vs, um, IDs::selector, "", juce::StringArray{"1", "2", "3", "4"});

    comps[i++] = std::make_unique<ComboComp> (v, um, IDs::wavetype, "",
                                              juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"});

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::freq, "Freq", juce::Range{param_limits.osc_freq_min, param_limits.osc_freq_max}, 0.4, "Hz");

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::gain, "Gain", juce::Range{param_limits.osc_gain_min, param_limits.osc_gain_max}, 3.0, "dB");

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::fm_freq, "FM freq", juce::Range{param_limits.osc_fm_freq_min, param_limits.osc_fm_freq_max}, 1, "Hz");

    comps[i] = std::make_unique<SliderComp> (
        v, um, IDs::fm_depth, "FM depth", juce::Range{param_limits.osc_fm_depth_min, param_limits.osc_fm_depth_max}, 0.3);

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

        if (c->propertie == IDs::wavetype)
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
    return 72 * 4;
}

int OscGui::getHeightNeeded()
{
    return 70 + 60 + 15;
}

//==============================================================================
LfoGui::LfoGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um, const juce::StringArray& comp_des,
                std::vector<MenuItems> _route_options, const std::function<void (std::vector<MenuItems>&, const int)> func)
    : route_options (_route_options), updateLfoRouteOptions (func)
{
    unsigned i = 0;

    comps[i++] = std::make_unique<ComboComp> (vs, um, IDs::selector, "", juce::StringArray{"1", "2", "3", "4"});

    comps[i++] = std::make_unique<ComboComp> (v, um, IDs::wavetype, "",
                                              juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"});

    comps[i++] = std::make_unique<PopupComp> (v, um, IDs::route, "", comp_des, route_options);

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::freq, "Freq", juce::Range{param_limits.lfo_freq_min, param_limits.lfo_freq_max}, 0.6, "Hz");

    comps[i++] = std::make_unique<SliderComp> (v, um, IDs::gain, "Gain",
                                               juce::Range{param_limits.lfo_gain_min, param_limits.lfo_gain_max}, 0.3);

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

        if (c->propertie == IDs::wavetype)
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
        {
            if (c->propertie == IDs::route)
            {
                auto menu = dynamic_cast<PopupComp*> (c.get());
                updateLfoRouteOptions (route_options, v.getParent().indexOf (v));
                menu->updateMenu (route_options);
            }
            comboBox->getSelectedIdAsValue().referTo (v.getPropertyAsValue (c->propertie, um));
        }
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
DelayGui::DelayGui (juce::ValueTree& v, juce::ValueTree& vs, juce::UndoManager* um)
{
    unsigned i = 0;

    comps[i++] = std::make_unique<ComboComp> (vs, um, IDs::selector, "", juce::StringArray{"1", "2", "3", "4"});

    comps[i++] = std::make_unique<SliderComp> (v, um, IDs::mix, "Dry/wet",
                                               juce::Range{param_limits.delay_mix_min, param_limits.delay_mix_max}, 1);

    comps[i++] = std::make_unique<SliderComp> (v, um, IDs::time, "Time",
                                               juce::Range{param_limits.delay_time_min, param_limits.delay_time_max}, 1);

    comps[i++] =
        std::make_unique<SliderComp> (v, um, IDs::feedback, "Feedback",
                                      juce::Range{param_limits.delay_feedback_min, param_limits.delay_feedback_max}, 1);

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
