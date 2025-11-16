param(
  [int]$Runs = 10,
  [int]$Size = 10000,
  [int]$MaxSize = 32768,
  [string]$Structures = 'array,slist,dlist,hashmap',
  [ValidateSet('csv','json')]
  [string]$OutFormat = 'csv',
  [string]$BenchOut,
  [string]$CrossOut,
  [ValidateSet('Debug','Release','RelWithDebInfo')]
  [string]$BuildType = 'Release',
  [int]$Jobs = 0,
  [string]$CsvDir,
  [string]$PlotsDir,
  [switch]$NoPlots,
  [switch]$Plots,
  [ValidateSet('auto','linear','mid','log')]
  [string]$YScale = 'auto',
  [int]$SeriesRuns = 1,
  [string]$Seed,
  [ValidateSet('sequential','random','mixed')]
  [string]$Pattern = 'sequential',
  [int]$MaxSeconds,
  [ValidateSet('open','chain')]
  [string]$HashStrategy = 'open',
  [Nullable[int]]$HashCapacity,
  [Nullable[double]]$HashLoad
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent
$BuildDir = Join-Path $Root 'build'
$ResultsDir = Join-Path $Root 'results'
if (-not $CsvDir) { $CsvDir = Join-Path $ResultsDir 'csvs' }
if (-not $PlotsDir) { $PlotsDir = Join-Path $ResultsDir 'plots' }
$Bin = Join-Path $BuildDir 'hashbrowns.exe'
if (-not (Test-Path $Bin)) { $Bin = Join-Path (Join-Path $BuildDir $BuildType) 'hashbrowns.exe' }

# Ensure build exists
if (-not (Test-Path $Bin)) {
  Write-Host "[INFO] Building project ($BuildType)..." -ForegroundColor Cyan
  & (Join-Path $Root 'scripts' 'build.ps1') -Type $BuildType -Jobs $Jobs | Write-Output
  if (-not (Test-Path $Bin)) { $Bin = Join-Path (Join-Path $BuildDir $BuildType) 'hashbrowns.exe' }
}

New-Item -ItemType Directory -Force -Path $CsvDir | Out-Null
New-Item -ItemType Directory -Force -Path $PlotsDir | Out-Null

if (-not $BenchOut) { $BenchOut = Join-Path $CsvDir ("benchmark_results." + $OutFormat) }
if (-not $CrossOut) { $CrossOut = Join-Path $CsvDir ("crossover_results." + $OutFormat) }

# Build CLI args
$argsList = @('--structures', $Structures, '--runs', $Runs, '--out-format', $OutFormat)
if ($OutFormat -eq 'csv') { $argsList += @('--output', $BenchOut) } else { $argsList += @('--output', $BenchOut) }

# Single benchmark for --size
$argsListSingle = $argsList + @('--size', $Size)

# Crossover sweep
$argsListCross = $argsList + @('--crossover-analysis', '--max-size', $MaxSize, '--series-runs', $SeriesRuns)

if ($Pattern) { $argsListSingle += @('--pattern', $Pattern); $argsListCross += @('--pattern', $Pattern) }
if ($Seed) { $argsListSingle += @('--seed', $Seed); $argsListCross += @('--seed', $Seed) }
if ($HashStrategy) { $argsListSingle += @('--hash-strategy', $HashStrategy); $argsListCross += @('--hash-strategy', $HashStrategy) }
if ($HashCapacity) { $argsListSingle += @('--hash-capacity', $HashCapacity); $argsListCross += @('--hash-capacity', $HashCapacity) }
if ($HashLoad) { $argsListSingle += @('--hash-load', $HashLoad); $argsListCross += @('--hash-load', $HashLoad) }

Write-Host "[INFO] Running benchmark..." -ForegroundColor Cyan
& $Bin @argsListSingle | Write-Output

Write-Host "[INFO] Running crossover analysis..." -ForegroundColor Cyan
# For crossover, direct output path is controlled inside the app via --output? We'll capture then move if needed via --series-out when present.
# The app writes to stdout unless --output is used; for crossover, it writes to default; we'll use --output to set it.
if ($OutFormat -eq 'csv') {
  $argsListCross += @('--output', $CrossOut)
} else {
  $argsListCross += @('--output', $CrossOut)
}
& $Bin @argsListCross | Write-Output

if (-not $NoPlots -or $Plots) {
  try {
    Write-Host "[INFO] Generating plots..." -ForegroundColor Cyan
    $py = $null
    try { $cmd = Get-Command python -ErrorAction Stop; $py = $cmd.Source } catch {}
    if (-not $py) { try { $cmd = Get-Command py -ErrorAction Stop; $py = $cmd.Source } catch {} }
    if (-not $py) { throw 'Python not found in PATH' }
    & $py (Join-Path $Root 'scripts' 'plot_results.py') --bench-csv $BenchOut --cross-csv $CrossOut --out-dir $PlotsDir --yscale $YScale | Write-Output
  } catch {
    Write-Warning "Plotting skipped: $_"
  }
}
