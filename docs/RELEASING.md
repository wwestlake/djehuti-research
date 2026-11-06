# Windows release procedure

The `Windows Release` workflow builds the Work Order 1 foundation in Release
configuration with one MSBuild core on `windows-2022`. It does not build LLVM,
modify external repositories, invoke vcpkg, or publish branch/PR builds.

## Inputs and local reproduction

`ci/dependencies.lock.json` pins FRust/CreationDock/Frate and JUCE commits,
the existing S3 LLVM 18.1.6 vcpkg artifact and corresponding source/port patches, and
WinFlexBison 2.5.25. Downloaded archives are verified by SHA-256. Official Actions
are pinned to commit hashes. The runner's Visual Studio 2022, SDK, CMake and
PowerShell are serviced by GitHub; exact image/redist/CMake details are recorded
with the artifact. This is a repeatable pinned-dependency build, not a claim of
bit-identical output across serviced runner images.

On a Windows x64 machine with Visual Studio 2022 C++ tools, Git, CMake >=3.25,
PowerShell 7, Python 3, AWS CLI and Windows/DIA SDK, extract `Research.tar` into the project root and
JUCE/FrustLang source archives into `work/deps/JUCE` and `work/deps/FrustLang`.
Alternatively check out the public repositories at the lock-file commits.
The distribution source archives contain the exact snapshots. Public Git
checkouts reproduce the original revision guard directly. For an offline rebuild
from the supplied source ZIP, verify its SHA-256 first, extract the source TARs,
and commit the verified FRust snapshot into a new local Git repository:

```powershell
git -C work/deps/FrustLang init
git -C work/deps/FrustLang add .
git -C work/deps/FrustLang -c user.name=SourceRebuild -c user.email=rebuild@example.invalid commit -m 'Verified release source snapshot'
$snapshotRevision = git -C work/deps/FrustLang rev-parse HEAD
```

Copy the supplied LLVM source archive into `work/ci-deps` before initializing
dependencies. After initialization, configure with
`cmake --preset windows-release "-DLLVM_DIR=$env:LLVM_DIR" "-DRESEARCH_FRUST_REVISION=$snapshotRevision"`
and build with `cmake --build --preset windows-release`. The offline rebuild's
embedded revision identifies that new local snapshot commit. Official packages
always use the original lock-file commit. Never use this override for an unknown
or unverified source tree. Building LLVM itself from supplied source is optional;
the pinned official development archive remains the normal build prerequisite.

From a committed, clean Research checkout:

```powershell
./tools/Initialize-CiDependencies.ps1
cmake --preset windows-release "-DLLVM_DIR=$env:LLVM_DIR"
cmake --build --preset windows-release
pwsh -NoProfile -File tools/Test-Foundation.ps1 -Executable bin/Release/djehuti_research.exe
./tools/Build-Release.ps1 -Version v0.1.0-alpha.1
```

The initializer defaults to isolated `work/ci-deps`. It never touches an installed
LLVM. It streams the existing S3 tar directly into isolated staging, checking all
11,746,078,720 bytes against the lock-file SHA-256. No duplicate 11 GB TAR is kept
on the runner. Discovery uses its existing `share/llvm` configuration. Research
rebinds the imported PDB target's remembered `diaguids.lib` path to the runner's
installed x64 DIA SDK in memory; LLVM files remain unmodified. The build directory
is `build/`, output `bin/Release/`.
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
The additional `vcruntime140_threads.dll` is included when the selected VS runtime
supplies it. The packager records EXE/CRT startup imports using `dumpbin` and
rejects any unresolved dependency outside the packaged CRT or Microsoft Windows
system/API-set libraries. Windows system DLLs/UCRT come from supported Windows. `Test-Package.ps1` checks
the ZIP checksum, manifest hashes, file allowlist, credential patterns and then
runs the actual extracted executable through the foundation/restart harness.

The combined executable follows JUCE 7's **GPL version 3** option. Research-owned
files remain MIT. Corresponding source is supplied at no charge in the same
release's source ZIP: Research, JUCE, FRust and LLVM source snapshots plus build
scripts, dependency pins, vcpkg LLVM patches/build-input hashes and notices. Microsoft runtime DLLs are separately
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

## Credentials and existing website integration

The dependency step uses the existing approved AWS credentials through repository
secrets `AWS_ACCESS_KEY_ID` and `AWS_SECRET_ACCESS_KEY`, with region `us-east-2`.
Only that preparation step receives them. The compiler, package/test steps and
dependency cache receive no AWS credentials. The workflow performs only the S3
read of the pinned artifact. The reused credential's existing IAM policy is
S3FullAccess; this workflow does not claim that IAM identity is read-only or
change its policy. A dedicated identity limited to `s3:GetObject` on this object
is the preferred credential replacement when the account administrator provides
one. Never put key values, SSH private keys or server environment files in Git,
artifacts, cache contents, reports or command output. Fork PRs cannot receive
repository secrets; a cache miss there needs a trusted build to populate the
default-branch cache, rather than exposing credentials to untrusted code.

Website integration follows the existing applications' GitHub release webhook
process, using the local website repository's documented server administration
path. Browser product-registration steps are not part of this procedure. The
receiver is `https://lagdaemon.com/djehuti/api/webhooks/github/releases` and checks
the HMAC SHA-256 signature before importing matching repository/tag metadata.
One-time backend mapping and the matching GitHub `release` webhook secret must
be managed privately through that existing authorized administrative process.
Normal releases then require only an approved tag and a successful pipeline.
The complete verified server setup and troubleshooting runbook is an owner
deliverable; it includes credential locations without publishing secret values.

Verify actual GitHub delivery status, website release metadata and the real
GitHub-hosted ZIP link. The inspected website's Downloads area is restricted to
admins/beta testers; an empty payment tier does not remove that page-level gate.
Any website code change must follow its own repository review/deploy procedure.
Do not overwrite other agents' working trees or place files directly in production.

To withdraw a bad release, unpublish/delete it through GitHub's normal release
controls. The existing signed webhook handles `unpublished`/`deleted` by removing
that tag from the catalog. Verify delivery and website removal. Keep required
corresponding source available for existing binary recipients. Do not change
other products or delete production files manually.

To exercise failure without affecting production, push a disposable branch with
a deliberately invalid dependency hash (or a failing test) and inspect the failed
build's diagnostics; the publication job must remain skipped. Delete that branch
after the test. Do not push a release tag or modify website production files for
this check. For a failed website deployment, use its existing PR compile checks
and workflow failure logs; do not force a production failure or merge an unapproved PR.
