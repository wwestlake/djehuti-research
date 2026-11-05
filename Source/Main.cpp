#include "ResearchWorkspace.h"
#include "FoundationCheck.h"
#include <crtdbg.h>

class ResearchApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Djehuti Research"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }
    void initialise(const juce::String& commandLine) override
    {
        const auto args = juce::StringArray::fromTokens(commandLine, true);
        if (args.size() == 2 && args[0] == "--foundation-check")
        {
            _CrtSetReportMode(_CRT_ASSERT, 0); _CrtSetReportMode(_CRT_ERROR, 0);
            const bool passed = runFoundationCheck(juce::File(args[1].unquoted()));
            setApplicationReturnValue(passed ? 0 : 1); quit(); return;
        }
        auto directory = ResearchSettings::defaultDirectory();
        if (args.size() == 2 && args[0] == "--settings-dir") directory = juce::File(args[1].unquoted());
        const bool verifyRestored = args.size() == 2 && args[0] == "--verify-restored";
        juce::File fixture;
        if (verifyRestored) { fixture = juce::File(args[1].unquoted()); directory = fixture.getChildFile("settings"); }
        settings = std::make_unique<ResearchSettings>(directory);
        window = std::make_unique<ResearchWindow>(*settings);
        window->onClose = [this] { quit(); };
        window->setVisible(true);
        if (verifyRestored)
        {
            window->workspace().onProofFinished = [this, fixture](const TrustedFrustProof& proof) {
                const bool passed = runRestorationCheck(*window, fixture, proof);
                setApplicationReturnValue(passed ? 0 : 1);
                window->closeButtonPressed();
            };
            juce::Timer::callAfterDelay(100, [this] { if (window) window->workspace().runSelfTest(); });
        }
    }
    void shutdown() override
    {
        if (window) { juce::String error; window->saveState(error); }
        window.reset(); settings.reset(); // joins the trusted proof worker and destroys any floating windows
    }
    void systemRequestedQuit() override { if (window) window->closeButtonPressed(); else quit(); }
    void anotherInstanceStarted(const juce::String&) override { if (window) { window->setMinimised(false); window->toFront(true); } }
private:
    std::unique_ptr<ResearchSettings> settings;
    std::unique_ptr<ResearchWindow> window;
};
START_JUCE_APPLICATION(ResearchApplication)
