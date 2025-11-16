param(
    [switch]$Clean,
    [ValidateSet('Debug','Release','RelWithDebInfo')]
    [string]$Type = 'Release',
    [switch]$Test,
    [int]$Jobs = 0
)

$ErrorActionPreference = 'Stop'

function Get-ExePath([string]$BuildDir, [string]$Name, [string]$Config){
    $flat = Join-Path $BuildDir "$Name.exe"
    if (Test-Path $flat) { return $flat }
    $configPath = Join-Path (Join-Path $BuildDir $Config) "$Name.exe"
    if (Test-Path $configPath) { return $configPath }
    return $flat
}

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent
$BuildDir = Join-Path $Root 'build'

if ($Clean) {
    if (Test-Path $BuildDir) {
        Write-Host "[INFO] Removing build directory: $BuildDir"
        Remove-Item -Recurse -Force $BuildDir
    }
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

function Get-CMakeGenerator {
    # Prefer Visual Studio if present; else Ninja; else default
    try {
        # VS 2022
        if (Test-Path "C:\\Program Files\\Microsoft Visual Studio\\2022") {
            return "Visual Studio 17 2022"
        }
        # VS 2019
        if (Test-Path "C:\\Program Files(x86)\\Microsoft Visual Studio\\2019") {
            return "Visual Studio 16 2019"
        }
    } catch {}
    if (Get-Command ninja -ErrorAction SilentlyContinue) { return "Ninja" }
    return $null
}

Push-Location $BuildDir
try {
    $genArgs = @('-S', $Root, '-B', $BuildDir, "-DCMAKE_BUILD_TYPE=$Type")
    $gen = Get-CMakeGenerator
    if ($gen) {
        $genArgs = @('-G', $gen) + $genArgs
        if ($gen -like 'Visual Studio*') { $genArgs += @('-A','x64') }
    }
    Write-Host "[INFO] Configuring CMake ($Type)" -ForegroundColor Cyan
    cmake @genArgs | Write-Output

    $buildArgs = @('--build', $BuildDir, '--config', $Type)
    if ($Jobs -gt 0) { $buildArgs += @('--', "-j$Jobs") }
    Write-Host "[INFO] Building" -ForegroundColor Cyan
    cmake @buildArgs | Write-Output

    if ($Test) {
        $unit = Get-ExePath $BuildDir 'unit_tests' $Type
        if (Test-Path $unit) {
            Write-Host "[INFO] Running unit tests: $unit" -ForegroundColor Cyan
            & $unit | Write-Output
        } else {
            Write-Host "[WARN] unit_tests not found; trying ctest" -ForegroundColor Yellow
            ctest -C $Type --output-on-failure | Write-Output
        }
    }
}
finally {
    Pop-Location
}
