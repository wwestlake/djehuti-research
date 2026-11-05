#pragma once
#include <juce_core/juce_core.h>

struct TrustedFrustProof
{
    bool answerPassed = false, diagnosticPassed = false, lifecyclePassed = false;
    juce::String log, diagnostic;
    juce::int64 answer = 0;
    int cycles = 0;
    bool passed() const { return answerPassed && diagnosticPassed && lifecyclePassed; }
};
TrustedFrustProof runTrustedFrustProof();
