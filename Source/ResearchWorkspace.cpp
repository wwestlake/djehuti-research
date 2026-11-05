#include "ResearchWorkspace.h"

namespace
{
constexpr auto background = 0xff151c26;
constexpr auto foreground = 0xffdae3ed;
enum Command { newTab = 1, closeTab, choosePath, exitApp, showAgent, showDocuments, showLog, floatLog, resetLayout, proof, about };

class EmptyView final : public juce::Component
{
public:
    EmptyView(juce::String title, juce::String message)
    {
        heading.setText(title, juce::dontSendNotification);
        heading.setFont(juce::Font(23.0f, juce::Font::bold));
        description.setText(message, juce::dontSendNotification);
        description.setJustificationType(juce::Justification::topLeft);
        description.setFont(juce::Font(15.0f));
        description.setMinimumHorizontalScale(1.0f);
        addAndMakeVisible(heading); addAndMakeVisible(description);
        setName(title);
    }
    void resized() override
    {
        auto area = getLocalBounds().reduced(24);
        heading.setBounds(area.removeFromTop(40)); area.removeFromTop(12);
        description.setBounds(area.removeFromTop(220));
    }
private:
    juce::Label heading, description;
};
class LogView final : public juce::Component
{
public:
    LogView()
    {
        output.setMultiLine(true); output.setReadOnly(true); output.setScrollbarsShown(true);
        output.setFont(juce::Font("Consolas", 13.0f, juce::Font::plain));
        output.setName("Research Log"); output.setDescription("Application status and trusted FRust compiler results");
        run.setTooltip("Compile the bundled trusted function, call it, unload it, and check a malformed input. No network access.");
        addAndMakeVisible(output); addAndMakeVisible(run);
    }
    void resized() override
    {
        auto area = getLocalBounds().reduced(8);
        run.setBounds(area.removeFromTop(30).removeFromLeft(220)); area.removeFromTop(6);
        output.setBounds(area);
    }
    juce::TextEditor output;
    juce::TextButton run { "Run trusted FRust self-test" };
};
bool usableLayout(const juce::var& layout)
{
    if (!layout.isObject() || !layout["zones"].isObject()) return false;
    juce::StringArray seen;
    auto add = [&](const juce::String& id) {
        if ((id != "agent" && id != "documents" && id != "log") || seen.contains(id)) return false;
        seen.add(id); return true;
    };
    for (const auto* zone : {"left", "right", "center", "bottom"})
    {
        const auto* panels = layout["zones"][zone]["panels"].getArray();
        if (panels == nullptr) return false;
        for (const auto& id : *panels) if (!add(id.toString())) return false;
    }
    if (const auto* floating = layout["floating"].getArray())
        for (const auto& entry : *floating) if (!add(entry["id"].toString())) return false;
    for (const auto* size : {"leftWidth", "rightWidth", "bottomHeight"})
        if ((int) layout[size] < 60 || (int) layout[size] > 2400) return false;
    return true;
}
}

