param(
    [string]$Executable = (Join-Path $PSScriptRoot '..\bin\Debug\djehuti_research.exe'),
    [switch]$Gui,
    [string]$SettingsDirectory = (Join-Path $PSScriptRoot '..\work\gui-settings')
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$fixture = Join-Path $repo ('work\acceptance\run-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
$denied = Join-Path $fixture 'denied'
New-Item -ItemType Directory -Path $denied | Out-Null
$originalAcl = Get-Acl -LiteralPath $denied
$testAcl = Get-Acl -LiteralPath $denied
$user = [Security.Principal.WindowsIdentity]::GetCurrent().User
$rule = [Security.AccessControl.FileSystemAccessRule]::new($user, [Security.AccessControl.FileSystemRights]::Write, [Security.AccessControl.AccessControlType]::Deny)
$testAcl.AddAccessRule($rule)

# OS connection tables, queried without opening a socket. Only rows belonging
# to the launched test PID are counted or retained.
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ResearchSocketObservation {
  [DllImport("iphlpapi.dll")] static extern uint GetExtendedTcpTable(IntPtr p, ref int size, bool order, int family, int cls, uint reserved);
  [DllImport("iphlpapi.dll")] static extern uint GetExtendedUdpTable(IntPtr p, ref int size, bool order, int family, int cls, uint reserved);
  public static int Count(int processId) {
    int count = 0;
    foreach (int family in new[] {2,23}) foreach (bool tcp in new[] {true,false}) {
      int size = 0;
      if(tcp) GetExtendedTcpTable(IntPtr.Zero, ref size, false, family, 5, 0);
      else GetExtendedUdpTable(IntPtr.Zero, ref size, false, family, 1, 0);
      if(size <= 0) continue;
      IntPtr data = Marshal.AllocHGlobal(size);
      try {
        uint error = tcp ? GetExtendedTcpTable(data, ref size, false, family, 5, 0) : GetExtendedUdpTable(data, ref size, false, family, 1, 0);
        if(error != 0) throw new Exception("Connection table failed: " + error);
        int rows = Marshal.ReadInt32(data);
        int rowSize = tcp ? (family == 2 ? 24 : 56) : (family == 2 ? 12 : 28);
        int pidOffset = rowSize - 4;
        for(int i=0;i<rows;i++) if(Marshal.ReadInt32(data, 4+i*rowSize+pidOffset)==processId) count++;
      } finally { Marshal.FreeHGlobal(data); }
    }
    return count;
  }
}
'@
$testProcess = $null
$samples = 0; $endpoints = 0
try {
    Set-Acl -LiteralPath $denied -AclObject $testAcl
    $start = [Diagnostics.ProcessStartInfo]::new([IO.Path]::GetFullPath($Executable))
    if ($Gui) {
        $start.ArgumentList.Add('--settings-dir'); $start.ArgumentList.Add([IO.Path]::GetFullPath($SettingsDirectory))
    } else {
        $start.ArgumentList.Add('--foundation-check'); $start.ArgumentList.Add($fixture)
    }
    $start.WorkingDirectory = $repo
    $start.UseShellExecute = $false; $start.CreateNoWindow = $true
    $start.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    $start.RedirectStandardOutput = $true; $start.RedirectStandardError = $true
    $testProcess = [Diagnostics.Process]::Start($start)
    $stdout = $testProcess.StandardOutput.ReadToEndAsync(); $stderr = $testProcess.StandardError.ReadToEndAsync()
    Write-Output ('Started Research PID ' + $testProcess.Id + '; observation directory: ' + $fixture)
    $deadline = [DateTime]::UtcNow.AddSeconds($(if ($Gui) { 900 } else { 90 }))
    while (-not $testProcess.HasExited) {
        $endpoints += [ResearchSocketObservation]::Count($testProcess.Id); $samples++
        if ([DateTime]::UtcNow -gt $deadline) { throw 'Research test exceeded its bounded time limit.' }
        Start-Sleep -Milliseconds 20
    }
    $testProcess.WaitForExit()
    [IO.File]::WriteAllText((Join-Path $fixture 'stdout.log'), $stdout.GetAwaiter().GetResult())
    [IO.File]::WriteAllText((Join-Path $fixture 'stderr.log'), $stderr.GetAwaiter().GetResult())
    $result = [ordered]@{pid=$testProcess.Id; exitCode=$testProcess.ExitCode; processExited=$testProcess.HasExited; endpointSamples=$samples; observedTcpUdpEndpoints=$endpoints; aclRestored=$false}
    $testExit = $testProcess.ExitCode
} finally {
    Set-Acl -LiteralPath $denied -AclObject $originalAcl
    if ($testProcess -and -not $testProcess.HasExited) {
        $testProcess.CloseMainWindow() | Out-Null
        if (-not $testProcess.WaitForExit(5000)) { $testProcess.Kill(); $testProcess.WaitForExit() }
    }
}
$result.aclRestored = $true
if (-not $Gui -and $testExit -eq 0) {
    $start.ArgumentList.Clear(); $start.ArgumentList.Add('--verify-restored'); $start.ArgumentList.Add($fixture)
    $restart = [Diagnostics.Process]::Start($start)
    $restartOut = $restart.StandardOutput.ReadToEndAsync(); $restartError = $restart.StandardError.ReadToEndAsync()
    $restartSamples = 0; $restartEndpoints = 0; $deadline = [DateTime]::UtcNow.AddSeconds(90)
    try {
        while (-not $restart.HasExited) {
            $restartEndpoints += [ResearchSocketObservation]::Count($restart.Id); $restartSamples++
            if ([DateTime]::UtcNow -gt $deadline) { throw 'Restoration test exceeded 90 seconds.' }
            Start-Sleep -Milliseconds 20
        }
        $restart.WaitForExit()
    } finally {
        if (-not $restart.HasExited) {
            $restart.CloseMainWindow() | Out-Null
            if (-not $restart.WaitForExit(5000)) { $restart.Kill(); $restart.WaitForExit() }
        }
    }
    [IO.File]::WriteAllText((Join-Path $fixture 'restart-stdout.log'), $restartOut.GetAwaiter().GetResult())
    [IO.File]::WriteAllText((Join-Path $fixture 'restart-stderr.log'), $restartError.GetAwaiter().GetResult())
    $result.restartPid = $restart.Id; $result.restartExitCode = $restart.ExitCode; $result.restartProcessExited = $restart.HasExited
    $result.restartEndpointSamples = $restartSamples; $result.restartObservedTcpUdpEndpoints = $restartEndpoints
    if ($restart.ExitCode -ne 0) { $testExit = $restart.ExitCode }
    $endpoints += $restartEndpoints
}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $fixture 'process-observation.json')
Write-Output ('Report: ' + (Join-Path $fixture 'foundation-check.json'))
$totalSamples = $samples
if ($result.Contains('restartEndpointSamples')) { $totalSamples += $result.restartEndpointSamples }
Write-Output ('Process exit: ' + $testExit + '; TCP/UDP endpoints observed: ' + $endpoints + ' across ' + $totalSamples + ' samples; ACL restored.')
if ($endpoints -ne 0) { exit 2 }
exit $testExit
