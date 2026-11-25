# Deploy a portable Windows GUI bundle, prune unused Qt runtime files, and build the Setup installer.
#
# Run it locally before tagging; .github/workflows/build.yml calls the same script, so local runs
# produce the exact same artifacts that releases publish.
#
#   pwsh scripts/package-windows.ps1 -BuildDir build/msvc-x64-release -Version v0.4.8
#
# Produces under -OutDir:
#   sekiro-mod-manager-<version>-windows-x64-gui/      portable bundle
#   sekiro-mod-manager-<version>-windows-x64-gui.zip
#   sekiro-mod-manager-<version>-windows-x64-setup.exe (unless -SkipInstaller)

[CmdletBinding()]
param(
    [string]$BuildDir = 'build/msvc-x64-release',
    [string]$OutDir = 'dist',
    [string]$Version = '',
    [switch]$SkipInstaller
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    $BuildDir = (Resolve-Path $BuildDir).Path
    if (-not [System.IO.Path]::IsPathRooted($OutDir)) { $OutDir = Join-Path $repoRoot $OutDir }
    $OutDir = [System.IO.Path]::GetFullPath($OutDir)
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

    if (-not $Version) {
        $cmake = Get-Content CMakeLists.txt -Raw
        $match = [regex]::Match($cmake, 'set\(SMM_APP_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"')
        if (-not $match.Success) { throw 'Cannot read SMM_APP_VERSION from CMakeLists.txt.' }
        $Version = "v$($match.Groups[1].Value)"
    }
    $plainVersion = $Version.TrimStart('v')

    $guiExe = Join-Path $BuildDir 'src/gui/smm_gui.exe'
    if (-not (Test-Path $guiExe)) { throw "Missing $guiExe. Build the Release preset first." }

    # windeployqt ships beside the Qt libraries it deploys; honour Qt's own discovery order.
    $qtPrefix = $env:QT_ROOT_DIR
    if (-not $qtPrefix) { $qtPrefix = $env:CMAKE_PREFIX_PATH }
    if (-not $qtPrefix -and $env:QTDIR) { $qtPrefix = $env:QTDIR }
    $windeployqt = if ($qtPrefix) { Join-Path $qtPrefix 'bin/windeployqt.exe' } else { $null }
    if (-not $windeployqt -or -not (Test-Path $windeployqt)) {
        $windeployqt = (Get-Command windeployqt.exe -ErrorAction SilentlyContinue).Source
    }
    if (-not $windeployqt) { throw 'windeployqt.exe not found. Set QT_ROOT_DIR/CMAKE_PREFIX_PATH or put it on PATH.' }

    $guiDir = Join-Path $OutDir "sekiro-mod-manager-$Version-windows-x64-gui"
    if (Test-Path $guiDir) { Remove-Item -Recurse -Force $guiDir }
    New-Item -ItemType Directory -Force -Path $guiDir | Out-Null

    Copy-Item 'LICENSE' -Destination (Join-Path $guiDir 'LICENSE.txt')
    Copy-Item 'THIRD_PARTY_NOTICES' -Destination (Join-Path $guiDir 'THIRD_PARTY_NOTICES.txt')
    foreach ($document in @('README.md', 'README.zh-CN.md')) {
        if (Test-Path $document) { Copy-Item $document -Destination $guiDir }
    }
    Copy-Item $guiExe -Destination (Join-Path $guiDir 'SekiroModManager.exe')

    # --skip-plugin-types drops the QML debugger/tooling plugins (qmltooling) and touch-screen plugin.
    # --no-system-d3d-compiler relies on Windows 10 1809+ shipping d3dcompiler_47.dll.
    # --no-opengl-sw skips the 20 MB software rasteriser; Qt Quick renders through D3D11.
    & $windeployqt --qmldir src/gui/qml --release --no-translations --no-opengl-sw `
        --no-system-d3d-compiler --skip-plugin-types qmltooling,generic `
        --compiler-runtime (Join-Path $guiDir 'SekiroModManager.exe')
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed with exit code $LASTEXITCODE." }

    # --- VC++ runtime ----------------------------------------------------------------------
    if (-not (Test-Path (Join-Path $guiDir 'MSVCP140.dll')) -and $env:VCToolsRedistDir) {
        $crt = Get-ChildItem (Join-Path $env:VCToolsRedistDir 'x64') -Directory -Filter 'Microsoft.VC*.CRT' -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending | Select-Object -First 1
        if ($crt) { Copy-Item (Join-Path $crt.FullName '*.dll') -Destination $guiDir -Force }
    }
    Remove-Item -Path (Join-Path $guiDir 'vc_redist.x64.exe') -Force -ErrorAction SilentlyContinue

    # Styles: main.cpp pins QtQuick.Controls.Basic, so every other style plugin is unused.
    $unusedStyles = @('FluentWinUI3', 'Imagine', 'Material', 'Universal', 'Fusion', 'Windows', 'NativeStyle')
    foreach ($style in $unusedStyles) {
        Remove-Item -Path (Join-Path $guiDir "qml/QtQuick/Controls/$style") -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $guiDir "qml/QtQuick/$style") -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $guiDir "Qt6QuickControls2$style.dll") -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $guiDir "Qt6QuickControls2${style}StyleImpl.dll") -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $guiDir "qtquickcontrols2${style}styleplugin.dll") -Force -ErrorAction SilentlyContinue
    }

    # Prune tooling metadata and unused shader compiler DLLs.
    Get-ChildItem -Path $guiDir -Filter '*.qmltypes' -Recurse -File | Remove-Item -Force
    Remove-Item -Path (Join-Path $guiDir 'qmltooling') -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -Path (Join-Path $guiDir 'opengl32sw.dll') -Force -ErrorAction SilentlyContinue
    Remove-Item -Path (Join-Path $guiDir 'D3Dcompiler_47.dll') -Force -ErrorAction SilentlyContinue
    Remove-Item -Path (Join-Path $guiDir 'dxcompiler.dll') -Force -ErrorAction SilentlyContinue
    Remove-Item -Path (Join-Path $guiDir 'dxil.dll') -Force -ErrorAction SilentlyContinue
    # Keep Qt6OpenGL.dll: Qt6Quick.dll imports it directly.

    # --- post-prune assertions -------------------------------------------------------------
    $required = @(
        'SekiroModManager.exe',
        'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Qml.dll', 'Qt6Quick.dll', 'Qt6OpenGL.dll',
        'Qt6QuickControls2.dll', 'Qt6QuickControls2Basic.dll',
        'platforms/qwindows.dll',
        'qml/QtQuick/Controls/Basic',
        'LICENSE.txt',
        'THIRD_PARTY_NOTICES.txt'
    )
    $missing = @()
    foreach ($item in $required) {
        if (-not (Test-Path (Join-Path $guiDir $item))) { $missing += $item }
    }
    if ($missing.Count -gt 0) {
        throw "pruned bundle is missing required entries: $($missing -join ', ')"
    }
    if (-not (Test-Path (Join-Path $guiDir 'MSVCP140.dll'))) {
        Write-Warning 'VC++ runtime not bundled: run from a Developer PowerShell for VS (or set VCToolsRedistDir) to embed it.'
    }

    $guiZip = Join-Path $OutDir "sekiro-mod-manager-$Version-windows-x64-gui.zip"
    if (Test-Path $guiZip) { Remove-Item $guiZip -Force }
    Compress-Archive -Path $guiDir -DestinationPath $guiZip

    $guiMb = [math]::Round((Get-ChildItem $guiDir -Recurse -File | Measure-Object Length -Sum).Sum / 1MB, 1)
    $zipMb = [math]::Round((Get-Item $guiZip).Length / 1MB, 1)
    Write-Host "GUI bundle: $guiMb MB (uncompressed), $zipMb MB zipped"

    if (-not $SkipInstaller) {
        $iscc = (Get-Command ISCC.exe -ErrorAction SilentlyContinue).Source
        if (-not $iscc) {
            $candidate = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe'
            if (Test-Path $candidate) { $iscc = $candidate }
        }
        if ($iscc) {
            & $iscc "/DMyAppVersion=$Version" "/DSourceDir=$guiDir" "/DOutputDir=$OutDir" installer/installer.iss
            if ($LASTEXITCODE -ne 0) { throw "ISCC failed with exit code $LASTEXITCODE." }
        } else {
            Write-Warning 'ISCC.exe not found; skipped the Inno Setup installer.'
        }
    }

    Write-Host "Packaging complete: $OutDir ($plainVersion)"
}
finally {
    Pop-Location
}
