#pragma once
#include <juce_core/juce_core.h>
class ResearchWindow;
struct TrustedFrustProof;
bool runFoundationCheck(const juce::File& directory);
bool runRestorationCheck(ResearchWindow&, const juce::File&, const TrustedFrustProof&);
