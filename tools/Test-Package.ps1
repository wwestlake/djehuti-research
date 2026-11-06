param([Parameter(Mandatory)][string]$Version)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Set-Location $repo
$name = "DjehutiResearch-$Version-windows-x64"
$out = Join-Path $repo 'work/dist'
$zip = Join-Path $out "$name.zip"
$expected = ((Get-Content "$zip.sha256" -Raw).Trim() -split '\s+')[0]
if ((Get-FileHash $zip -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) { throw 'Package checksum mismatch' }
$extract = Join-Path $repo "work/package-test/$Version"
if (Test-Path -LiteralPath $extract) { throw 'Archive verification requires a fresh extraction path' }
Expand-Archive -LiteralPath $zip -DestinationPath $extract
$root = Join-Path $extract $name
$manifest = Get-Content "$root/build-provenance.json" -Raw | ConvertFrom-Json
if ($manifest.version -ne $Version -or $manifest.configuration -ne 'Release') { throw 'Wrong packaged version/configuration' }
foreach ($file in $manifest.files) {
    if ((Get-FileHash -LiteralPath (Join-Path $root $file.path) -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.sha256) { throw "Content checksum mismatch: $($file.path)" }
}
$allowed = @('djehuti_research.exe','README.txt','THIRD_PARTY_NOTICES.md','SOURCE.txt','build-provenance.json',
    'msvcp140.dll','msvcp140_1.dll','msvcp140_2.dll','msvcp140_atomic_wait.dll','msvcp140_codecvt_ids.dll','vcruntime140.dll','vcruntime140_1.dll','vcruntime140_threads.dll','concrt140.dll')
foreach ($file in Get-ChildItem -LiteralPath $root -File -Recurse) {
    $relative=[IO.Path]::GetRelativePath($root,$file.FullName)
    if ($relative -notlike 'licenses\*' -and $relative -notin $allowed) { throw "Unexpected package file: $relative" }
    if ($file.Extension -in @('.txt','.md','.json')) {
        if ((Get-Content -LiteralPath $file.FullName -Raw) -match '(?i)(-----BEGIN (?:RSA |OPENSSH )?PRIVATE KEY|gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{30,}|AIza[A-Za-z0-9_-]{30,})') { throw 'Potential credential in package text' }
    }
}
& pwsh -NoProfile -File tools/Test-Foundation.ps1 -Executable "$root/djehuti_research.exe"
if ($LASTEXITCODE -ne 0) { throw 'Extracted archive foundation checks failed' }
$fixture = Get-ChildItem work/acceptance -Directory | Sort-Object Name -Descending | Select-Object -First 1
$foundation=Get-Content "$($fixture.FullName)/foundation-check.json" -Raw | ConvertFrom-Json
$restart=Get-Content "$($fixture.FullName)/restart-check.json" -Raw | ConvertFrom-Json
$process=Get-Content "$($fixture.FullName)/process-observation.json" -Raw | ConvertFrom-Json
if (!$foundation.passed -or $foundation.checks.Count -ne 25 -or !$restart.passed -or $restart.checks.Count -ne 6 -or !$process.processExited -or !$process.restartProcessExited) { throw 'Incomplete archive verification reports' }
[ordered]@{version=$Version;sha256=$expected;foundationChecks=25;restartChecks=6;processesExited=$true;fixture=$fixture.Name} | ConvertTo-Json | Set-Content work/ci/package-verification.json
Write-Output 'Extracted package: 25 foundation + 6 restart checks passed; both processes exited.'
