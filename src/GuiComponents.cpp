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
    // g.setColour (juce::Colours::white);
    // g.drawRect (getLocalBounds(), 1);
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
OutputGui::OutputGui (const juce::ValueTree& v, juce::UndoManager& um, juce::ChangeListener* listener)
{

    for (unsigned i = 0; i < comps.size(); ++i)
    {
        if (i != 0)
        {
            const auto& ch_node = v.getChildWithName (IDs::Group::CHAN[i - 1]);
            comps[i] = std::make_unique<SliderComp> (ch_node, um, IDs::gain, "Ch" + std::to_string (i),
                                                     juce::Range{gui_params.chan_min, gui_params.chan_max}, 1, "dB");
        }
        else
            comps[i] = std::make_unique<SliderComp> (v, um, IDs::master, "Master",
                                                     juce::Range{gui_params.master_min, gui_params.master_max}, 1, "%");

        addAndMakeVisible (comps[i]->getComponent());
        addAndMakeVisible (comps[i]->label);
        comps[i]->addChangeListener (listener);
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
OscGui::OscGui (const juce::ValueTree& v, const juce::ValueTree& vs, juce::UndoManager& um,
                juce::ChangeListener* listener)
{
    unsigned i = 0;

    comps[i++] = std::make_unique<ChoiceComp> (vs, um, IDs::selector, "", juce::StringArray{"1", "2", "3", "4"});

    comps[i++] = std::make_unique<ChoiceComp> (v, um, IDs::wavetype, "",
                                               juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"});

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::freq, "Freq", juce::Range{gui_params.osc_freq_min, gui_params.osc_freq_max}, 0.4, "Hz");

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::gain, "Gain", juce::Range{gui_params.osc_gain_min, gui_params.osc_gain_max}, 3.0, "dB");

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::fm_freq, "FM freq", juce::Range{gui_params.osc_fm_freq_min, gui_params.osc_fm_freq_max}, 1, "Hz");

    comps[i] = std::make_unique<SliderComp> (
        v, um, IDs::fm_depth, "FM depth", juce::Range{gui_params.osc_fm_depth_min, gui_params.osc_fm_depth_max}, 0.4);

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        c->addChangeListener (listener);
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
            c->getComponent()->setTopLeftPosition (
                boxes_bounds.removeFromLeft (gui_sizes.selector_box_width).getTopLeft());
            continue;
        }

        if (c->propertie == IDs::wavetype)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), c->getPreferredWidth()),
                                        c->getPreferredHeight());
            c->getComponent()->setTopLeftPosition (boxes_bounds.removeFromLeft (c->getPreferredWidth()).getTopLeft());
            continue;
        }
        c->getComponent()->setSize (juce::jmin (slider_bounds.getWidth(), c->getPreferredWidth()),
                                    c->getPreferredHeight());
        c->getComponent()->setTopLeftPosition (
            slider_bounds.removeFromLeft (c->getPreferredWidth() + 2).getTopLeft().translated (0, 35 + 30));
    }
}

