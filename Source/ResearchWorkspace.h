#pragma once
#include "Settings.h"
#include "TrustedFrust.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <CreationDock/DockManager.h>
#include <thread>

class ResearchWorkspace final : public juce::Component, public juce::MenuBarModel
{
public:
    explicit ResearchWorkspace(ResearchSettings&);
    ~ResearchWorkspace() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int, const juce::String&) override;
    void menuItemSelected(int, int) override;
    void addEmptyDocumentTab();
    void closeCurrentDocumentTab();
    bool applyProjectDirectory(const juce::String&);
    void runSelfTest();
    void appendLog(const juce::String&);
    juce::var captureLayout() const;
    void restoreLayout(const juce::var&);
    int documentTabCount() const { return documents->getNumTabs(); }
    juce::String statusText() const { return pathStatus.getText(); }
    juce::String logText() const { return log->getText(); }
    CreationDock::DockManager& docking() { return *dock; }
    std::function<void()> requestExit;
    std::function<void(const TrustedFrustProof&)> onProofFinished;
private:
    void chooseProjectDirectory();
    void addDocumentTab(const juce::String& title, const juce::String& description);
    ResearchSettings& settings;
    juce::LookAndFeel_V4 look;
    juce::MenuBarComponent menu { this };
    juce::Label heading, pathLabel, pathStatus;
    juce::TextEditor projectPath;
    juce::TextButton browse { "Choose..." }, apply { "Apply" };
    std::unique_ptr<CreationDock::DockManager> dock;
    juce::TabbedComponent* documents = nullptr;
    juce::TextEditor* log = nullptr;
    juce::TextButton* proofButton = nullptr;
    std::unique_ptr<juce::FileChooser> chooser;
    std::thread proofWorker;
    bool proofRunning = false;
    int nextTab = 1;
    juce::TooltipWindow tooltips { this, 600 };
};

class ResearchWindow final : public juce::DocumentWindow
{
public:
    explicit ResearchWindow(ResearchSettings&);
    ResearchWorkspace& workspace() { return *content; }
    bool saveState(juce::String& error);
    void closeButtonPressed() override;
    std::function<void()> onClose;
private:
    ResearchSettings& settings;
    ResearchWorkspace* content = nullptr;
};
