#include "Utils.h"

juce::ValueTree createDefaultTree()
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
    float lfo_gain = 0; // linear

    float del_mix = 0;

    juce::ValueTree output = {IDs::OUTPUT_GAIN,
                              {},
                              {{IDs::MASTER, {{IDs::gain, master_gain}}},
                               {IDs::CHAN1, {{IDs::gain, chan_gain}}},
                               {IDs::CHAN2, {{IDs::gain, chan_gain}}}}};

    juce::ValueTree osc = {IDs::OSC,
                           {},
                           {
                               {IDs::OSC1,
                                {{IDs::wavetype, osc_wavetype},
                                 {IDs::freq, osc_freq},
                                 {IDs::gain, osc_gain},
                                 {IDs::fm_freq, osc_fm_freq},
                                 {IDs::fm_depth, osc_fm_depth}}},
                               {IDs::OSC2,
                                {{IDs::wavetype, osc_wavetype},
                                 {IDs::freq, osc_freq},
                                 {IDs::gain, osc_gain},
                                 {IDs::fm_freq, osc_fm_freq},
                                 {IDs::fm_depth, osc_fm_depth}}},
                           }};

    juce::ValueTree lfo = {
        IDs::LFO,
        {},
        {
            {IDs::LFO1, {{IDs::wavetype, lfo_wavetype}, {IDs::freq, lfo_freq}, {IDs::gain, lfo_gain}}},
            {IDs::LFO2, {{IDs::wavetype, lfo_wavetype}, {IDs::freq, lfo_freq}, {IDs::gain, lfo_gain}}},
        }};

    juce::ValueTree delay = {IDs::DELAY,
                             {},
                             {
                                 {IDs::DELAY1, {{IDs::mix, del_mix}}},
                                 {IDs::DELAY2, {{IDs::mix, del_mix}}},
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
    return 40;
}
int SliderComp::getPreferredWidth()
{
    return 500;
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
    return 250;
}

int ChoiceComp::getCurrentValue() const
{
    return parameterBox.getSelectedId();
}

//==============================================================================
ControlComponent::ControlComponent (std::vector<std::unique_ptr<BaseComp>>& params)
{
    controls = std::move (params);

    for (auto&& param : controls)
    {
        addAndMakeVisible (param->getComponent());

        if (instanceof <juce::TextButton> (param.get()))
            continue;

        std::unique_ptr<juce::Label> paramLabel = std::make_unique<juce::Label> ("", param->name);
        paramLabel->attachToComponent (param->getComponent(), true);
        paramLabel->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (paramLabel.get());
        labels.push_back (std::move (paramLabel));
    }
}

void ControlComponent::resized()
{
    auto bounds = getLocalBounds();

    for (auto&& p : controls)
    {
        auto* comp = p->getComponent();

        comp->setSize (juce::jmin (bounds.getWidth(), p->getPreferredWidth()), p->getPreferredHeight());

        auto compBounds = bounds.removeFromTop (p->getPreferredHeight());
        comp->setCentrePosition (compBounds.getCentre());
    }
}

int ControlComponent::getWidthNeeded()
{
    auto width = 0;

    for (auto&& p : controls)
        width = std::max (p->getPreferredWidth(), width);

    // width + lable_width + gap
    return width + 10;
}

int ControlComponent::getHeightNeeded()
{
    auto height = 0;

    for (auto&& p : controls)
        height += p->getPreferredHeight();

    return height + 10;
}

//==============================================================================