ResearchWorkspace::ResearchWorkspace(ResearchSettings& configuration) : settings(configuration)
{
    look.setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(background));
    look.setColour(juce::Label::textColourId, juce::Colour(foreground));
    look.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff101721));
    look.setColour(juce::TextEditor::textColourId, juce::Colour(foreground));
    look.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d465f));
    setLookAndFeel(&look);
    heading.setText("Djehuti Research", juce::dontSendNotification);
    heading.setFont(juce::Font(21.0f, juce::Font::bold));
    pathLabel.setText("Default project directory", juce::dontSendNotification);
    projectPath.setText(settings.projectDirectory(), false);
    projectPath.setName("Default project directory");
    projectPath.setTabKeyUsedAsCharacter(false);
    projectPath.setTooltip("An existing writable local directory. Apply validates and saves the selection; no project is created.");
    browse.setTooltip("Choose an existing local directory for future research projects.");
    apply.setTooltip("Validate and save this directory. An invalid choice leaves the last saved value unchanged.");
    apply.onClick = [this] { applyProjectDirectory(projectPath.getText()); };
    browse.onClick = [this] { chooseProjectDirectory(); };
    projectPath.onReturnKey = [this] { apply.triggerClick(); };
    pathStatus.setText(settings.projectDirectory().isEmpty() ? "No directory selected. Choose a location to save this preference." : "Saved directory loaded.", juce::dontSendNotification);
    for (juce::Component* child : std::initializer_list<juce::Component*> {&menu, &heading, &pathLabel, &projectPath, &browse, &apply, &pathStatus}) addAndMakeVisible(child);
    dock = std::make_unique<CreationDock::DockManager>(*this); addAndMakeVisible(*dock);
    dock->registerPanel("agent", "Agent", std::make_unique<EmptyView>("Agent", "No agent is connected.\n\nThis panel reserves space for future conversation and tool hosting. LLM calls, tools and credentials are not enabled in this foundation."), CreationDock::DockTargetZone::Left);
    auto tabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    documents = tabs.get(); documents->setName("Document tabs");
    dock->registerPanel("documents", "Documents", std::move(tabs), CreationDock::DockTargetZone::CenterTab);
    addDocumentTab("Welcome", "No research document is open.\n\nUse File > New empty document tab to try the tab container. These empty views do not create or edit files. Document types will be added in later orders.");
    auto logs = std::make_unique<LogView>(); log = &logs->output; proofButton = &logs->run;
    proofButton->onClick = [this] { runSelfTest(); };
    dock->registerPanel("log", "Log", std::move(logs), CreationDock::DockTargetZone::Bottom);
    restoreLayout(settings.snapshot()["layout"]);
    appendLog("Research foundation ready. Local settings and trusted FRust proof only.\nNo agent, backend or network service is started.\n");
    if (settings.warning().isNotEmpty()) { pathStatus.setText(settings.warning(), juce::dontSendNotification); appendLog(settings.warning() + "\n"); }
    setSize(1240, 820);
}
ResearchWorkspace::~ResearchWorkspace()
{
    if (proofWorker.joinable()) proofWorker.join();
    chooser.reset(); dock.reset(); setLookAndFeel(nullptr);
}
void ResearchWorkspace::paint(juce::Graphics& g) { g.fillAll(juce::Colour(background)); }
void ResearchWorkspace::resized()
{
    auto area = getLocalBounds(); menu.setBounds(area.removeFromTop(25));
    auto header = area.removeFromTop(106).reduced(12, 4);
    heading.setBounds(header.removeFromTop(30));
    auto row = header.removeFromTop(30); pathLabel.setBounds(row.removeFromLeft(175));
    apply.setBounds(row.removeFromRight(70).reduced(2, 0)); browse.setBounds(row.removeFromRight(95).reduced(3, 0)); projectPath.setBounds(row.reduced(3, 0));
    pathStatus.setBounds(header.removeFromTop(28));
    if (dock) dock->setBounds(area.reduced(8, 0));
}
void ResearchWorkspace::addDocumentTab(const juce::String& title, const juce::String& description)
{
    documents->addTab(title, juce::Colour(0xff203044), new EmptyView(title, description), true);
    documents->setCurrentTabIndex(documents->getNumTabs() - 1);
}
void ResearchWorkspace::addEmptyDocumentTab()
{
    addDocumentTab("Empty " + juce::String(nextTab++), "An empty document view.\n\nNo file is loaded and no content is stored. This tab demonstrates the container for future document types.");
    dock->activatePanel("documents");
}
void ResearchWorkspace::closeCurrentDocumentTab()
{
    if (documents->getNumTabs() > 1) documents->removeTab(documents->getCurrentTabIndex());
}
bool ResearchWorkspace::applyProjectDirectory(const juce::String& path)
{
    juce::String error;
    const bool ok = settings.setProjectDirectory(path, error);
    if (ok) projectPath.setText(settings.projectDirectory(), false);
    pathStatus.setColour(juce::Label::textColourId, juce::Colour(ok ? 0xff9bd5b1 : 0xffffb49a));
    pathStatus.setText(ok ? "Saved: " + settings.projectDirectory() : error, juce::dontSendNotification);
    pathStatus.setTooltip(pathStatus.getText());
    appendLog((ok ? "PASS directory: " + settings.projectDirectory() : "Directory rejected: " + error) + "\n");
    return ok;
}
void ResearchWorkspace::chooseProjectDirectory()
{
    const auto initial = settings.projectDirectory();
    chooser = std::make_unique<juce::FileChooser>("Choose default research-project directory",
        juce::File::isAbsolutePath(initial) ? juce::File(initial) : juce::File(), "", true);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [safe = juce::Component::SafePointer<ResearchWorkspace>(this)](const juce::FileChooser& selected) {
            if (safe != nullptr && selected.getResult() != juce::File()) safe->applyProjectDirectory(selected.getResult().getFullPathName());
        });
}
void ResearchWorkspace::appendLog(const juce::String& text)
{
    log->moveCaretToEnd(); log->insertTextAtCaret(text);
}
void ResearchWorkspace::runSelfTest()
{
    if (proofRunning) return;
    if (proofWorker.joinable()) proofWorker.join();
    proofRunning = true; proofButton->setEnabled(false); dock->showPanel("log");
    appendLog("Running bundled trusted FRust proof...\n");
    proofWorker = std::thread([safe = juce::Component::SafePointer<ResearchWorkspace>(this)] {
        const auto result = runTrustedFrustProof();
        juce::MessageManager::callAsync([safe, result] {
            if (safe == nullptr) return;
            safe->appendLog(result.log); safe->proofRunning = false; safe->proofButton->setEnabled(true);
            if (safe->onProofFinished) safe->onProofFinished(result);
        });
    });
}
juce::var ResearchWorkspace::captureLayout() const { return dock->captureLayout(); }
void ResearchWorkspace::restoreLayout(const juce::var& layout)
{
    if (!usableLayout(layout)) { dock->resetLayout(); return; }
    for (const auto& id : dock->getRegisteredPanelIds()) dock->hidePanel(id);
    dock->applyLayout(layout);
}
juce::StringArray ResearchWorkspace::getMenuBarNames() { return {"File", "View", "Help"}; }
juce::PopupMenu ResearchWorkspace::getMenuForIndex(int index, const juce::String&)
{
    juce::PopupMenu out;
    if (index == 0) { out.addItem(newTab, "New empty document tab"); out.addItem(closeTab, "Close current document tab", documentTabCount() > 1); out.addSeparator(); out.addItem(choosePath, "Choose project directory..."); out.addSeparator(); out.addItem(exitApp, "Exit"); }
    if (index == 1)
    {
        out.addItem(showAgent, "Agent", true, dock->isPanelOpen("agent"));
        out.addItem(showDocuments, "Documents", true, dock->isPanelOpen("documents"));
        out.addItem(showLog, "Log", true, dock->isPanelOpen("log"));
        out.addItem(floatLog, "Float Log panel"); out.addSeparator(); out.addItem(resetLayout, "Restore default layout");
    }
    if (index == 2) { out.addItem(proof, "Run trusted FRust self-test", !proofRunning); out.addItem(about, "About this foundation"); }
    return out;
}
void ResearchWorkspace::menuItemSelected(int command, int)
{
    switch (command)
    {
        case newTab: addEmptyDocumentTab(); break;
        case closeTab: closeCurrentDocumentTab(); break;
        case choosePath: chooseProjectDirectory(); break;
        case exitApp: if (requestExit) requestExit(); break;
        case showAgent: dock->togglePanel("agent"); break;
        case showDocuments: dock->togglePanel("documents"); break;
        case showLog: dock->togglePanel("log"); break;
        case floatLog: dock->showPanel("log"); dock->floatPanel(dock->findPanelById("log")); break;
        case resetLayout: dock->resetLayout(); break;
        case proof: runSelfTest(); break;
        case about: appendLog("Djehuti Research 0.1.0 | Work Order 1 foundation\nAgent, research storage and document editing are not implemented.\n"); dock->showPanel("log"); break;
        default: break;
    }
}

ResearchWindow::ResearchWindow(ResearchSettings& configuration)
    : DocumentWindow("Djehuti Research", juce::Colour(background), allButtons), settings(configuration)
{
    setUsingNativeTitleBar(true); setResizable(true, true); setResizeLimits(940, 620, 3840, 2160);
    content = new ResearchWorkspace(settings); setContentOwned(content, true);
    content->requestExit = [this] { closeButtonPressed(); };
    centreWithSize(1240, 820);
    const auto state = settings.snapshot()["windowState"].toString();
    if (state.isNotEmpty()) restoreWindowStateFromString(state);
    bool onScreen = false;
    for (const auto& display : juce::Desktop::getInstance().getDisplays().displays)
        if (display.userArea.getIntersection(getBounds()).getWidth() >= 100 && display.userArea.getIntersection(getBounds()).getHeight() >= 100) onScreen = true;
    if (!onScreen) centreWithSize(1240, 820);
}
bool ResearchWindow::saveState(juce::String& error) { return settings.saveWorkspace(getWindowStateAsString(), content->captureLayout(), error); }
void ResearchWindow::closeButtonPressed()
{
    juce::String error;
    if (!saveState(error)) content->appendLog("Settings save failed: " + error + "\n");
    if (onClose) onClose();
}
