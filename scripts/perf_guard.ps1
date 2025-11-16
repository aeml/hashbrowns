param(
  [switch]$Update,
  [int]$Size = 20000,
  [int]$Runs = 5,
  [string]$Seed = '12345',
  [string]$Structures = 'array,slist,dlist,hashmap',
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

if (-not (Test-Path $Bin)) {
  Write-Host "[INFO] Building project ($BuildType)..." -ForegroundColor Cyan
  & (Join-Path $Root 'scripts' 'build.ps1') -Type $BuildType | Write-Output
  if (-not (Test-Path $Bin)) { $Bin = Join-Path (Join-Path $BuildDir $BuildType) 'hashbrowns.exe' }
}

# Run a fixed-seed small benchmark to produce JSON. If updating, simply
# write the reference baseline; otherwise, use the built-in baseline
# comparison mode in the hashbrowns binary.
Write-Host "[INFO] Running benchmark for perf guard..." -ForegroundColor Cyan
& $Bin --size $Size --runs $Runs --structures $Structures --pattern sequential --seed $Seed --out-format json --output $TmpJson | Write-Output

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
& $Bin --size $Size --runs $Runs --structures $Structures --pattern sequential --seed $Seed --out-format json --output $TmpJson --baseline $BaselineJson --baseline-threshold $TolInsertPct --baseline-noise 1.0 --baseline-scope mean | Write-Output
if ($LASTEXITCODE -ne 0) {
  Write-Error "Performance regression detected (exit code $LASTEXITCODE)"
  exit 2
}
Write-Host "All metrics within tolerance" -ForegroundColor Green
exit 0
