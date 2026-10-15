# Run only this recorder elevated; launch benchmark processes from a normal shell.
# The fixed CPU.light profile records CSwitch/ReadyThread without stack walking.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [ValidateRange(15,600)][int]$DurationSeconds = 240
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../output/perf-scheduler'))
$destination = (Resolve-Path -LiteralPath $OutputDirectory).Path
if (-not $destination.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Use a fresh subdirectory of output/perf-scheduler.'
}
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'ETW scheduler recording requires Windows administrator/UAC elevation.'
}
$statusFile = Join-Path $destination 'recorder-status.json'
if (Test-Path -LiteralPath $statusFile) { throw 'Recorder directory was already used.' }
$log = Join-Path $destination 'recorder.log'
$trace = Join-Path $destination 'scheduler.etl'
$owned = $false
function Write-Status([string]$state, [string]$detail) {
    @{state=$state; detail=$detail; utc=[DateTime]::UtcNow.ToString('o'); pid=$PID} |
        ConvertTo-Json | Set-Content -LiteralPath $statusFile -Encoding UTF8
}
function Invoke-Wpr([string]$phase, [string[]]$arguments) {
    # WPR writes progress to stderr. Do not let Windows PowerShell turn that
    # stream into a terminating NativeCommandError during the final merge.
    $process = Start-Process -FilePath "$env:WINDIR/System32/wpr.exe" -NoNewWindow -ArgumentList $arguments -Wait -PassThru `
        -RedirectStandardOutput (Join-Path $destination ($phase + '.stdout.txt')) `
        -RedirectStandardError (Join-Path $destination ($phase + '.stderr.txt'))
    $process.ExitCode
}
try {
    $startCode = Invoke-Wpr 'start' @('-start','CPU.light','-filemode','-recordtempto',('"' + $destination + '"'))
    if ($startCode -ne 0) { throw "WPR start failed ($startCode). An existing trace is never cancelled." }
    $owned = $true
    Write-Status 'recording' 'CPU.light; waiting for STOP file or safety timeout'
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while ($watch.Elapsed.TotalSeconds -lt $DurationSeconds -and -not (Test-Path -LiteralPath (Join-Path $destination 'STOP'))) {
        Start-Sleep -Milliseconds 200
    }
    $null = Invoke-Wpr 'collectors' @('-status','collectors')
}
catch {
    $_ | Out-File -LiteralPath $log -Append -Encoding UTF8
    Write-Status 'failed' $_.Exception.Message
}
finally {
    if ($owned) {
        Write-Status 'merging' 'Stopping the recording started by this helper'
        try {
            $stopCode = Invoke-Wpr 'stop' @('-stop',('"' + $trace + '"'),'"Silver-K KS scheduler comparison"','-skipPdbGen')
            if ($stopCode -eq 0 -and (Test-Path -LiteralPath $trace)) {
                Write-Status 'complete' $trace
            } else {
                Write-Status 'failed' "WPR stop failed ($stopCode); inspect stop stdout/stderr and WPR status"
            }
        } catch {
            $_ | Out-File -LiteralPath $log -Append -Encoding UTF8
            Write-Status 'failed' $_.Exception.Message
        }
    }
}
