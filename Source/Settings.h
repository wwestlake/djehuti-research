#pragma once
#include <juce_core/juce_core.h>

class ResearchSettings
{
public:
    explicit ResearchSettings(juce::File directory);
    static juce::File defaultDirectory();
    const juce::var& snapshot() const { return data; }
    juce::String projectDirectory() const { return data["projectDirectory"].toString(); }
    juce::String warning() const { return loadWarning; }
    const juce::File& file() const { return settingsFile; }
    bool setProjectDirectory(const juce::String& path, juce::String& error);
    bool saveWorkspace(const juce::String& windowState, const juce::var& layout, juce::String& error);
    static bool validateProjectDirectory(const juce::String&, juce::String& error, bool probeWrite = true);
private:
    bool commit(juce::var candidate, juce::String& error);
    juce::File settingsFile;
    juce::var data;
    juce::String loadWarning;
    bool readable = true;
};
