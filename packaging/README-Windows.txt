Djehuti Research -- experimental Windows x64 foundation

Extract the entire ZIP to a writable local folder and run djehuti_research.exe.
Windows 10/11 x64 is the intended desktop platform. CI verifies Windows Server
2022 x64; the original Debug foundation was tested on Windows 10 build 19045.
The release is unsigned. There is no MSI, automatic updater, or install service.
The app-local Microsoft C++ runtime files are included; Windows system libraries
and the Universal CRT are supplied by a supported Windows installation.

This is an early shell with Agent placeholder, Document tabs, dockable Log,
settings, and a bundled trusted FRust proof. It has no research database,
ingestion/retrieval, document editor, LLM integration, or general code sandbox.
The bundled compiler proof processes trusted built-in input only.

Settings are stored per user in %APPDATA%\DjehutiResearch. To remove the app,
close it and delete its extracted folder. Remove that settings folder separately
only if you also want to discard preferences.

The combined executable is distributed under GNU GPL version 3, without warranty.
Research-owned source remains MIT. Read licenses and THIRD_PARTY_NOTICES.md.
Matching Research, JUCE, FRust and LLVM source archives accompany the binary
release or CI candidate, at no charge. See SOURCE.txt for their actual location
and build-provenance.json for the dependency identity.
