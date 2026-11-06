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
$flexArchive = Get-VerifiedArchive $lock.winflexbison.url $lock.winflexbison.file $lock.winflexbison.sha256
$sourceArchive = Get-VerifiedArchive $lock.llvm.sourceUrl $lock.llvm.sourceFile $lock.llvm.sourceSha256
$vcpkgSource = Get-VerifiedArchive $lock.vcpkg.sourceUrl $lock.vcpkg.sourceFile $lock.vcpkg.sourceSha256
$llvm = Join-Path $Root 'llvm'
if (!(Test-Path "$llvm/s3-provenance.json")) {
    if (Test-Path -LiteralPath $llvm) { throw 'Incomplete/other LLVM staging directory; use a fresh isolated dependency root' }
    if (!$env:AWS_ACCESS_KEY_ID -or !$env:AWS_SECRET_ACCESS_KEY) { throw 'S3 dependency cache miss: run a trusted branch build with the approved AWS repository secrets first' }
    if (!(Get-Command aws -ErrorAction SilentlyContinue)) { throw 'AWS CLI is required to read the existing S3 LLVM artifact' }
    if (Get-Command python -ErrorAction SilentlyContinue) { & python "$PSScriptRoot/../ci/extract-llvm.py" "$PSScriptRoot/../ci/dependencies.lock.json" $llvm }
    else { & py -3 "$PSScriptRoot/../ci/extract-llvm.py" "$PSScriptRoot/../ci/dependencies.lock.json" $llvm }
    if ($LASTEXITCODE -ne 0) { throw 'S3 LLVM extraction/verification failed' }
}
$provenance = Get-Content "$llvm/s3-provenance.json" -Raw | ConvertFrom-Json
if ($provenance.sha256 -ne $lock.llvm.sha256 -or !(Test-Path "$llvm/lib/LLVMCore.lib") -or !(Test-Path "$llvm/share/llvm/LLVMConfig.cmake")) { throw 'Cached LLVM identity/development libraries mismatch' }
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dia = Join-Path $vs 'DIA SDK/lib/amd64/diaguids.lib'
if (!(Test-Path -LiteralPath $dia)) { throw 'Runner Visual Studio 2022 x64 DIA SDK is missing' }
$flex = Join-Path $Root 'winflexbison'
if (!(Test-Path "$flex/win_bison.exe")) { Expand-Archive -LiteralPath $flexArchive -DestinationPath $flex }
$values = [ordered]@{
    JUCE_PATH = (Join-Path $Root '../deps/JUCE')
    FRUSTLANG_ROOT = (Join-Path $Root '../deps/FrustLang')
    FRUST_VCPKG_INSTALLED_DIR = $llvm
    LLVM_DIR = "$llvm/share/llvm"
    RESEARCH_DIA_GUIDS_LIBRARY = $dia
    FLEX_EXECUTABLE = "$flex/win_flex.exe"
    BISON_EXECUTABLE = "$flex/win_bison.exe"
    FRUST_WINFLEXBISON_DIR = $flex
    RESEARCH_LLVM_SOURCE = $sourceArchive
    RESEARCH_VCPKG_SOURCE = $vcpkgSource
}
foreach ($key in $values.Keys) {
    $value = [IO.Path]::GetFullPath($values[$key])
    [Environment]::SetEnvironmentVariable($key, $value, 'Process')
    if ($env:GITHUB_ENV) { "$key=$value" | Add-Content $env:GITHUB_ENV }
}
Write-Output 'Verified existing S3 LLVM and source/tool archives; configured isolated dependency paths.'
