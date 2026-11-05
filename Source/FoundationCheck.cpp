#include "FoundationCheck.h"
#include "ResearchWorkspace.h"

namespace
{
bool saveNativeSnapshot(ResearchWindow& window, const juce::File& file)
{
    auto stream = file.createOutputStream();
    juce::PNGImageFormat png;
    return stream != nullptr && png.writeImageToStream(window.workspace().createComponentSnapshot(window.workspace().getLocalBounds()), *stream);
}
}

bool runFoundationCheck(const juce::File& directory)
{
    juce::Array<juce::var> checks;
    bool passed = true;
    auto check = [&](bool ok, const juce::String& name, const juce::String& detail = {}) {
        auto* value = new juce::DynamicObject(); value->setProperty("name", name); value->setProperty("passed", ok); value->setProperty("detail", detail); checks.add(juce::var(value)); passed &= ok;
    };
    directory.createDirectory();
    const auto chosen = directory.getChildFile("chosen"); chosen.createDirectory();
    const auto config = directory.getChildFile("settings");
    juce::String savedWindow, savedLayout, error;
    const auto proof = runTrustedFrustProof();
    {
        ResearchSettings settings(config);
        ResearchWindow window(settings); window.setVisible(true);
        auto& workspace = window.workspace();
        workspace.appendLog(proof.log);
        check(saveNativeSnapshot(window, directory.getChildFile("workspace-default.png")), "Native JUCE workspace rendering saved");
        check(workspace.documentTabCount() == 1, "One descriptive empty-state document tab");
        workspace.addEmptyDocumentTab(); workspace.addEmptyDocumentTab();
        auto* tabs = dynamic_cast<juce::TabbedComponent*>(workspace.docking().findPanelById("documents")->getContent());
        check(workspace.documentTabCount() == 3 && tabs != nullptr && tabs->getCurrentTabIndex() == 2, "Multiple document tabs open and select");
        if (tabs != nullptr) tabs->setCurrentTabIndex(0);
        check(tabs != nullptr && tabs->getCurrentTabIndex() == 0, "Switch back to an existing document tab");
        check(saveNativeSnapshot(window, directory.getChildFile("workspace-tabs.png")), "Multiple-tab workspace rendering saved");
        workspace.closeCurrentDocumentTab(); check(workspace.documentTabCount() == 2, "Current empty document tab closes");
        check(workspace.applyProjectDirectory(chosen.getFullPathName()), "Writable local project directory is applied", workspace.statusText());
        const auto before = settings.file().loadFileAsString();
        check(!workspace.applyProjectDirectory("relative-path") && before == settings.file().loadFileAsString(), "Relative path is rejected without changing saved settings", workspace.statusText());
        check(!workspace.applyProjectDirectory(directory.getChildFile("does-not-exist").getFullPathName()) && before == settings.file().loadFileAsString(), "Missing directory is rejected without creation", workspace.statusText());
        check(!workspace.applyProjectDirectory(settings.file().getFullPathName()) && before == settings.file().loadFileAsString(), "A file is rejected as a directory", workspace.statusText());
        check(!workspace.applyProjectDirectory("\\\\localhost\\share") && before == settings.file().loadFileAsString(), "Network path rejected before filesystem access", workspace.statusText());
        const auto denied = directory.getChildFile("denied");
        check(denied.isDirectory() && !workspace.applyProjectDirectory(denied.getFullPathName()) && before == settings.file().loadFileAsString(), "Unwritable directory rejected; saved choice retained", workspace.statusText());
        auto& docking = workspace.docking();
        docking.hidePanel("agent"); check(!docking.isPanelOpen("agent"), "Agent panel hides");
        docking.showPanel("agent"); check(docking.isPanelOpen("agent"), "Agent panel reopens");
        auto* log = docking.findPanelById("log"); docking.floatPanel(log);
        check(workspace.captureLayout()["floating"].getArray() != nullptr && workspace.captureLayout()["floating"].size() == 1, "Log panel floats in its own window");
        docking.dockPanel(log, CreationDock::DockTargetZone::Right);
        check(docking.zoneContainingPanel(log) == CreationDock::DockTargetZone::Right, "Log panel redocks");
        window.setBounds(110, 95, 1140, 760);
        savedWindow = window.getWindowStateAsString(); savedLayout = juce::JSON::toString(workspace.captureLayout(), true);
        check(window.saveState(error), "Window and panel layout saved", error);
        window.setVisible(false);
    }
    {
        ResearchSettings reloaded(config); ResearchWindow reopened(reloaded); reopened.setVisible(true);
        check(reloaded.projectDirectory() == chosen.getFullPathName(), "Project directory survives a fresh settings instance");
        check(reopened.getWindowStateAsString() == savedWindow, "Window position and size restored", reopened.getWindowStateAsString());
        check(juce::JSON::toString(reopened.workspace().captureLayout(), true) == savedLayout, "Dock layout restored after window destruction/recreation");
        check(saveNativeSnapshot(reopened, directory.getChildFile("workspace-restored.png")), "Restored layout rendering saved");
        reopened.setVisible(false);
    }
    {
        const auto malformedDir = directory.getChildFile("malformed-settings"); malformedDir.createDirectory();
        const auto file = malformedDir.getChildFile("settings.json"); file.replaceWithText("{broken settings");
        ResearchSettings malformed(malformedDir);
        check(malformed.warning().isNotEmpty() && !malformed.setProjectDirectory(chosen.getFullPathName(), error) && file.loadFileAsString() == "{broken settings", "Malformed settings reported and original retained");
    }
    check(proof.answerPassed, "Real FRust function returns integer 42", proof.log);
    check(proof.diagnosticPassed, "Malformed FRust emits readable file/line diagnostic", proof.diagnostic);
    check(proof.lifecyclePassed, "Three native load/invoke/unload cycles complete");
    auto* report = new juce::DynamicObject(); report->setProperty("passed", passed); report->setProperty("checks", checks);
    report->setProperty("frustRevision", RESEARCH_FRUST_REVISION); report->setProperty("frustOutput", proof.log);
    report->setProperty("expectedProjectDirectory", chosen.getFullPathName());
    report->setProperty("expectedWindowState", savedWindow); report->setProperty("expectedLayout", savedLayout);
    report->setProperty("networkPolicy", "No provider, registry or network operations are invoked; sources compile from bundled memory only.");
    directory.getChildFile("foundation-check.json").replaceWithText(juce::JSON::toString(juce::var(report), false));
    directory.getChildFile("frust-output.txt").replaceWithText(proof.log);
    return passed;
}

