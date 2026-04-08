param(
  [switch]$Update,
  [Nullable[int]]$Size = $null,
  [Nullable[int]]$Runs = $null,
  [string]$Seed = $null,
  [string]$Structures = $null,
  [int]$TolInsertPct = 20,
  [int]$TolSearchPct = 20,
  [int]$TolRemovePct = 20,
  [ValidateSet('Debug','Release','RelWithDebInfo')]
  [string]$BuildType = 'Release'
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent
$BuildDir = Join-Path $Root 'build'
$Bin = Join-Path $BuildDir 'hashbrowns.exe'
if (-not (Test-Path $Bin)) { $Bin = Join-Path (Join-Path $BuildDir $BuildType) 'hashbrowns.exe' }
$BaselineDir = Join-Path $Root 'perf_baselines'
$BaselineJson = Join-Path $BaselineDir 'baseline.json'
$TmpJson = Join-Path $BuildDir 'perf_guard_current.json'
$ReportJson = Join-Path $BuildDir 'perf_guard_report.json'
$RunArgs = @('--no-banner', '--profile', 'ci', '--output', $TmpJson, '--out-format', 'json')
if ($PSBoundParameters.ContainsKey('Size')) { $RunArgs += @('--size', $Size) }
if ($PSBoundParameters.ContainsKey('Runs')) { $RunArgs += @('--runs', $Runs) }
if ($PSBoundParameters.ContainsKey('Structures')) { $RunArgs += @('--structures', $Structures) }
if ($PSBoundParameters.ContainsKey('Seed')) { $RunArgs += @('--seed', $Seed) }

if (-not (Test-Path $Bin)) {
  Write-Host "[INFO] Building project ($BuildType)..." -ForegroundColor Cyan
  & (Join-Path $Root 'scripts' 'build.ps1') -Type $BuildType | Write-Output
  if (-not (Test-Path $Bin)) { $Bin = Join-Path (Join-Path $BuildDir $BuildType) 'hashbrowns.exe' }
}

# Run a fixed-seed small benchmark to produce JSON. If updating, simply
# write the reference baseline; otherwise, use the built-in baseline
# comparison mode in the hashbrowns binary.
Write-Host "[INFO] Running benchmark for perf guard..." -ForegroundColor Cyan
& $Bin @RunArgs | Write-Output

if ($Update) {
  New-Item -ItemType Directory -Force -Path $BaselineDir | Out-Null
  Copy-Item -Force $TmpJson $BaselineJson
  Write-Host "[INFO] Baseline updated: $BaselineJson" -ForegroundColor Green
  exit 0
}

if (-not (Test-Path $BaselineJson)) {
  Write-Error "Baseline not found: $BaselineJson. Run with -Update to create it."
  exit 3
}

# Delegate comparison to the native baseline mode in the executable.
Write-Host "[INFO] Comparing against baseline via built-in checker..." -ForegroundColor Cyan
if (Test-Path $ReportJson) { Remove-Item -Force $ReportJson }
$CompareArgs = $RunArgs + @('--baseline', $BaselineJson, '--baseline-threshold', $TolInsertPct, '--baseline-noise', '1.0', '--baseline-scope', 'mean', '--baseline-report-json', $ReportJson)
& $Bin @CompareArgs | Write-Output
if ($LASTEXITCODE -ne 0) {
  Write-Error "Performance regression detected (exit code $LASTEXITCODE). See $ReportJson"
  exit 2
}
if (-not (Test-Path $ReportJson)) {
  Write-Error "Expected perf guard report was not written: $ReportJson. Rebuild hashbrowns so the current binary supports --baseline-report-json."
  exit 3
}
Write-Host "All metrics within tolerance. Report: $ReportJson" -ForegroundColor Green
exit 0
