#include "RandSequencer.h"

RandSequencer::RandSequencer (std::function<void()> func) : setRandParams (func)
{
    time = 1000;
    enabled = false;
}

void RandSequencer::start()
{
    startTimer (time);
}

void RandSequencer::stop()
{
    stopTimer();
}

void RandSequencer::setEnabled (const bool b)
{
    enabled = b;
}

void RandSequencer::setTime (const int t)
{
    time = t;
    start();
}

void RandSequencer::timerCallback()
{
    if (enabled)
        setRandParams();
}
