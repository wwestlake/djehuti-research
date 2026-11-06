# Windows release procedure

The `Windows Release` workflow builds the Work Order 1 foundation in Release
configuration with one MSBuild core on `windows-2022`. It does not build LLVM,
modify external repositories, invoke vcpkg, or publish branch/PR builds.

## Inputs and local reproduction

`ci/dependencies.lock.json` pins FRust/CreationDock/Frate and JUCE commits,
LLVM 18.1.6 official MSVC development binaries and corresponding source, and
WinFlexBison 2.5.25. Downloaded archives are verified by SHA-256. Official Actions
are pinned to commit hashes. The runner's Visual Studio 2022, SDK, CMake and
PowerShell are serviced by GitHub; exact image/redist/CMake details are recorded
with the artifact. This is a repeatable pinned-dependency build, not a claim of
bit-identical output across serviced runner images.

On a Windows x64 machine with Visual Studio 2022 C++ tools, Git, CMake >=3.25,
PowerShell 7 and Windows SDK, extract `Research.tar` into the project root and
JUCE/FrustLang source archives into `work/deps/JUCE` and `work/deps/FrustLang`.
Alternatively check out the public repositories at the lock-file commits.
The distribution source archives contain the exact snapshots; initialize Git
repositories with the original commits if reproducing the revision guard, or
use the public Git checkouts. Do not override the guard to pass an unknown tree.

From a committed, clean Research checkout:

```powershell
./tools/Initialize-CiDependencies.ps1
cmake --preset windows-release "-DLLVM_DIR=$env:LLVM_DIR"
cmake --build --preset windows-release
pwsh -NoProfile -File tools/Test-Foundation.ps1 -Executable bin/Release/djehuti_research.exe
./tools/Build-Release.ps1 -Version v0.1.0-alpha.1
```

The initializer defaults to isolated `work/ci-deps`. It never touches an installed
LLVM. It adds a `share/llvm/LLVMConfig.cmake` forwarding marker only in that new
staging prefix for the pinned FRust installation guard; discovery explicitly uses
the official package's real `lib/cmake/llvm` directory. Native compiler libraries
remain unmodified. The build directory is `build/`, output `bin/Release/`.
Do not configure another build against an actively running build in that directory.

The pipeline measures configure, build, tests, packaging and extracted-archive
tests in `work/ci/timings.json` and the Actions summary. Both build and archive
executables must pass the existing 25 foundation + 6 restart checks. Each process
has a 90-second deadline and exits; the harness restores its test-only denied ACL
and closes/kills only its own timed-out processes. Snapshots are rendered by the
native JUCE components. No claim of manual mouse/keyboard coverage is made.

## Packaging and licensing

The portable Windows ZIP contains the Release EXE, eight unmodified app-local
Microsoft VC143 Release CRT DLLs from the VS redistributable directory, notices,
license texts, run instructions and content-hash provenance. It excludes compiler
tools, SDK files, Debug runtimes, credentials, work orders and user settings.
Windows system DLLs/UCRT come from supported Windows. `Test-Package.ps1` checks
the ZIP checksum, manifest hashes, file allowlist, credential patterns and then
runs the actual extracted executable through the foundation/restart harness.

The combined executable follows JUCE 7's **GPL version 3** option. Research-owned
files remain MIT. Corresponding source is supplied at no charge in the same
release's source ZIP: Research, JUCE, FRust and LLVM source snapshots plus build
scripts, dependency pins and notices. Microsoft runtime DLLs are separately
licensed redistributable system/compiler components. Keep source assets and
checksums available alongside every distributed binary; never publish a binary
alone or claim it is wholly MIT. See the upstream license files and
[GPLv3 section 6](https://www.gnu.org/licenses/gpl-3.0.html#section6),
[JUCE 7 terms](https://github.com/juce-framework/JUCE/blob/7.0.9/LICENSE.md), and
[Microsoft's redistributable list](https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution).

## Review, CI and publication

1. Push a focused branch and open a PR to `master`. Push/PR/manual runs produce
   CI candidate ZIPs and diagnostics as Actions artifacts; they never create a
   Release or notify the website catalog. Only public dependencies are cached;
   no tokens, checkout `.git` metadata, application settings or build objects
   enter the cache. Cache writes require a successful repository push, not PRs.
2. The owner reviews and merges the PR. Do not self-merge or push to `master`
   to bypass review. Read the successful workflow and inspect candidate contents.
3. After the version is approved, tag that reviewed commit, e.g.
   `git tag -a v0.1.0-alpha.1 <commit> -m "Experimental foundation release"`
   then `git push origin v0.1.0-alpha.1`. Tags accepted by this workflow are
   `v0.1.0-alpha.N`. Do not move/reuse a published tag; use the next number.
4. The tag run fails if its commit is not an ancestor of `origin/master`.
   It builds and verifies again. Only a successful tag build enables a separate
   publication job with `contents: write`; ordinary build jobs are read-only.
   GitHub Releases receives both versioned ZIPs and their checksums as a clearly
   marked experimental pre-release. Publication never rewrites sources or commits.
5. Download assets from the actual release, check SHA-256, inspect contents and
   launch the extracted Windows package on an available desktop test environment.
   Record commit, run, duration, release URL, hashes and compatibility evidence.

## Existing website integration and withdrawal

Use lagdaemon.com's existing admin Products facility. Create slug
`djehuti-research`, name `Djehuti Research (experimental)`, describe the limited
foundation, require no payment tier if supported, link `wwestlake/djehuti-research`
with tag prefix `v0.1.0-alpha.`. The GitHub Release remains a pre-release.
The admin linking response supplies a webhook secret: enter it privately in a
GitHub repository webhook for `release` events, JSON content and SSL verification,
target `https://lagdaemon.com/djehuti/api/webhooks/github/releases`. Never place
the secret in this repository, reports, terminal output or Actions logs.
Inspect webhook delivery status and verify the site's actual product, version,
pre-release marker and GitHub-hosted asset link. If the release predates linking,
use GitHub's normal release edit/redelivery facility to import it; do not invent
release metadata or insert production rows manually.

The inspected website restricts `/downloads` and its `/api/downloads/products`
data to admins/active beta testers. A no-payment tier does not make that area
anonymous/public. Registration requires an authenticated admin. Any missing
admin access or desired public-access behavior is a precise integration blocker;
it must not be bypassed by manufacturing tokens or editing the production DB.
Website code changes require its own branch/PR workflow and owner deployment
approval. Preserve unrelated working-tree changes there.

To withdraw a bad release, unpublish/delete it through GitHub's normal release
controls: the existing signed webhook handles `unpublished`/`deleted` by removing
that tag from the catalog. Verify both webhook delivery and website removal.
An admin can deactivate only the Research product to hide its listing if a
delivery fails. Keep required corresponding source available for existing binary
recipients. Do not modify other products or manually delete production files.

To exercise failure without affecting production, push a disposable branch with
a deliberately invalid dependency hash (or a failing test) and inspect the failed
build's diagnostics; the publication job must remain skipped. Delete that branch
after the test. Do not push a release tag or modify website production files for
this check. For a failed website deployment, use its existing PR compile checks
and workflow failure logs; do not force a production failure or merge an unapproved PR.
