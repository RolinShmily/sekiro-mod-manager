# Deploy a portable Windows GUI bundle and prune everything the app never loads.
#
# `windeployqt` is deliberately conservative: it copies every Controls style, the QML
# type metadata and the tooling plugins, because it cannot know which of them a program
# imports at run time. SMM is a single-style app (main.cpp pins QtQuick.Controls.Basic),
# so the rest is dead weight in the zip and the installer.
#
# Run it locally before tagging; the release workflow calls the same script, so what you
# measure here is what users download.
#
#   pwsh scripts/package-windows.ps1 -BuildDir build/msvc-x64-release -Version v0.5.0
#
# Produces under -OutDir:
#   sekiro-mod-manager-<version>-windows-x64-gui/      portable bundle
#   sekiro-mod-manager-<version>-windows-x64-gui.zip
#   sekiro-mod-manager-<version>-windows-x64-cli.zip
#   sekiro-mod-manager-<version>-windows-x64-setup.exe   (unless -SkipInstaller)

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
        $match = [regex]::Match($cmake, 'project\(sekiro_mod_manager VERSION ([0-9]+\.[0-9]+\.[0-9]+)')
        if (-not $match.Success) { throw 'Cannot read the project version from CMakeLists.txt.' }
        $Version = "v$($match.Groups[1].Value)"
    }
    $plainVersion = $Version.TrimStart('v')

    $guiExe = Join-Path $BuildDir 'src/gui/smm_gui.exe'
    $cliExe = Join-Path $BuildDir 'src/cli/smm.exe'
    foreach ($exe in @($guiExe, $cliExe)) {
        if (-not (Test-Path $exe)) { throw "Missing $exe. Build the Release preset first." }
    }

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
    $cliDir = Join-Path $OutDir "sekiro-mod-manager-$Version-windows-x64-cli"
    foreach ($dir in @($guiDir, $cliDir)) {
        if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }

    foreach ($document in @('LICENSE', 'THIRD_PARTY_NOTICES.md', 'README.md', 'README.zh-CN.md')) {
        Copy-Item $document -Destination $guiDir
        Copy-Item $document -Destination $cliDir
    }
    Copy-Item $guiExe -Destination (Join-Path $guiDir 'SekiroModManager.exe')
    Copy-Item $cliExe -Destination (Join-Path $guiDir 'smm.exe')
    Copy-Item $cliExe -Destination (Join-Path $cliDir 'smm.exe')

    # --- agent skill -----------------------------------------------------------------------
    # `website/scripts/package_skill.py` owns the file allowlist (it deliberately keeps eval
    # fixtures and local settings out) and the website serves the same archive, so the installer
    # ships that exact zip plus an extracted copy for agents that read the folder directly.
    $skillDir = Join-Path $guiDir 'skill'
    New-Item -ItemType Directory -Force -Path $skillDir | Out-Null
    & python website/scripts/package_skill.py
    if ($LASTEXITCODE -ne 0) { throw "skill packaging failed with exit code $LASTEXITCODE." }
    $skillZip = 'website/public/downloads/smm-cli.zip'
    if (-not (Test-Path $skillZip)) { throw "skill archive not found at $skillZip" }
    Copy-Item $skillZip -Destination (Join-Path $skillDir 'smm-cli.zip') -Force
    Expand-Archive -Path $skillZip -DestinationPath $skillDir -Force

    # --skip-plugin-types drops the QML debugger/tooling plugins (qmltooling) and the
    # touch-screen plugin (generic); neither is reachable from a released build.
    # --no-system-d3d-compiler relies on Windows 10 1809+ shipping d3dcompiler_47.dll, and
    # --no-opengl-sw skips the 20 MB software rasteriser; Quick renders through D3D11 here.
    & $windeployqt --qmldir src/gui/qml --release --no-translations --no-opengl-sw `
        --no-system-d3d-compiler --skip-plugin-types qmltooling,generic `
        --compiler-runtime (Join-Path $guiDir 'SekiroModManager.exe')
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed with exit code $LASTEXITCODE." }

    # --- VC++ runtime ----------------------------------------------------------------------
    # `--compiler-runtime` only deploys the redist when windeployqt can locate the toolset; the CI
    # workflows provide that through msvc-dev-cmd. Fall back to the toolset's redist directory so a
    # Developer PowerShell run produces the same bundle, then mirror it into the CLI package.
    if (-not (Test-Path (Join-Path $guiDir 'MSVCP140.dll')) -and $env:VCToolsRedistDir) {
        $crt = Get-ChildItem (Join-Path $env:VCToolsRedistDir 'x64') -Directory -Filter 'Microsoft.VC*.CRT' -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending | Select-Object -First 1
        if ($crt) { Copy-Item (Join-Path $crt.FullName '*.dll') -Destination $guiDir -Force }
    }
    # windeployqt answers --compiler-runtime with the 25 MB vc_redist installer when it can see the
    # toolset. Shipping the three CRT DLLs (~2.5 MB) covers the same machines at a tenth of the size.
    Remove-Item -Path (Join-Path $guiDir 'vc_redist.x64.exe') -Force -ErrorAction SilentlyContinue
    foreach ($runtime in @('MSVCP140.dll', 'VCRUNTIME140.dll', 'VCRUNTIME140_1.dll')) {
        if ((Test-Path (Join-Path $guiDir $runtime)) -and -not (Test-Path (Join-Path $cliDir $runtime))) {
            Copy-Item (Join-Path $guiDir $runtime) -Destination $cliDir
        }
    }

    # Styles: main.cpp pins Basic, so every other style (and its plugin) is dead code.
    $unusedStyles = @('FluentWinUI3', 'Imagine', 'Material', 'Universal', 'Fusion', 'Windows', 'NativeStyle')
    foreach ($style in $unusedStyles) {
        Remove-Item -Path (Join-Path $guiDir "qml/QtQuick/Controls/$style") -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $guiDir "qml/QtQuick/$style") -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $guiDir "Qt6QuickControls2$style.dll") -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $guiDir "Qt6QuickControls2${style}StyleImpl.dll") -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $guiDir "qtquickcontrols2${style}styleplugin.dll") -Force -ErrorAction SilentlyContinue
    }

    # Runtime never reads the QML type metadata (`*.qmltypes`); only qmllint and Qt Creator do.
    Get-ChildItem -Path $guiDir -Filter '*.qmltypes' -Recurse -File | Remove-Item -Force
    Remove-Item -Path (Join-Path $guiDir 'qmltooling') -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -Path (Join-Path $guiDir 'opengl32sw.dll') -Force -ErrorAction SilentlyContinue
    Remove-Item -Path (Join-Path $guiDir 'D3Dcompiler_47.dll') -Force -ErrorAction SilentlyContinue
    # dxcompiler is only pulled in when windeployqt runs with the MSVC toolset on PATH. Measured on
    # Qt 6.11.2: the app starts with or without it, both on the default D3D11 path and with
    # QSG_RHI_BACKEND=d3d12 (3/3 runs each). Keep it if a future Qt starts compiling RHI shaders
    # at run time.
    Remove-Item -Path (Join-Path $guiDir 'dxcompiler.dll') -Force -ErrorAction SilentlyContinue
    # dxil.dll only carries meaning next to dxcompiler.dll (both are the DXC HLSL toolchain), so it
    # goes with it. Verified: the app starts 3/3 without either, on the default and on D3D12.
    Remove-Item -Path (Join-Path $guiDir 'dxil.dll') -Force -ErrorAction SilentlyContinue
    # Do NOT prune Qt6OpenGL.dll: Qt6Quick.dll imports it directly (dumpbin /dependents), and the
    # bundle then dies at process start with "missing DLL" (exit 127) before any QML loads.

    # --- post-prune assertions -------------------------------------------------------------
    # A bundle that cannot start is worse than a large one: check the imports the runtime cannot
    # live without, so a pruning change fails here instead of on a user's machine.
    $required = @(
        'SekiroModManager.exe',
        'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Qml.dll', 'Qt6Quick.dll', 'Qt6OpenGL.dll',
        'Qt6QuickControls2.dll', 'Qt6QuickControls2Basic.dll',
        'platforms/qwindows.dll',
        'qml/QtQuick/Controls/Basic',
        'smm.exe',
        'skill/smm-cli.zip',
        'skill/smm-cli/SKILL.md'
    )
    $missing = @()
    foreach ($item in $required) {
        if (-not (Test-Path (Join-Path $guiDir $item))) { $missing += $item }
    }
    if ($missing.Count -gt 0) {
        throw "pruned bundle is missing required entries: $($missing -join ', ')"
    }
    if (-not (Test-Path (Join-Path $guiDir 'MSVCP140.dll'))) {
        Write-Warning 'VC++ runtime not bundled: these packages rely on a system-installed vc_redist. Run from a Developer PowerShell for VS (or set VCToolsRedistDir) to embed it.'
    }

    $guiZip = Join-Path $OutDir "sekiro-mod-manager-$Version-windows-x64-gui.zip"
    $cliZip = Join-Path $OutDir "sekiro-mod-manager-$Version-windows-x64-cli.zip"
    if (Test-Path $guiZip) { Remove-Item $guiZip -Force }
    if (Test-Path $cliZip) { Remove-Item $cliZip -Force }
    Compress-Archive -Path $guiDir -DestinationPath $guiZip
    Compress-Archive -Path $cliDir -DestinationPath $cliZip

    $guiMb = [math]::Round((Get-ChildItem $guiDir -Recurse -File | Measure-Object Length -Sum).Sum / 1MB, 1)
    Write-Host "GUI bundle: $guiMb MB (uncompressed), $(Get-Item $guiZip | Select-Object -ExpandProperty Length | ForEach-Object { [math]::Round($_ / 1MB, 1) }) MB zipped"

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
