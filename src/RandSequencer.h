#pragma once

#include <JuceHeader.h>
#include <functional>

class RandSequencer : private juce::Timer
{
public:
    RandSequencer (std::function<void()> func);
    void start();
    void stop();

    void setEnabled (const bool b);
    void setTime (const int t);

private:
    int time;
    bool enabled;
    std::function<void()> setRandParams;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RandSequencer)
};
