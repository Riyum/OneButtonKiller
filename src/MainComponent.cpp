#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent (const int num_input_channels, const int num_output_channels)
// : adsc (deviceManager, 0, num_input_channels, 0, num_output_channels, false, false, true, false)
{
    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted)
                                           { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        setAudioChannels (num_input_channels, num_output_channels);
    }

    initParameters();
    // addAndMakeVisible (adsc);

    setSize (700, 450);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();

    /* for now, removing listners from the paramters is done in ParametersComponent destructor */
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::dsp::ProcessSpec spec;

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlockExpected;
    spec.numChannels = 1;

    for (int i = 0; i < 2; i++)
    {
        chain1[i].prepare (spec);
        chain2[i].prepare (spec);
    }

    lfo1.prepare ({ spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels });
    lfo2.prepare ({ spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels });
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    juce::dsp::AudioBlock<float> audioBlock { *bufferToFill.buffer };

    auto chan1_block = audioBlock.getSubsetChannelBlock (0, 2);
    auto chan2_block = audioBlock.getSubsetChannelBlock (2, 2);

    static const auto max_freq = 22000.f;
    float lfo1_range = max_freq - osc1_freq;
    float lfo2_range = max_freq - osc2_freq;

    for (auto samp = 0; samp < bufferToFill.numSamples; ++samp)
    {
        // auto noise = random.nextFloat() * 2.0f - 1.0f;

        // mainOutputLeft[samp] = noise * chan1_gain * mod * master_gain;
        // mainOutputRight[samp] = noise * chan1_gain * mod * master_gain;

        if (lfoUpdateCounter == 0)
        {
            lfoUpdateCounter = lfoUpdateRate;

            auto lfoOut1 = lfo1.processSample (0.0f);
            float mod1 = juce::jmap (lfoOut1, -1.0f, 1.0f, static_cast<float> (osc1_freq), lfo1_range);
            chain1[0].get<oscIdx>().setFrequency (osc1_freq + mod1 * lfo1_gain);
            chain1[1].get<oscIdx>().setFrequency (osc1_freq + mod1 * lfo1_gain);

            auto lfoOut2 = lfo2.processSample (0.0f);
            float mod2 = juce::jmap (lfoOut2, -1.0f, 1.0f, static_cast<float> (osc2_freq), lfo2_range);
            chain2[0].get<oscIdx>().setFrequency (osc2_freq + mod2 * lfo2_gain);
            chain2[1].get<oscIdx>().setFrequency (osc2_freq + mod2 * lfo2_gain);
        }
        --lfoUpdateCounter;
    }

    for (unsigned i = 0; i < 2; i++)
    {
        auto block1 = chan1_block.getSingleChannelBlock (i);
        auto block2 = chan2_block.getSingleChannelBlock (i);

        chain1[i].process (juce::dsp::ProcessContextReplacing<float> (block1));
        chain2[i].process (juce::dsp::ProcessContextReplacing<float> (block2));
    }
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    ParamId id = static_cast<ParamId> (dynamic_cast<ParameterBase*> (source)->id);

    switch (id)
    {
        case ParamId::MASTER:
            master_gain = static_cast<float> (params->getParam<double> (ParamId::MASTER));
            return;

        case ParamId::CHAN1_GAIN:
            chan1_gain = static_cast<float> (params->getParam<double> (ParamId::CHAN1_GAIN));
            chain1[0].get<gainIdx>().setGainDecibels (chan1_gain);
            chain1[1].get<gainIdx>().setGainDecibels (chan1_gain);
            return;
        case ParamId::CHAN2_GAIN:
            chan2_gain = static_cast<float> (params->getParam<double> (ParamId::CHAN2_GAIN));
            chain2[0].get<gainIdx>().setGainDecibels (chan2_gain);
            chain2[1].get<gainIdx>().setGainDecibels (chan2_gain);
            return;

        case ParamId::OSC1_WAVETYPE:
            chain1[0].get<oscIdx>().setWaveType (static_cast<WaveType> (params->getParam<int> (ParamId::OSC1_WAVETYPE)));
            chain1[1].get<oscIdx>().setWaveType (static_cast<WaveType> (params->getParam<int> (ParamId::OSC1_WAVETYPE)));
            return;
        case ParamId::OSC1_FREQ:
            osc1_freq = params->getParam<double> (ParamId::OSC1_FREQ);
            chain1[0].get<oscIdx>().setFrequency (osc1_freq);
            chain1[1].get<oscIdx>().setFrequency (osc1_freq);
            return;
        case ParamId::OSC1_GAIN:
            chain1[0].get<oscIdx>().setLevel (static_cast<float> (params->getParam<double> (ParamId::OSC1_GAIN)));
            chain1[1].get<oscIdx>().setLevel (static_cast<float> (params->getParam<double> (ParamId::OSC1_GAIN)));
            return;

        case ParamId::OSC2_WAVETYPE:
            chain2[0].get<oscIdx>().setWaveType (static_cast<WaveType> (params->getParam<int> (ParamId::OSC2_WAVETYPE)));
            chain2[1].get<oscIdx>().setWaveType (static_cast<WaveType> (params->getParam<int> (ParamId::OSC2_WAVETYPE)));
            return;
        case ParamId::OSC2_FREQ:
            osc2_freq = params->getParam<double> (ParamId::OSC2_FREQ);
            chain2[0].get<oscIdx>().setFrequency (osc2_freq);
            chain2[1].get<oscIdx>().setFrequency (osc2_freq);
            return;
        case ParamId::OSC2_GAIN:
            chain2[0].get<oscIdx>().setLevel (static_cast<float> (params->getParam<double> (ParamId::OSC2_GAIN)));
            chain2[1].get<oscIdx>().setLevel (static_cast<float> (params->getParam<double> (ParamId::OSC2_GAIN)));
            return;

        case ParamId::LFO1_WAVETYPE:
            lfo1.setWaveType (static_cast<WaveType> (params->getParam<int> (ParamId::LFO1_WAVETYPE)));
            return;
        case ParamId::LFO1_FREQ:
            lfo1_freq = params->getParam<double> (ParamId::LFO1_FREQ);
            lfo1.setFrequency (lfo1_freq);
            return;
        case ParamId::LFO1_GAIN:
            lfo1_gain = static_cast<float> (params->getParam<double> (ParamId::LFO1_GAIN));
            lfo1.setLevel (lfo1_gain);
            return;

        case ParamId::LFO2_WAVETYPE:
            lfo2.setWaveType (static_cast<WaveType> (params->getParam<int> (ParamId::LFO2_WAVETYPE)));
            return;
        case ParamId::LFO2_FREQ:
            lfo2_freq = params->getParam<double> (ParamId::LFO2_FREQ);
            lfo2.setFrequency (lfo2_freq);
            return;
        case ParamId::LFO2_GAIN:
            lfo2_gain = static_cast<float> (params->getParam<double> (ParamId::LFO2_GAIN));
            lfo2.setLevel (lfo2_gain);
            return;
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

void MainComponent::initParameters()
{
    /*
    **         Don't forget to update ParamsId in Utils.h
     */

    std::vector<std::unique_ptr<ParameterBase>> parameters;

    // Master Gain
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { 0.0, 1.0 }, 1, 0.8, ParamId::MASTER, "Master", "%"));

    // Channels
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { -100.0, 0.0 }, 1, -20.0, ParamId::CHAN1_GAIN, "ch1 Gain", "dB"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { -100.0, 0.0 }, 1, -20.0, ParamId::CHAN2_GAIN, "ch2 Gain", "dB"));

    // OSC
    parameters.push_back (std::make_unique<ChoiceParameter> (juce::StringArray { "sine", "saw", "square", "wsine", "wsaw", "wsqr" }, 3, ParamId::OSC1_WAVETYPE, "OSC1 Wave type"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { 0.0, 22000.0 }, 0.4, 440.0, ParamId::OSC1_FREQ, "Freq", "Hz"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { -100.0, 20.0 }, 3.0, -100.0, ParamId::OSC1_GAIN, "Gain", "dB"));

    parameters.push_back (std::make_unique<ChoiceParameter> (juce::StringArray { "sine", "saw", "square", "wsine", "wsaw", "wsqr" }, 3, ParamId::OSC2_WAVETYPE, "OSC2 Wave type"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { 0.0, 22000.0 }, 0.4, 440.0, ParamId::OSC2_FREQ, "Freq", "Hz"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { -100.0, 20.0 }, 3.0, -100.0, ParamId::OSC2_GAIN, "Gain", "dB"));

    // LFO
    parameters.push_back (std::make_unique<ChoiceParameter> (juce::StringArray { "sine", "saw", "square", "wsine", "wsaw", "wsqr" }, 3, ParamId::LFO1_WAVETYPE, "LFO1 Wave type"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { 0.0, 70.0 }, 0.6, 0.0, ParamId::LFO1_FREQ, "Freq", "Hz"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { 0.0, 1.0 }, 0.3, 0.0, ParamId::LFO1_GAIN, "Gain"));

    parameters.push_back (std::make_unique<ChoiceParameter> (juce::StringArray { "sine", "saw", "square", "wsine", "wsaw", "wsqr" }, 3, ParamId::LFO2_WAVETYPE, "LFO2 Wave type"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { 0.0, 70.0 }, 0.6, 0.0, ParamId::LFO2_FREQ, "Freq", "Hz"));
    parameters.push_back (std::make_unique<SliderParameter> (juce::Range { 0.0, 1.0 }, 0.3, 0.0, ParamId::LFO2_GAIN, "Gain"));

    for (auto& p : parameters)
        p->addChangeListener (this);

    params = std::make_unique<ParametersComponent> (parameters);
    addAndMakeVisible (params.get());
    resized();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

    auto bounds = getLocalBounds();

    if (params.get() != nullptr)
        params->setBounds (bounds.removeFromTop (params->getHeightNeeded()).reduced (20, 0));
}
