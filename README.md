# Djehuti Research

Work Order 1: an independent native Windows JUCE/C++ application foundation.

The workspace has an explicitly inactive Agent panel, a multi-tab Document area and a dockable/floating Log. File > New empty document tab demonstrates the document container without creating files. View restores hidden panels or the default layout. The Log button and Help menu run the bundled trusted FRust proof.

Choose an existing writable local default project directory, then Apply. Invalid, inaccessible and network locations are rejected visibly; the last saved selection is retained. The check creates only a unique temporary write probe, automatically removed when its handle closes. No project or directory is created by choosing a location.

Window geometry, panel layout and the project-directory preference are saved to `%APPDATA%/DjehutiResearch/settings.json` using temporary-file replacement. A malformed or unsupported settings file is reported and preserved, rather than silently overwritten. `--settings-dir <absolute-directory>` isolates settings for testing.

## Build

Use Visual Studio 2022 x64, CMake, existing JUCE, standalone FrustLang, WinFlexBison and the existing LLVM 18.1.6 installation. `CMakePresets.json` records this machine's dependency locations; override these cache paths in `CMakeUserPresets.json` on another machine. Runtime code contains no developer-machine source paths.

```powershell
New-Item -ItemType Directory -Path work -Force | Out-Null
cmake --preset windows-debug --graphviz=work/dependencies.dot *> work/configure.log
cmake --build --preset windows-debug *> work/build-debug.log
```

Executable: `bin/Debug/djehuti_research.exe`. One target, Debug, one build core. Do not run vcpkg or rebuild LLVM. Configuration rejects a FRust revision mismatch or tracked edits in the referenced compiler/dock/host directories. FRust headers and libraries are compiled together from revision `008afac7e5bea9569e0631795871334b2336a022`; generated files, libraries and PDBs are redirected into Research's own `build/` tree. No prebuilt FRust archive of unknown provenance is consumed. The LLVM package is imported read-only.

## Focused verification

```powershell
pwsh -NoProfile -File tools/Test-Foundation.ps1
```

The executable's `--foundation-check <absolute-fixture-directory>` mode exercises the same window, settings, tab/docking and FRust functions as the UI. The wrapper creates an isolated directory with a denied-write ACL for the negative test, restores its original ACL in `finally`, records process exit and samples only that process's TCP/UDP endpoints. A second process uses `--verify-restored` to reload those exact settings, show the real window, run the asynchronous workspace self-test, verify its Log result and exit normally. Native JUCE component snapshots record the actual rendered workspace; they do not capture the OS window frame. Reports and logs are under `work/acceptance/`. No elevation, firewall change or packet capture is used. Endpoint sampling is observational: it is supplemented by source/dependency review, and cannot establish the absence of a very short-lived connection between samples.

Also perform a real GUI check: choose a directory, create/switch tabs, float/redock the Log, run the self-test, resize/reposition, exit and relaunch with the same settings directory. Close all test windows/processes explicitly afterward. Compile success alone does not satisfy GUI acceptance.

## Scope and dependencies

Adapted the JUCE lifecycle, CreationDock workspace and settings patterns from DjehutiMath foundation `ff2ee952abe2e697db71d9342618fb6cc359f870`. Reuses CreationDock and the actual FRust compiler/plugin-host implementation. Research's settings, directory validation, empty tab views, bundled proof and focused checks are new application code.

Direct application libraries: ResearchProofSources, CreationDock, frust_plugin_host, frust_runtime and JUCE GUI. FRust adds frust_lang, frate_lib and imported LLVM component libraries. No Suite VFS, SuiteFrust, Services, AssetSystem, AI provider or website target is linked. The generated dependency graph and `build/frust-provenance.txt` record the configured build.

The proof compiles a bundled `answer() -> i64`, loads its in-memory x64 object with the existing `frust_plugin_load_object` C ABI, calls it and destroys the host handle before the pointer leaves its scope. It repeats three times, then compiles malformed bundled input and reports the real diagnostic. No user code, imports, provider calls or registry resolution are offered.

Database, semantic retrieval/ISD, document editing, untrusted plugin permissions, credentials and website/LLM integration await separate orders. The Orders folder on Drive is authoritative; no later order is started automatically.
