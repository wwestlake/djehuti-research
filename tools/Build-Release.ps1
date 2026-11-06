param([string]$Version = 'v0.1.0-alpha.1')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
New-Item -ItemType Directory -Path work/ci -Force | Out-Null
$timings = [ordered]@{ version=$Version; commit=(& git rev-parse HEAD); runner=$env:ImageOS; runnerVersion=$env:ImageVersion }
function Invoke-Timed($name, [scriptblock]$command) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    try { & $command; if ($LASTEXITCODE -ne 0) { throw "$name failed; inspect work/ci/$name.log" } }
    finally { $watch.Stop(); $timings[$name + 'Seconds'] = [Math]::Round($watch.Elapsed.TotalSeconds, 2); $timings | ConvertTo-Json | Set-Content work/ci/timings.json }
}
Invoke-Timed configure { & cmake --preset windows-release "-DLLVM_DIR=$env:LLVM_DIR" *> work/ci/configure.log }
Invoke-Timed build { & cmake --build --preset windows-release *> work/ci/build.log }
Invoke-Timed test { & pwsh -NoProfile -File tools/Test-Foundation.ps1 -Executable bin/Release/djehuti_research.exe *> work/ci/test.log }
Invoke-Timed package { & pwsh -NoProfile -File tools/Package-Windows.ps1 -Version $Version *> work/ci/package.log }
Invoke-Timed archiveTest { & pwsh -NoProfile -File tools/Test-Package.ps1 -Version $Version *> work/ci/archive-test.log }
$timings | ConvertTo-Json | Write-Output
if ($env:GITHUB_STEP_SUMMARY) {
    "### Verified Windows x64 package`n``````json`n$($timings | ConvertTo-Json)`n``````" | Add-Content $env:GITHUB_STEP_SUMMARY
}