bool runRestorationCheck(ResearchWindow& window, const juce::File& directory, const TrustedFrustProof& proof)
{
    const auto expected = juce::JSON::parse(directory.getChildFile("foundation-check.json").loadFileAsString());
    ResearchSettings settings(directory.getChildFile("settings"));
    juce::Array<juce::var> checks; bool passed = true;
    auto check = [&](bool ok, const juce::String& name) {
        auto* result = new juce::DynamicObject(); result->setProperty("name", name); result->setProperty("passed", ok); checks.add(juce::var(result)); passed &= ok;
    };
    check(settings.projectDirectory() == expected["expectedProjectDirectory"].toString(), "Saved project directory survives a separate process restart");
    check(window.getWindowStateAsString() == expected["expectedWindowState"].toString(), "Window size/position survives a separate process restart");
    check(juce::JSON::toString(window.workspace().captureLayout(), true) == expected["expectedLayout"].toString(), "Dock layout survives a separate process restart");
    check(proof.passed(), "Asynchronous workspace self-test completes on the live application");
    check(window.workspace().logText().contains("Trusted FRust self-test PASSED"), "Actual result is published in the live Log panel");
    check(saveNativeSnapshot(window, directory.getChildFile("workspace-live-restart.png")), "Live restarted workspace rendering saved after proof completion");
    auto* report = new juce::DynamicObject(); report->setProperty("passed", passed); report->setProperty("checks", checks); report->setProperty("frustOutput", proof.log);
    directory.getChildFile("restart-check.json").replaceWithText(juce::JSON::toString(juce::var(report), false));
    return passed;
}
