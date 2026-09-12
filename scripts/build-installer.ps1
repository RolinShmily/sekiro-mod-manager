# ==============================================================================
# Sekiro Mod Manager (SMM) - Automated Installer & Packaging Pipeline
# ==============================================================================

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RootDir = (Resolve-Path "$ScriptDir\..").Path
Set-Location $RootDir

Write-Host "=================================================================" -ForegroundColor Cyan
Write-Host " Sekiro Mod Manager (SMM) Automated Packaging Pipeline" -ForegroundColor Cyan
Write-Host " Root Directory: $RootDir" -ForegroundColor Gray
Write-Host "=================================================================" -ForegroundColor Cyan

# Read version from tauri.conf.json
$tauriConfPath = Join-Path $RootDir "apps\smm-desktop\src-tauri\tauri.conf.json"
if (-not (Test-Path $tauriConfPath)) {
    throw "tauri.conf.json not found at $tauriConfPath"
}
$tauriConf = Get-Content $tauriConfPath -Raw | ConvertFrom-Json
$version = $tauriConf.version
if (-not $version) { $version = "0.1.0" }

Write-Host "Target Version: v$version`n" -ForegroundColor Magenta

# 1. Compile release smm-cli (target/release/smm.exe)
Write-Host "[1/5] Compiling release smm-cli (smm.exe)..." -ForegroundColor Yellow
& cargo build -p smm-cli --release
if ($LASTEXITCODE -ne 0) {
    throw "cargo build -p smm-cli --release failed with exit code $LASTEXITCODE"
}

$cliExe = Join-Path $RootDir "target\release\smm.exe"
if (-not (Test-Path $cliExe)) {
    throw "Compiled CLI binary not found at $cliExe"
}
$cliSize = (Get-Item $cliExe).Length
Write-Host "      CLI binary successfully built: $cliExe ($([math]::Round($cliSize/1MB, 2)) MB)" -ForegroundColor Green

# 2. Compile React frontend static bundle
Write-Host "`n[2/5] Compiling React frontend static bundle..." -ForegroundColor Yellow
& pnpm --filter smm-desktop build
if ($LASTEXITCODE -ne 0) {
    throw "pnpm --filter smm-desktop build failed with exit code $LASTEXITCODE"
}
Write-Host "      React frontend dist assets successfully built." -ForegroundColor Green

# 3. Execute Tauri packaging pipeline to generate NSIS installer
Write-Host "`n[3/5] Executing Tauri NSIS packaging pipeline..." -ForegroundColor Yellow
Push-Location (Join-Path $RootDir "apps\smm-desktop")
try {
    & npx @tauri-apps/cli build
    if ($LASTEXITCODE -ne 0) {
        throw "npx @tauri-apps/cli build failed with exit code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}
Write-Host "      Tauri NSIS packaging completed." -ForegroundColor Green

# 4. Archive output into dist-installer/
Write-Host "`n[4/5] Archiving installer artifacts to dist-installer/..." -ForegroundColor Yellow
$distDir = Join-Path $RootDir "dist-installer"
if (-not (Test-Path $distDir)) {
    New-Item -ItemType Directory -Path $distDir -Force | Out-Null
}

$nsisDir = Join-Path $RootDir "target\release\bundle\nsis"
$sourceInstaller = Get-ChildItem -Path $nsisDir -Filter "*setup.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $sourceInstaller) {
    throw "NSIS installer output not found under $nsisDir"
}

$setupExe = Join-Path $distDir "setup.exe"
$versionedExe = Join-Path $distDir "Sekiro-Mod-Manager-Setup-v$version.exe"
$distCliExe = Join-Path $distDir "smm.exe"

Copy-Item -Path $sourceInstaller.FullName -Destination $setupExe -Force
Copy-Item -Path $sourceInstaller.FullName -Destination $versionedExe -Force
if (Test-Path $cliExe) {
    Copy-Item -Path $cliExe -Destination $distCliExe -Force
}

$installerSize = (Get-Item $setupExe).Length
Write-Host "      Archived: $setupExe ($([math]::Round($installerSize/1MB, 2)) MB)" -ForegroundColor Green
Write-Host "      Archived: $versionedExe" -ForegroundColor Green
if (Test-Path $distCliExe) {
    Write-Host "      Archived: $distCliExe (CLI)" -ForegroundColor Green
}

# 5. Compute SHA256 checksums
Write-Host "`n[5/5] Generating SHA256SUMS.txt checksums..." -ForegroundColor Yellow
$checksumFile = Join-Path $distDir "SHA256SUMS.txt"

$hashSetup = (Get-FileHash -Path $setupExe -Algorithm SHA256).Hash.ToLower()
$hashVersioned = (Get-FileHash -Path $versionedExe -Algorithm SHA256).Hash.ToLower()

$checksumLines = @(
    "$hashSetup  setup.exe",
    "$hashVersioned  Sekiro-Mod-Manager-Setup-v$version.exe"
)

# Optional accompanying binaries for verification
$desktopExe = Join-Path $RootDir "target\release\smm-desktop.exe"
if (Test-Path $desktopExe) {
    $hashDesktop = (Get-FileHash -Path $desktopExe -Algorithm SHA256).Hash.ToLower()
    $checksumLines += "$hashDesktop  smm-desktop.exe"
}
if (Test-Path $cliExe) {
    $hashCli = (Get-FileHash -Path $cliExe -Algorithm SHA256).Hash.ToLower()
    $checksumLines += "$hashCli  smm.exe"
}

[System.IO.File]::WriteAllLines($checksumFile, $checksumLines)
Write-Host "      Checksum file generated at: $checksumFile" -ForegroundColor Green
foreach ($line in $checksumLines) {
    Write-Host "      $line" -ForegroundColor DarkCyan
}

Write-Host "`n=================================================================" -ForegroundColor Cyan
Write-Host " All packaging tasks completed successfully!" -ForegroundColor Green
Write-Host " Installer ready at: $setupExe" -ForegroundColor Green
Write-Host "=================================================================" -ForegroundColor Cyan
