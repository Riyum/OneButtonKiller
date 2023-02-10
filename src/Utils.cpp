#include "Utils.h"

juce::ValueTree createDefaultTree()
{
    juce::ValueTree output = {IDs::OUTPUT_GAIN,
                              {},
                              {{IDs::MASTER, {{IDs::gain, def_param.master_gain}}},
                               {IDs::CHAN1, {{IDs::gain, def_param.chan_gain}}},
                               {IDs::CHAN2, {{IDs::gain, def_param.chan_gain}}}}};

    juce::ValueTree osc = {IDs::OSC,
                           {},
                           {
                               {IDs::OSC1,
                                {{IDs::wavetype, def_param.osc_wavetype},
                                 {IDs::freq, def_param.osc_freq},
                                 {IDs::gain, def_param.osc_gain},
                                 {IDs::fm_freq, def_param.osc_fm_freq},
                                 {IDs::fm_depth, def_param.osc_fm_depth}}},
                               {IDs::OSC2,
                                {{IDs::wavetype, def_param.osc_wavetype},
                                 {IDs::freq, def_param.osc_freq},
                                 {IDs::gain, def_param.osc_gain},
                                 {IDs::fm_freq, def_param.osc_fm_freq},
                                 {IDs::fm_depth, def_param.osc_fm_depth}}},
                           }};

    juce::ValueTree lfo = {IDs::LFO,
                           {},
                           {
                               {IDs::LFO1,
                                {{IDs::wavetype, def_param.lfo_wavetype},
                                 {IDs::freq, def_param.lfo_freq},
                                 {IDs::gain, def_param.lfo_gain}}},
                               {IDs::LFO2,
                                {{IDs::wavetype, def_param.lfo_wavetype},
                                 {IDs::freq, def_param.lfo_freq},
                                 {IDs::gain, def_param.lfo_gain}}},
                           }};

    juce::ValueTree delay = {
        IDs::DELAY,
        {},
        {
            {IDs::DELAY1,
             {{IDs::mix, def_param.del_mix}, {IDs::time, def_param.del_time}, {IDs::feedback, def_param.del_feedback}}},
            {IDs::DELAY2,
             {{IDs::mix, def_param.del_mix}, {IDs::time, def_param.del_time}, {IDs::feedback, def_param.del_feedback}}},
        }};

    juce::ValueTree root (IDs::ROOT);
    root.addChild (output, -1, nullptr);
    root.addChild (osc, -1, nullptr);
    root.addChild (lfo, -1, nullptr);
    root.addChild (delay, -1, nullptr);

    return root;
}
//==============================================================================
BaseComp::BaseComp (const juce::ValueTree& v, const juce::Identifier p, juce::UndoManager& um,
                    const juce::String& labelName)
    : name (labelName), state (v), undoManager (um)
{
    state.addListener (this);
    prop = p;
}

juce::ValueTree BaseComp::getState() const
{
    return state;
}

juce::UndoManager* BaseComp::getUndoManager() const
{
    return &undoManager;
}
void BaseComp::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& p)
{
    juce::ignoreUnused (v);
    if (prop == p)
    {
        sendChangeMessage();
    }
}
// void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree&) override;
// void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree&, int) override;
// void valueTreeChildOrderChanged (juce::ValueTree& parentTree, int, int) override;
// void valueTreeParentChanged (juce::ValueTree&) override;
// void treeChildrenChanged (const juce::ValueTree& parentTree) override;

//==============================================================================
SliderComp::SliderComp (const juce::ValueTree& v, const juce::Identifier propertie, juce::UndoManager& um,
                        const juce::String& labelName, juce::Range<double> range, double skew, double initialValue,
                        const juce::String& suffix)
    : BaseComp (v, propertie, um, labelName)
{
    slider.setRange (range.getStart(), range.getEnd(), 0.001);
    slider.setSliderStyle (juce::Slider::SliderStyle::Rotary);
    slider.setTextBoxStyle (juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 60, 20);
    slider.setSkewFactor (skew);
    slider.setValue (initialValue);

    if (suffix.isNotEmpty())
        slider.setTextValueSuffix (suffix);

    slider.getValueObject().referTo (getState().getPropertyAsValue (propertie, getUndoManager()));
}

juce::Component* SliderComp::getComponent()
{
    return &slider;
}

int SliderComp::getPreferredHeight()
{
    return 70;
}
int SliderComp::getPreferredWidth()
{
    return 70;
}

