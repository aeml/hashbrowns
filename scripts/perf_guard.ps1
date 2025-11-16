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

# Run a fixed-seed small benchmark to produce JSON
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

$base = Get-Content $BaselineJson -Raw | ConvertFrom-Json
$cur  = Get-Content $TmpJson -Raw | ConvertFrom-Json

# Build lookup by structure (use approved verb naming)
function ConvertTo-ResultsMap {
  param($obj)
  $map = @{}
  foreach($r in $obj.results){ $map[$r.structure] = $r }
  return $map
}
$mb = ConvertTo-ResultsMap $base
$mc = ConvertTo-ResultsMap $cur

$fail = $false
function PctDelta($a, $b){ if ($a -eq 0) { return 0 } else { return [math]::Abs(($b - $a) / $a) * 100 } }

$structuresList = $Structures.Split(',')
foreach($s in $structuresList){
  if (-not $mb.ContainsKey($s) -or -not $mc.ContainsKey($s)) { Write-Warning "Missing results for $s in baseline or current"; continue }
  $rb = $mb[$s]
  $rc = $mc[$s]
  $di = PctDelta $rb.insert_ms_mean $rc.insert_ms_mean
  $ds = PctDelta $rb.search_ms_mean $rc.search_ms_mean
  $dr = PctDelta $rb.remove_ms_mean $rc.remove_ms_mean
  $okI = $di -le $TolInsertPct
  $okS = $ds -le $TolSearchPct
  $okR = $dr -le $TolRemovePct
  $emoji = if ($okI -and $okS -and $okR) { '✅' } else { '❌' }
  Write-Host ("{0} {1,-8} insert {2,6:N1}% (tol {3}%), search {4,6:N1}% (tol {5}%), remove {6,6:N1}% (tol {7}%)" -f $emoji,$s,$di,$TolInsertPct,$ds,$TolSearchPct,$dr,$TolRemovePct)
  if (-not ($okI -and $okS -and $okR)) { $fail = $true }
}

if ($fail) { Write-Error "Performance regression detected"; exit 2 } else { Write-Host "All metrics within tolerance" -ForegroundColor Green; exit 0 }
