param([Parameter(Mandatory)][ValidatePattern('^v[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+(?:-ci\.[0-9]+)?$')][string]$Version)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Set-Location $repo
$lock = Get-Content ci/dependencies.lock.json -Raw | ConvertFrom-Json
foreach ($item in @(@($env:JUCE_PATH,$lock.juce.revision), @($env:FRUSTLANG_ROOT,$lock.frust.revision))) {
    if ((& git -C $item[0] rev-parse HEAD) -ne $item[1]) { throw 'Package source revision mismatch' }
    & git -C $item[0] diff --quiet HEAD
    if ($LASTEXITCODE -ne 0) { throw 'Package dependency has tracked edits' }
}
& git diff --quiet HEAD
if ($LASTEXITCODE -ne 0) { throw 'Commit Research changes before creating a distributable package' }
$out = Join-Path $repo 'work/dist'
$name = "DjehutiResearch-$Version-windows-x64"
$stage = Join-Path $out $name
if (Test-Path -LiteralPath $stage) { throw 'Use a fresh package version or remove only its previous staging folder explicitly' }
New-Item -ItemType Directory -Path "$stage/licenses" -Force | Out-Null
Copy-Item bin/Release/djehuti_research.exe $stage
Copy-Item LICENSE "$stage/licenses/Research-MIT.txt"
Copy-Item THIRD_PARTY_NOTICES.md $stage
Copy-Item packaging/README-Windows.txt "$stage/README.txt"
Copy-Item packaging/licenses/* "$stage/licenses/"
Copy-Item "$env:JUCE_PATH/LICENSE.md" "$stage/licenses/JUCE.txt"
Copy-Item "$env:FRUSTLANG_ROOT/LICENSE" "$stage/licenses/FRust-MIT.txt"
Copy-Item "$env:FRUST_VCPKG_INSTALLED_DIR/share/llvm/copyright" "$stage/licenses/LLVM-vcpkg.txt"
Copy-Item "$env:FRUST_WINFLEXBISON_DIR/FlexLexer.h" "$stage/licenses/FlexLexer-header.txt"
# Retain the generated parser's copyright and skeleton exception without its
# machine-specific #line directives or the parser implementation.
Get-Content build/deps/frust/generated/parser.cpp -TotalCount 32 | Set-Content "$stage/licenses/Bison-parser-notice.txt"
# Preserve the notices embedded in the actual bundled JUCE dependencies.
$notices = @{
    'juce_core/juce_core.h'='JUCE-core-header.txt'; 'juce_events/juce_events.h'='JUCE-events-header.txt'
    'juce_graphics/image_formats/pnglib/LICENSE'='libpng.txt'
    'juce_graphics/image_formats/jpglib/README'='libjpeg.txt'
    'juce_core/zip/zlib/zlib.h'='zlib-header.txt'
}
foreach ($path in $notices.Keys) { Copy-Item "$env:JUCE_PATH/modules/$path" "$stage/licenses/$($notices[$path])" }
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio 2022 C++ redistributable path unavailable' }
$redist = Get-ChildItem "$vs/VC/Redist/MSVC" -Directory | Where-Object Name -Match '^14\.\d+\.\d+$' | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$crt = Join-Path $redist.FullName 'x64/Microsoft.VC143.CRT'
# Use only Release CRT files listed as Visual Studio distributable code.
$crtNames = @('msvcp140.dll','msvcp140_1.dll','msvcp140_2.dll','msvcp140_atomic_wait.dll','msvcp140_codecvt_ids.dll','vcruntime140.dll','vcruntime140_1.dll','concrt140.dll')
foreach ($dll in $crtNames) { Copy-Item -LiteralPath (Join-Path $crt $dll) -Destination $stage }
$threads = Join-Path $crt 'vcruntime140_threads.dll'
if (Test-Path -LiteralPath $threads) { Copy-Item -LiteralPath $threads -Destination $stage }
$tools = Get-ChildItem "$vs/VC/Tools/MSVC" -Directory | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
$dumpbin = Join-Path $tools.FullName 'bin/Hostx64/x64/dumpbin.exe'
$imports = [ordered]@{}
foreach ($binary in Get-ChildItem $stage -File | Where-Object Extension -In @('.exe','.dll')) {
    $lines = & $dumpbin /nologo /dependents $binary.FullName
    if ($LASTEXITCODE -ne 0) { throw "Import inspection failed: $($binary.Name)" }
    $dlls = @($lines | Where-Object { $_ -match '^\s+[A-Za-z0-9_.-]+\.dll\s*$' } | ForEach-Object { $_.Trim() } | Select-Object -Unique)
    $imports[$binary.Name] = $dlls
    foreach ($dll in $dlls) {
        if (Test-Path -LiteralPath (Join-Path $stage $dll)) { continue }
        if ($dll -like 'api-ms-win-*' -or $dll -like 'ext-ms-win-*') { continue }
        $systemFile = Join-Path "$env:WINDIR/System32" $dll
        if (!(Test-Path -LiteralPath $systemFile) -or (Get-Item -LiteralPath $systemFile).VersionInfo.CompanyName -notlike '*Microsoft*') {
            throw "Unpackaged non-system startup dependency: $($binary.Name) -> $dll"
        }
    }
}
$imports | ConvertTo-Json -Depth 5 | Set-Content work/ci/startup-imports.json
$commit = (& git rev-parse HEAD)
$provenance = [ordered]@{
    version=$Version; commit=$commit; configuration='Release'; architecture='x64'; dependencies=$lock
    runnerImage=$env:ImageOS; runnerVersion=$env:ImageVersion
    cmake=(& cmake --version | Select-Object -First 1)
    msvcRedistVersion=$redist.Name; startupImports=$imports; license='GPL-3.0-only combined executable; MIT project sources'
    files=@(Get-ChildItem $stage -File -Recurse | ForEach-Object { @{path=[IO.Path]::GetRelativePath($stage,$_.FullName); sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(); bytes=$_.Length} })
}
$provenance | ConvertTo-Json -Depth 12 | Set-Content "$stage/build-provenance.json"
$sourceName = "DjehutiResearch-$Version-source.zip"
@"
Corresponding Source for $Version, Research commit $commit
Download $sourceName from this same release:
https://github.com/wwestlake/djehuti-research/releases/tag/$Version
It contains Research, JUCE, FRust source snapshots and the LLVM 18.1.6 source
archive, license notices, and build instructions/scripts. No source access fee.
Research-owned files retain MIT terms; the linked executable uses GPL version 3.
"@ | Set-Content "$stage/SOURCE.txt"
$sourceStage = Join-Path $out "source-$Version"
New-Item -ItemType Directory -Path $sourceStage | Out-Null
foreach ($item in @(@($repo,'Research'), @($env:JUCE_PATH,'JUCE'), @($env:FRUSTLANG_ROOT,'FrustLang'))) {
    & git -C $item[0] archive --format=tar "--output=$sourceStage/$($item[1]).tar" HEAD
    if ($LASTEXITCODE -ne 0) { throw 'Corresponding source archive failed' }
}
if ((Get-FileHash -LiteralPath $env:RESEARCH_LLVM_SOURCE -Algorithm SHA256).Hash.ToLowerInvariant() -ne $lock.llvm.sourceSha256) { throw 'LLVM corresponding source hash mismatch' }
Copy-Item -LiteralPath $env:RESEARCH_LLVM_SOURCE -Destination $sourceStage
if ((Get-FileHash -LiteralPath $env:RESEARCH_VCPKG_SOURCE -Algorithm SHA256).Hash.ToLowerInvariant() -ne $lock.vcpkg.sourceSha256) { throw 'vcpkg source/patch archive hash mismatch' }
Copy-Item -LiteralPath $env:RESEARCH_VCPKG_SOURCE -Destination $sourceStage
Copy-Item "$env:FRUST_VCPKG_INSTALLED_DIR/share/llvm/vcpkg_abi_info.txt" "$sourceStage/LLVM-vcpkg-build-inputs.txt"
Copy-Item docs/RELEASING.md "$sourceStage/BUILD.md"
Copy-Item packaging/licenses/GPL-3.0.txt $sourceStage
Compress-Archive -Path "$sourceStage/*" -DestinationPath "$out/$sourceName" -CompressionLevel Optimal
Compress-Archive -LiteralPath $stage -DestinationPath "$out/$name.zip" -CompressionLevel Optimal
foreach ($file in @("$name.zip",$sourceName)) {
    $hash=(Get-FileHash -LiteralPath "$out/$file" -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $file" | Set-Content -LiteralPath "$out/$file.sha256" -Encoding ascii
}
Get-ChildItem "$out/*.zip*" | Select-Object Name,Length | ConvertTo-Json
