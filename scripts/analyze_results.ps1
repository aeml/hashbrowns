param(
  [string]$BenchCsv = (Join-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent) 'results/csvs') 'benchmark_results.csv'),
  [string]$CrossCsv = (Join-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent) 'results/csvs') 'crossover_results.csv')
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent
$py = $null
try { $cmd = Get-Command python -ErrorAction Stop; $py = $cmd.Source } catch {}
if (-not $py) { try { $cmd = Get-Command py -ErrorAction Stop; $py = $cmd.Source } catch {} }
if (-not $py) { throw 'Python not found in PATH' }

& $py (Join-Path $Root 'scripts' 'analyze_results.py') | Write-Output
