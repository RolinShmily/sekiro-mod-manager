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

# 0. Compliance guard: third-party license texts must be present before we build anything.
#    Shipping binaries without them would breach the SIL OFL (bundled subset fonts) and the
#    RARLAB UnRAR license (statically linked via unrar_sys). Fail fast, before the long build.
$requiredLicenseFiles = @(
    "LICENSE",
    "NOTICE",
    "licenses\README.md",
    "licenses\OFL-1.1.txt",
    "licenses\UnRAR.txt",
    "licenses\THIRD-PARTY-NOTICES.md"
)
$missing = @()
foreach ($rel in $requiredLicenseFiles) {
    if (-not (Test-Path (Join-Path $RootDir $rel))) { $missing += $rel }
}
if ($missing.Count -gt 0) {
    throw "Compliance guard failed: missing third-party license file(s): $($missing -join ', ')"
}
Write-Host "[0/5] Compliance guard passed: all $($requiredLicenseFiles.Count) license files present.`n" -ForegroundColor Green

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

# 1) Standalone Portable GUI: Sekiro-Mod-Manager.exe (no setup wizard required)
$desktopExe = Join-Path $RootDir "target\release\smm-desktop.exe"
$distStandaloneExe = Join-Path $distDir "Sekiro-Mod-Manager.exe"
if (Test-Path $desktopExe) {
    Copy-Item -Path $desktopExe -Destination $distStandaloneExe -Force
    $desktopSize = (Get-Item $distStandaloneExe).Length
    Write-Host "      Archived Standalone GUI: $distStandaloneExe ($([math]::Round($desktopSize/1MB, 2)) MB)" -ForegroundColor Green
}

# 2) Single Windows Installer: Sekiro-Mod-Manager-Setup.exe
$distSetupExe = Join-Path $distDir "Sekiro-Mod-Manager-Setup.exe"
Copy-Item -Path $sourceInstaller.FullName -Destination $distSetupExe -Force
$installerSize = (Get-Item $distSetupExe).Length
Write-Host "      Archived Installer: $distSetupExe ($([math]::Round($installerSize/1MB, 2)) MB)" -ForegroundColor Green

# 3) Standalone CLI: smm-cli.exe
$cliCandidate = Join-Path $RootDir "target\release\smm-cli.exe"
if (-not (Test-Path $cliCandidate)) {
    $cliCandidate = Join-Path $RootDir "target\release\smm.exe"
}
$distCliExe = Join-Path $distDir "smm-cli.exe"
if (Test-Path $cliCandidate) {
    Copy-Item -Path $cliCandidate -Destination $distCliExe -Force
    Write-Host "      Archived CLI: $distCliExe" -ForegroundColor Green
}

# 4) License & notice files: shipped alongside the portable exes (Sekiro-Mod-Manager.exe is a
#    single-file binary with no resource directory of its own) and inside the NSIS installer
#    (via `bundle.resources` in tauri.conf.json).
$distLicensesDir = Join-Path $distDir "licenses"
if (Test-Path $distLicensesDir) { Remove-Item -Path $distLicensesDir -Recurse -Force }
New-Item -ItemType Directory -Path $distLicensesDir -Force | Out-Null

Copy-Item -Path (Join-Path $RootDir "LICENSE") -Destination (Join-Path $distDir "LICENSE") -Force
Copy-Item -Path (Join-Path $RootDir "NOTICE") -Destination (Join-Path $distDir "NOTICE") -Force
Copy-Item -Path (Join-Path $RootDir "licenses\*") -Destination $distLicensesDir -Force
$distLicenseCount = (Get-ChildItem -Path $distLicensesDir -File).Count
Write-Host "      Archived LICENSE, NOTICE and $distLicenseCount license file(s) to dist-installer/" -ForegroundColor Green

# Clean up legacy redundant files if present
$legacySetup = Join-Path $distDir "setup.exe"
if (Test-Path $legacySetup) { Remove-Item -Path $legacySetup -Force }
$legacySmm = Join-Path $distDir "smm.exe"
if (Test-Path $legacySmm) { Remove-Item -Path $legacySmm -Force }
Get-ChildItem -Path $distDir -Filter "Sekiro-Mod-Manager-Setup-v*.exe" | ForEach-Object { Remove-Item $_.FullName -Force }

# 5. Compute SHA256 checksums
Write-Host "`n[5/5] Generating SHA256SUMS.txt checksums..." -ForegroundColor Yellow
$checksumFile = Join-Path $distDir "SHA256SUMS.txt"

function Get-Sha256Hex($filePath) {
    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    $stream = [System.IO.File]::OpenRead($filePath)
    try {
        $bytes = $sha256.ComputeHash($stream)
        return (-join ($bytes | ForEach-Object { "{0:x2}" -f $_ }))
    } finally {
        $stream.Close()
    }
}

$checksumLines = @()
if (Test-Path $distStandaloneExe) {
    $hashStandalone = Get-Sha256Hex $distStandaloneExe
    $checksumLines += "$hashStandalone  Sekiro-Mod-Manager.exe"
}
if (Test-Path $distSetupExe) {
    $hashSetup = Get-Sha256Hex $distSetupExe
    $checksumLines += "$hashSetup  Sekiro-Mod-Manager-Setup.exe"
}
if (Test-Path $distCliExe) {
    $hashCli = Get-Sha256Hex $distCliExe
    $checksumLines += "$hashCli  smm-cli.exe"
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
