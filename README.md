# Djehuti Research

An early native Windows JUCE/C++ research application foundation. It currently provides an inactive Agent placeholder, a tabbed Document area, a dockable Log, persistent window/layout settings, a validated default project-directory preference, and a bundled trusted FRust compiler/runtime proof.

Research functionality is under development. There is no research database, ingestion, semantic retrieval/ISD, document editing, LLM integration or website connector. The application is independent of the Djehuti Suite VFS and does not require its runtime, Services or AssetSystem targets.

## Build prerequisites

The foundation was built and functionally checked on Windows 10 x64, build 19045, using Visual Studio 2022 (MSVC 19.36), CMake 4.0.1 and PowerShell 7. Other operating systems have not been tested. The project requires Windows/MSVC; the supplied schema-6 presets require CMake 3.25 or later.

Install or provide these external dependencies separately; none is vendored here:

| Dependency | Verified version / source |
|---|---|
| JUCE | [7.0.9](https://github.com/juce-framework/JUCE/tree/7.0.9) source checkout |
| FRust, CreationDock and Frate | [FrustLang](https://github.com/wwestlake/FrustLang), exact commit `008afac7e5bea9569e0631795871334b2336a022` |
| LLVM | Existing LLVM 18.1.6 installation, built with the native x64 libraries consumed by FRust; includes `share/llvm/LLVMConfig.cmake` |
| WinFlexBison | Verified Flex 2.6.4 and Bison 3.8.2 executables, with `FlexLexer.h` |
| Windows development tools | Visual Studio C++ workload, Windows SDK and DIA SDK |

FRust's referenced source directories must have no tracked edits. Configuration verifies the exact FRust revision. It compiles matching headers/libraries into this project's build tree. It does not use old FRust archives, install dependencies, invoke vcpkg or rebuild LLVM. Reuse the existing verified LLVM installation rather than reconciling it through a package manager.

## Local configuration and reproduction

Set these environment variables to your existing dependency locations before configuring, or override the equivalent cache variables in an ignored `CMakeUserPresets.json`:

| Variable | Local value required |
|---|---|
| `JUCE_PATH` | JUCE source root containing `CMakeLists.txt` |
| `FRUSTLANG_ROOT` | Exact FrustLang checkout listed above |
| `FRUST_VCPKG_INSTALLED_DIR` | Existing LLVM installation root containing `share/llvm/LLVMConfig.cmake`; its name does not authorize running vcpkg |
| `FLEX_EXECUTABLE` | Full path to `win_flex.exe` |
| `BISON_EXECUTABLE` | Full path to `win_bison.exe` |
| `FRUST_WINFLEXBISON_DIR` | Directory containing `FlexLexer.h` |

From the repository root:

```powershell
New-Item -ItemType Directory -Path work -Force | Out-Null
cmake --preset windows-debug --graphviz=work/dependencies.dot *> work/configure.log
if ($LASTEXITCODE -ne 0) { throw 'Configuration failed; inspect work/configure.log' }
cmake --build --preset windows-debug *> work/build-debug.log
if ($LASTEXITCODE -ne 0) { throw 'Build failed; inspect work/build-debug.log' }
pwsh -NoProfile -File tools/Test-Foundation.ps1 *> work/test-foundation.log
if ($LASTEXITCODE -ne 0) { throw 'Focused checks failed; inspect work/test-foundation.log' }
```

The Debug executable is `bin/Debug/djehuti_research.exe`. Use one build core and the existing `build/` directory. The supplied build preset selects only the application target. A local preset may inherit `windows-debug` and supply machine-specific cache values; keep it out of Git.

The Work Order 1 baseline passed 25 native foundation checks and 6 separate-process restoration checks. These cover tabs, docking, valid/invalid/unwritable directories, settings preservation, a real FRust `42` result, readable malformed-input diagnostics, handle unloading and normal process exit. Test reports and native workspace renderings are written beneath `work/acceptance/`. Network endpoint sampling supplements source review but cannot exclude a connection shorter than the sampling interval. Manual desktop input was not verified because the UI helper could not capture/input reliably; native component rendering and behavior were checked independently.

## Application settings

Choose an existing writable local default project directory and Apply. Invalid choices leave the saved preference intact. Validation creates a unique temporary write probe and removes it when its handle closes; it does not create a research project. Keep personal research files outside the source checkout.

Window geometry, panel layout and the preference are stored in the current user's application-data directory under `DjehutiResearch/settings.json`. Malformed settings are reported and preserved. `--settings-dir <absolute-directory>` isolates state for testing.

## Licensing and contributions

Project-owned material is licensed under [MIT](LICENSE). Preserve the inherited attribution and permissions in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). This does not relicense JUCE, LLVM or other external components. In particular, the JUCE GUI modules have GPLv3/commercial terms; a combined application is not automatically an MIT-only binary.

Use focused branches and PRs to `master` after the initial repository publication; see [CONTRIBUTING.md](CONTRIBUTING.md). Release automation and website downloads are outside this foundation's current scope.