void OscGui::setSelector (const juce::ValueTree& v)
{
    for (auto& c : comps)
    {

        if (c->propertie == IDs::selector)
            continue;

        c->setState (v);

        if (auto slider = dynamic_cast<juce::Slider*> (c->getComponent()))
            slider->getValueObject().referTo (c->getState().getPropertyAsValue (c->propertie, c->getUndoManager()));

        else if (auto comboBox = dynamic_cast<juce::ComboBox*> (c->getComponent()))
            comboBox->getSelectedIdAsValue().referTo (
                c->getState().getPropertyAsValue (c->propertie, c->getUndoManager()));
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
LfoGui::LfoGui (const juce::ValueTree& v, const juce::ValueTree& vs, juce::UndoManager& um,
                juce::ChangeListener* listener)
{
    unsigned i = 0;

    comps[i++] = std::make_unique<ChoiceComp> (vs, um, IDs::selector, "", juce::StringArray{"1", "2", "3", "4"});

    comps[i++] = std::make_unique<ChoiceComp> (v, um, IDs::wavetype, "",
                                               juce::StringArray{"sine", "saw", "square", "wsine", "wsaw", "wsqr"});

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::freq, "Freq", juce::Range{gui_params.lfo_freq_min, gui_params.lfo_freq_max}, 0.6, "Hz");

    comps[i++] = std::make_unique<SliderComp> (v, um, IDs::gain, "Gain",
                                               juce::Range{gui_params.lfo_gain_min, gui_params.lfo_gain_max}, 1, "dB");

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        c->addChangeListener (listener);
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
    int slider_xGap = 10;

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        if (c->propertie == IDs::selector)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), gui_sizes.selector_box_width),
                                        c->getPreferredHeight());
            c->getComponent()->setTopLeftPosition (
                boxes_bounds.removeFromLeft (gui_sizes.selector_box_width).getTopLeft());
            continue;
        }

        if (c->propertie == IDs::wavetype)
        {
            c->getComponent()->setSize (juce::jmin (boxes_bounds.getWidth(), c->getPreferredWidth()),
                                        c->getPreferredHeight());
            c->getComponent()->setTopLeftPosition (boxes_bounds.removeFromLeft (c->getPreferredWidth()).getTopLeft());
            continue;
        }
        c->getComponent()->setSize (juce::jmin (slider_bounds.getWidth(), c->getPreferredWidth()),
                                    c->getPreferredHeight());
        slider_bounds.removeFromLeft (slider_xGap);
        c->getComponent()->setTopLeftPosition (
            slider_bounds.removeFromLeft (c->getPreferredWidth()).getTopLeft().translated (0, 35 + 30));
    }
}

void LfoGui::setSelector (const juce::ValueTree& v)
{
    for (auto& c : comps)
    {
        if (c->propertie == IDs::selector)
            continue;

        c->setState (v);

        if (auto slider = dynamic_cast<juce::Slider*> (c->getComponent()))
            slider->getValueObject().referTo (c->getState().getPropertyAsValue (c->propertie, c->getUndoManager()));

        else if (auto comboBox = dynamic_cast<juce::ComboBox*> (c->getComponent()))
            comboBox->getSelectedIdAsValue().referTo (
                c->getState().getPropertyAsValue (c->propertie, c->getUndoManager()));
    }
}

int LfoGui::getWidthNeeded()
{

    return 180;
}

int LfoGui::getHeightNeeded()
{
    return 70 + 60 + 15;
}

//==============================================================================
DelayGui::DelayGui (const juce::ValueTree& v, const juce::ValueTree& vs, juce::UndoManager& um,
                    juce::ChangeListener* listener)
{
    unsigned i = 0;

    comps[i++] = std::make_unique<ChoiceComp> (vs, um, IDs::selector, "", juce::StringArray{"1", "2", "3", "4"});

    comps[i++] = std::make_unique<SliderComp> (v, um, IDs::mix, "Dry/wet",
                                               juce::Range{gui_params.delay_mix_min, gui_params.delay_mix_max}, 1);

    comps[i++] = std::make_unique<SliderComp> (v, um, IDs::time, "Time",
                                               juce::Range{gui_params.delay_time_min, gui_params.delay_time_max}, 1);

    comps[i++] = std::make_unique<SliderComp> (
        v, um, IDs::feedback, "Feedback", juce::Range{gui_params.delay_feedback_min, gui_params.delay_feedback_max}, 1);

    for (auto& c : comps)
    {
        if (c == nullptr)
            continue;

        c->addChangeListener (listener);
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
            c->getComponent()->setTopLeftPosition (
                boxes_bounds.removeFromLeft (gui_sizes.selector_box_width).getTopLeft());
            continue;
        }

        c->getComponent()->setSize (juce::jmin (slider_bounds.getWidth(), c->getPreferredWidth()),
                                    c->getPreferredHeight());
        slider_bounds.removeFromLeft (slider_xGap);
        c->getComponent()->setTopLeftPosition (
            slider_bounds.removeFromLeft (c->getPreferredWidth()).getTopLeft().translated (0, 35 + 30));
    }
}

void DelayGui::setSelector (const juce::ValueTree& v)
{
    for (auto& c : comps)
    {
        if (c->propertie == IDs::selector)
            continue;

        c->setState (v);

        if (auto slider = dynamic_cast<juce::Slider*> (c->getComponent()))
            slider->getValueObject().referTo (c->getState().getPropertyAsValue (c->propertie, c->getUndoManager()));
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