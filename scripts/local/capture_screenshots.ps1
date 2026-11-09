# Capture the GUI screenshots used by docs/design/screenshots.md and the READMEs.
#
# The app is started against a throwaway demo library (scripts/local/seed_demo.ps1), so the images
# show neutral, fictional mods instead of someone's real install. The two roots are passed with
# `--staging=` / `--game-dir=`, which the GUI applies to this process only, so the user's saved
# settings (HKCU) are never touched.
#
#   powershell -ExecutionPolicy Bypass -File scripts/local/capture_screenshots.ps1
#   ... -Root D:\demo -OutDir docs\designssets\screenshots -Theme light
#
# Requires an interactive desktop session: the window is captured through PrintWindow, which needs
# a real (non-headless) window station.

param(
    [string]$Root = 'C:\Users\Public\SMM-Demo',
    [string]$OutDir = (Join-Path $PSScriptRoot '../../docs/design/assets/screenshots'),
    [string]$Exe = '',
    [ValidateSet('dark', 'light', 'system')][string]$Theme = 'dark',
    [switch]$Reseed,
    [switch]$KeepRunning
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class SmmShot {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@

[void][SmmShot]::SetProcessDPIAware()

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$staging = Join-Path $Root 'staging'
$game = Join-Path $Root 'game'

function Find-Exe {
    # Newest wins: a stale preset build renders QML that no longer matches the tree.
    $candidates = @(
        'build\msvc-x64-release\src\gui\smm_gui.exe',
        'build\msvc-x64-debug\src\gui\smm_gui.exe',
        'build\tests-all\src\gui\smm_gui.exe',
        'build\vs2022-x64\src\gui\Release\smm_gui.exe'
    ) | ForEach-Object { Join-Path $repoRoot $_ }
    $found = $candidates | Where-Object { Test-Path $_ } |
        Sort-Object { (Get-Item $_).LastWriteTime } -Descending | Select-Object -First 1
    if (-not $found) { throw "smm_gui.exe not found. Build it first, or pass -Exe." }
    return $found
}

if (-not $Exe) { $Exe = Find-Exe }
if (-not (Test-Path $Exe)) { throw "smm_gui.exe not found at $Exe" }
if ($Reseed -or -not (Test-Path (Join-Path $game 'mods\.smm_manifest.json'))) {
    # The manifest only exists after a deploy, which is what makes the launchpad card meaningful.
    & (Join-Path $PSScriptRoot 'seed_demo.ps1') -Root $Root
    $smm = Get-ChildItem (Join-Path $repoRoot 'build') -Recurse -Filter smm.exe -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $smm) { throw 'smm.exe not found; build the CLI first.' }
    & $smm.FullName --quiet --staging $staging deploy --game-dir $game
    if ($LASTEXITCODE -ne 0) { throw 'deploy into the demo library failed' }
}

# The GUI takes the two roots from the command line for this run only; the user's saved settings
# (HKCU\Software\SekiroModManager\SMM) are neither read nor written here.
$runArgs = @("--theme=$Theme", "--staging=`"$staging`"", "--game-dir=`"$game`"")

$qtBin = 'C:\Qt\6.11.2\msvc2022_64\bin'
if (Test-Path $qtBin) { $env:PATH = "$qtBin;$env:PATH" }

$outPath = [System.IO.Path]::GetFullPath($OutDir)
if (-not (Test-Path $outPath)) { New-Item -ItemType Directory -Force -Path $outPath | Out-Null }

$proc = $null
try {
    $proc = Start-Process -FilePath $Exe -ArgumentList $runArgs -PassThru

    $h = $null
    for ($i = 0; $i -lt 60; $i++) {
        Start-Sleep -Milliseconds 500
        $proc.Refresh()
        $cand = $proc.MainWindowHandle
        if ($null -ne $cand -and $cand.ToInt64() -ne 0) { $h = $cand; break }
    }
    if ($null -eq $h) { throw 'the GUI window never appeared' }

    # The handle exists before the window is shown; wait for real geometry, then for the first frame.
    $rect = New-Object SmmShot+RECT
    for ($j = 0; $j -lt 40; $j++) {
        Start-Sleep -Milliseconds 250
        [void][SmmShot]::GetWindowRect($h, [ref]$rect)
        if (($rect.Right - $rect.Left) -gt 800) { break }
    }
    Start-Sleep -Milliseconds 2000

    function EnsureShown {
        # The window can end up minimized when it is started from a background shell; a minimized
        # window still answers PrintWindow, but with the last frame it composited, which is stale.
        if ([SmmShot]::IsIconic($h)) {
            [void][SmmShot]::ShowWindow($h, 9)
            Start-Sleep -Milliseconds 1200
        }
    }

    function Save-Shot([string]$name) {
        EnsureShown
        Start-Sleep -Milliseconds 400
        $r = New-Object SmmShot+RECT
        [void][SmmShot]::GetWindowRect($h, [ref]$r)
        $w = $r.Right - $r.Left
        $hh = $r.Bottom - $r.Top
        if ($w -le 0 -or $hh -le 0) { throw "bad window rect while capturing $name" }

        $bmp = New-Object System.Drawing.Bitmap $w, $hh
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $hdc = $g.GetHdc()
        $ok = [SmmShot]::PrintWindow($h, $hdc, 2)
        $g.ReleaseHdc($hdc)
        $g.Dispose()
        if (-not $ok) { throw "PrintWindow failed for $name" }

        # PrintWindow includes the resize border; crop to the client area so the PNG matches the
        # window geometry declared in Main.qml (1280x840) without transparent padding.
        $pt = New-Object SmmShot+POINT
        [void][SmmShot]::ClientToScreen($h, [ref]$pt)
        $ox = $pt.X - $r.Left
        $oy = $pt.Y - $r.Top
        $cw = 1280
        $ch = 840
        $crop = New-Object System.Drawing.Bitmap $cw, $ch
        $g2 = [System.Drawing.Graphics]::FromImage($crop)
        $src = New-Object System.Drawing.Rectangle $ox, $oy, $cw, $ch
        $dst = New-Object System.Drawing.Rectangle 0, 0, $cw, $ch
        $g2.DrawImage($bmp, $dst, $src, [System.Drawing.GraphicsUnit]::Pixel)
        $g2.Dispose()
        $bmp.Dispose()

        $suffix = if ($Theme -eq 'light') { '-light' } else { '' }
        $file = Join-Path $outPath "$name$suffix.png"
        $crop.Save($file, [System.Drawing.Imaging.ImageFormat]::Png)
        $crop.Dispose()
        Write-Host "  $name -> $file"
    }

    function Open-Page([int]$y) {
        EnsureShown
        $lp = [IntPtr](($y -shl 16) -bor (100 -band 0xFFFF))
        [void][SmmShot]::PostMessage($h, 0x0200, [IntPtr]0, $lp)
        Start-Sleep -Milliseconds 150
        [void][SmmShot]::PostMessage($h, 0x0201, [IntPtr]1, $lp)
        Start-Sleep -Milliseconds 90
        [void][SmmShot]::PostMessage($h, 0x0202, [IntPtr]0, $lp)
        Start-Sleep -Milliseconds 1600
    }

    # Nav rail item centres, in client coordinates, for the 1280x840 layout in Main.qml.
    $pages = [ordered]@{
        hub = 97
        armoury = 143
        packs = 189
        settings = 235
        about = 281
    }

    Write-Host "capturing $Theme theme into $outPath"
    foreach ($page in $pages.Keys) {
        Open-Page $pages[$page]
        Save-Shot $page
    }
}
finally {
    if ($proc -and -not $KeepRunning) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue }
}
