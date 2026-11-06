param([string]$Root = (Join-Path $PSScriptRoot '../work/ci-deps'))
$ErrorActionPreference = 'Stop'
$Root = [IO.Path]::GetFullPath($Root)
$lock = Get-Content (Join-Path $PSScriptRoot '../ci/dependencies.lock.json') -Raw | ConvertFrom-Json
New-Item -ItemType Directory -Path $Root -Force | Out-Null
function Get-VerifiedArchive($url, $name, $sha) {
    if ($sha -notmatch '^[a-f0-9]{64}$') { throw "Invalid dependency hash: $name" }
    $path = Join-Path $Root $name
    if (!(Test-Path -LiteralPath $path)) {
        & curl.exe --fail --location --retry 2 --connect-timeout 30 --max-time 900 --output "$path.partial" $url
        if ($LASTEXITCODE -ne 0) { throw "Download failed: $name" }
        Move-Item -LiteralPath "$path.partial" -Destination $path
    }
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $sha) { throw "SHA256 mismatch: $name" }
    return $path
}
$llvmArchive = Get-VerifiedArchive $lock.llvm.url $lock.llvm.file $lock.llvm.sha256
$flexArchive = Get-VerifiedArchive $lock.winflexbison.url $lock.winflexbison.file $lock.winflexbison.sha256
$sourceArchive = Get-VerifiedArchive $lock.llvm.sourceUrl $lock.llvm.sourceFile $lock.llvm.sourceSha256
$llvm = Join-Path $Root 'llvm'
if (!(Test-Path "$llvm/lib/cmake/llvm/LLVMConfig.cmake")) {
    New-Item -ItemType Directory -Path $llvm -Force | Out-Null
    if (Get-Command python -ErrorAction SilentlyContinue) { & python "$PSScriptRoot/../ci/extract-llvm.py" $llvmArchive $llvm }
    else { & py -3 "$PSScriptRoot/../ci/extract-llvm.py" $llvmArchive $llvm }
    if ($LASTEXITCODE -ne 0) { throw 'LLVM extraction failed' }
}
if (!(Test-Path "$llvm/lib/LLVMCore.lib")) { throw 'LLVM development libraries missing' }
# The pinned FRust guard expects a vcpkg-shaped marker. Use the official
# package's real CMake directory explicitly; no upstream/installed file changes.
New-Item -ItemType Directory -Path "$llvm/share/llvm" -Force | Out-Null
'include("${CMAKE_CURRENT_LIST_DIR}/../../lib/cmake/llvm/LLVMConfig.cmake")' | Set-Content "$llvm/share/llvm/LLVMConfig.cmake"
$flex = Join-Path $Root 'winflexbison'
if (!(Test-Path "$flex/win_bison.exe")) { Expand-Archive -LiteralPath $flexArchive -DestinationPath $flex }
$values = [ordered]@{
    JUCE_PATH = (Join-Path $Root '../deps/JUCE')
    FRUSTLANG_ROOT = (Join-Path $Root '../deps/FrustLang')
    FRUST_VCPKG_INSTALLED_DIR = $llvm
    LLVM_DIR = "$llvm/lib/cmake/llvm"
    FLEX_EXECUTABLE = "$flex/win_flex.exe"
    BISON_EXECUTABLE = "$flex/win_bison.exe"
    FRUST_WINFLEXBISON_DIR = $flex
    RESEARCH_LLVM_SOURCE = $sourceArchive
}
foreach ($key in $values.Keys) {
    $value = [IO.Path]::GetFullPath($values[$key])
    [Environment]::SetEnvironmentVariable($key, $value, 'Process')
    if ($env:GITHUB_ENV) { "$key=$value" | Add-Content $env:GITHUB_ENV }
}
Write-Output 'Verified LLVM and WinFlexBison archives; configured isolated dependency paths.'