double SliderComp::getCurrentValue() const
{
    return slider.getValue();
}

//==============================================================================
ChoiceComp::ChoiceComp (const juce::ValueTree& v, const juce::Identifier propertie, juce::UndoManager& um,
                        const juce::String& labelName, const juce::StringArray& options, int initialId)
    : BaseComp (v, propertie, um, labelName)
{
    parameterBox.addItemList (options, 1);
    parameterBox.setSelectedId (initialId);

    parameterBox.getSelectedIdAsValue().referTo (getState().getPropertyAsValue (propertie, getUndoManager()));
}

juce::Component* ChoiceComp::getComponent()
{
    return &parameterBox;
}

int ChoiceComp::getPreferredHeight()
{
    return 25;
}
int ChoiceComp::getPreferredWidth()
{
    return 100;
}

int ChoiceComp::getCurrentValue() const
{
    return parameterBox.getSelectedId();
}

//==============================================================================
LabelComp::LabelComp (const juce::ValueTree& v, const juce::Identifier propertie, juce::UndoManager& um,
                      const juce::String& labelName)
    : BaseComp (v, propertie, um, labelName)
{
    label.setFont (juce::Font (15, juce::Font::FontStyleFlags::bold | juce::Font::FontStyleFlags::italic));
    label.setText (labelName, juce::dontSendNotification);
}

juce::Component* LabelComp::getComponent()
{
    return &label;
}

int LabelComp::getPreferredHeight()
{
    return 20;
}
int LabelComp::getPreferredWidth()
{
    return 100;
}

juce::String LabelComp::getCurrentValue() const
{

    return label.getText();
}

//==============================================================================
ControlComponent::ControlComponent (std::vector<std::unique_ptr<BaseComp>>& params)
{
    controls = std::move (params);

    for (auto&& param : controls)
    {
        addAndMakeVisible (param->getComponent());

        if (instanceof <LabelComp> (param.get()))
            continue;

        std::unique_ptr<juce::Label> paramLabel = std::make_unique<juce::Label> ("", param->name);
        if (instanceof <SliderComp> (param.get()))
        {
            paramLabel->attachToComponent (param->getComponent(), false);
            paramLabel->setJustificationType (juce::Justification::centredBottom);
        }
        else
        {
            paramLabel->attachToComponent (param->getComponent(), true);
            paramLabel->setJustificationType (juce::Justification::centredLeft);
        }
        addAndMakeVisible (paramLabel.get());
        labels.push_back (std::move (paramLabel));
    }
}

void ControlComponent::resized()
{
    auto bounds = getLocalBounds(), slider_bounds = bounds;
    juce::Rectangle<int> compBounds;
    int non_slider_gap = 30, slider_gap = 90;

    for (auto&& p : controls)
    {
        auto* comp = p->getComponent();

        comp->setSize (juce::jmin (bounds.getWidth(), p->getPreferredWidth()), p->getPreferredHeight());

        if (! instanceof <SliderComp> (p.get()))
        {
            compBounds = bounds.removeFromTop (p->getPreferredHeight() + non_slider_gap);
            slider_bounds = bounds.removeFromTop (slider_gap);
        }
        else
            compBounds = slider_bounds.removeFromLeft (p->getPreferredWidth());

        if (instanceof <ChoiceComp> (p.get()))
            comp->setTopLeftPosition (compBounds.getTopLeft().translated (p->name.length() * 8, 0));
        else
            comp->setTopLeftPosition (compBounds.getTopLeft());

        // if (instanceof <LabelComp> (p.get()))
        //     comp->setTopLeftPosition (compBounds.getTopLeft());
        // else
        //     comp->setCentrePosition (compBounds.getCentre());
    }
}

int ControlComponent::getWidthNeeded()
{
    return 4 * 70;

    // auto width = 0;

    // for (auto&& p : controls)
    //     width = std::max (p->getPreferredWidth(), width);

    // width + lable_width + gap
    // return width + 100 + 10;
}

int ControlComponent::getHeightNeeded()
{
    // (slider height + slider box height + label height) * number of components that have sliders
    int slider = (70 + 20 + 20) * 7;
    int combox = 25 * 4;
    int label = 20 * 3;

    return slider + combox + label + 60;

    // auto height = 0;

    // for (auto&& p : controls)
    //     height += p->getPreferredHeight();

    // return height + 10;
}

//==============================================================================
