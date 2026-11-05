#include "Settings.h"
#include <windows.h>

namespace
{
juce::var defaults()
{
    auto* value = new juce::DynamicObject();
    value->setProperty("schemaVersion", 1);
    value->setProperty("projectDirectory", "");
    return juce::var(value);
}
}
ResearchSettings::ResearchSettings(juce::File directory) : settingsFile(directory.getChildFile("settings.json")), data(defaults())
{
    if (!settingsFile.existsAsFile()) return;
    auto parsed = juce::JSON::parse(settingsFile.loadFileAsString());
    if (!parsed.isObject() || (int) parsed["schemaVersion"] != 1 || !parsed["projectDirectory"].isString())
    {
        readable = false;
        loadWarning = "Settings are invalid or from an unsupported version. The original file has been preserved: " + settingsFile.getFullPathName();
        return;
    }
    data = parsed;
    juce::String reason;
    if (projectDirectory().isNotEmpty() && !validateProjectDirectory(projectDirectory(), reason, false))
        loadWarning = "Saved project directory is unavailable: " + reason;
}
juce::File ResearchSettings::defaultDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("DjehutiResearch");
}
bool ResearchSettings::validateProjectDirectory(const juce::String& input, juce::String& error, bool probeWrite)
{
    error.clear();
    const auto path = input.trim();
    if (path.isEmpty() || !juce::File::isAbsolutePath(path)) { error = "Choose an existing absolute local directory."; return false; }
    if (path.startsWith("\\\\") || path.startsWith("//")) { error = "Choose a local directory; network locations are not enabled."; return false; }
    const juce::File directory(path);
    const auto driveRoot = path.substring(0, 3).replaceCharacter('/', '\\');
    if (GetDriveTypeW(driveRoot.toWideCharPointer()) == DRIVE_REMOTE) { error = "Network drives are not enabled."; return false; }
    if (!directory.isDirectory()) { error = "The selected directory does not exist or cannot be accessed."; return false; }
    if (!probeWrite) return true;
    const auto probe = directory.getChildFile(".djehuti-write-check-" + juce::Uuid().toString() + ".tmp");
    const auto handle = CreateFileW(probe.getFullPathName().toWideCharPointer(), GENERIC_WRITE, 0, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (handle == INVALID_HANDLE_VALUE) { error = "The selected directory is not writable (Windows error " + juce::String((int) GetLastError()) + ")."; return false; }
    DWORD written = 0;
    const char marker = 0;
    const bool ok = WriteFile(handle, &marker, 1, &written, nullptr) && written == 1 && FlushFileBuffers(handle);
    CloseHandle(handle); // FILE_FLAG_DELETE_ON_CLOSE removes only this unique probe.
    if (!ok) error = "The directory could not complete a write check.";
    return ok;
}
bool ResearchSettings::commit(juce::var candidate, juce::String& error)
{
    if (!readable) { error = loadWarning; return false; }
    const auto created = settingsFile.getParentDirectory().createDirectory();
    if (created.failed()) { error = "Could not create the settings directory: " + created.getErrorMessage(); return false; }
    juce::TemporaryFile temporary(settingsFile);
    if (!temporary.getFile().replaceWithText(juce::JSON::toString(candidate, false)) || !temporary.overwriteTargetFileWithTemporary())
    { error = "Could not save settings. The last saved settings were retained."; return false; }
    data = candidate;
    error.clear();
    return true;
}
bool ResearchSettings::setProjectDirectory(const juce::String& path, juce::String& error)
{
    if (!validateProjectDirectory(path, error)) return false;
    auto candidate = data.clone();
    candidate.getDynamicObject()->setProperty("projectDirectory", juce::File(path.trim()).getFullPathName());
    return commit(candidate, error);
}
bool ResearchSettings::saveWorkspace(const juce::String& windowState, const juce::var& layout, juce::String& error)
{
    auto candidate = data.clone();
    candidate.getDynamicObject()->setProperty("windowState", windowState);
    candidate.getDynamicObject()->setProperty("layout", layout.clone());
    return commit(candidate, error);
}
