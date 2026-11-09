# Seed a demo library so every screen has something real to show.
#
# Half of this UI is about conflicts and precedence, and those are exactly the states you cannot
# review against an empty staging area. The library below is laid out deliberately:
#
#   * two parameter mods sharing param\gameparam\gameparam.parambnd.dcx  -> a CRITICAL conflict
#   * two weapon mods sharing parts\wp_a_0300.partsbnd.dcx               -> an exclusive-slot clash
#   * a mod that loses every file it has                                 -> "fully taken over" state
#   * several clean mods                                                 -> bands in the fingerprint
#
# Idempotent: re-running replaces the same mods. It also points the CLI at this library through
# `smm config` (which writes %APPDATA%\SMM\settings.json). The GUI does not read that file: it
# keeps stagingDir/sekiroDir in QSettings (HKCU\Software\SekiroModManager\SMM), so for screenshots
# either pick these paths on the Settings page or set the two registry values yourself.

param(
    # Scratch root for the demo library. Override it to keep several demo libraries side by side
    # (screenshots, a manual UI pass, an end-to-end run) without them deleting each other.
    [string]$Root = (Join-Path $env:TEMP 'smm-demo')
)

$ErrorActionPreference = 'Stop'

$root    = $Root
$staging = Join-Path $root 'staging'
$game    = Join-Path $root 'game'
$src     = Join-Path $root 'pkgs'
# cmake presets put the binary under build/<presetName>/src/cli; older trees used build/gui.
# Probe both, newest first, so this keeps working whichever preset was built last.
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$candidates = @(
    'build\msvc-x64-debug\src\cli\smm.exe',
    'build\msvc-x64-release\src\cli\smm.exe',
    'build\vs2022-x64\src\cli\Debug\smm.exe',
    'build\vs2022-x64\src\cli\Release\smm.exe',
    'build\gui\src\cli\smm.exe'
) | ForEach-Object { Join-Path $repoRoot $_ }

$smm = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $smm) {
    Write-Error "smm.exe not found. Looked in:`n  $($candidates -join "`n  ")`nBuild it first:  cmake --preset msvc-x64-debug ; cmake --build --preset build-debug --target smm"
}

# id, display name, priority (lower wins), and the asset paths the mod provides. The metadata
# fields exist so screenshots and reviews show a populated card instead of "Unknown - v1.0.0";
# `smm import` has no flags for them, so they are patched into .smm_mod.json below.
$mods = @(
    @{ Id = 'gameparam-rebalance-v3';  Name = 'GameParam Rebalance';   Priority = 60;
       Author = 'Ashina Labs'; Version = '3.2.1'; Category = 'gameplay_overhaul';
       Description = 'Retunes enemy, prosthetic and skill parameters for a harder late-game curve.';
       Tags = @('difficulty', 'balance');
       Files = @('param\gameparam\gameparam.parambnd.dcx') }
    @{ Id = 'boss-rebalance-v2';       Name = 'Boss Rebalance';        Priority = 40;
       Author = 'Ashina Labs'; Version = '2.0.4'; Category = 'gameplay_overhaul';
       Description = 'Reworks boss phase transitions and posture recovery windows.';
       Tags = @('boss', 'balance');
       Files = @('param\gameparam\gameparam.parambnd.dcx') }
    @{ Id = 'kusabimaru-reaper-v1-2';  Name = 'Kusabimaru Reaper';     Priority = 30;
       Author = 'Reaper Workshop'; Version = '1.2.0'; Category = 'weapon_skin';
       Description = 'Blackened blade and bone-wrapped hilt for the Kusabimaru.';
       Tags = @('katana', 'retexture');
       Files = @('chr\c5110.chrbnd.dcx', 'parts\wp_a_0300.partsbnd.dcx') }
    @{ Id = 'mortal-blade-glow-v2';    Name = 'Mortal Blade Glow';     Priority = 70;
       Author = 'Reaper Workshop'; Version = '2.0.0'; Category = 'weapon_skin';
       Description = 'Emissive crimson glow along the Mortal Blade edge.';
       Tags = @('katana', 'vfx');
       Files = @('parts\wp_a_0310.partsbnd.dcx') }
    @{ Id = 'dream-of-the-damned-v1';  Name = 'Dream Of The Damned';   Priority = 20;
       Author = 'Moonlit Studio'; Version = '1.1.3'; Category = 'weapon_skin';
       Description = 'Pale spectral finish for both katanas, with matching sheaths.';
       Tags = @('katana', 'retexture');
       Files = @('parts\wp_a_0300.partsbnd.dcx', 'parts\wp_a_0310.partsbnd.dcx') }
    @{ Id = 'chinese-ui-v1';           Name = 'Chinese UI';            Priority = 80;
       Author = 'Ink Brush'; Version = '1.0.6'; Category = 'ui';
       Description = 'Full Simplified Chinese localisation for menus, items and subtitles.';
       Tags = @('localisation');
       Files = @('menu\hi\01_common.tpf.dcx', 'menu\font\font_zh.gfx') }
    @{ Id = 'sekiro-ultrawide-fix-v1'; Name = 'Sekiro Ultrawide Fix';  Priority = 90;
       Author = 'Wide Ashina'; Version = '0.9.2'; Category = 'ui';
       Description = 'Corrects HUD anchoring and pillarboxing on 21:9 and 32:9 displays.';
       Tags = @('ultrawide', 'hud');
       Files = @('menu\hi\04_hud.tpf.dcx') }
)

Write-Host "Seeding a demo library in $root"
foreach ($dir in @($staging, (Join-Path $game 'mods'), $src)) {
    if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
}

Write-Host "`nCreating source packages..."
foreach ($mod in $mods) {
    $modDir = Join-Path $src $mod.Name
    foreach ($asset in $mod.Files) {
        $target = Join-Path $modDir $asset
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Set-Content -Path $target -Value "$($mod.Name) asset: $asset" -Encoding utf8
    }
    Set-Content -Path (Join-Path $modDir 'README.md') `
        -Value "$($mod.Name) - demo asset pack generated by scripts\local\seed_demo.ps1" -Encoding utf8
    Write-Host "  $($mod.Name): $($mod.Files.Count) asset(s)"
}

Write-Host "`nImporting..."
$failed = 0
foreach ($mod in $mods) {
    & $smm --quiet --staging $staging import (Join-Path $src $mod.Name) `
        --id $mod.Id --name $mod.Name --priority $mod.Priority --overwrite
    if ($LASTEXITCODE -ne 0) { Write-Warning "  import failed: $($mod.Name)"; $failed++ }
}

# `smm import` records author "Unknown" and version "1.0.0"; fill in the demo metadata here.
Write-Host "`nWriting mod metadata..."
foreach ($mod in $mods) {
    $metaPath = Join-Path $staging (Join-Path $mod.Id '.smm_mod.json')
    if (-not (Test-Path $metaPath)) { Write-Warning "  metadata missing: $($mod.Id)"; $failed++; continue }
    $meta = Get-Content $metaPath -Raw | ConvertFrom-Json
    $meta.author = $mod.Author
    $meta.version = $mod.Version
    $meta.category = $mod.Category
    $meta.description = $mod.Description
    $meta.tags = @($mod.Tags)
    # No BOM: the reader parses the file as strict JSON.
    [System.IO.File]::WriteAllText($metaPath, ($meta | ConvertTo-Json -Depth 5),
        (New-Object System.Text.UTF8Encoding($false)))
}

Write-Host "`nSetting priorities (lower number wins a shared file)..."
foreach ($mod in $mods) {
    & $smm --quiet --staging $staging priority $mod.Id $mod.Priority
    if ($LASTEXITCODE -ne 0) { Write-Warning "  priority failed: $($mod.Id)"; $failed++ }
}

# Two presets so the preset page has real content: a full kit and a skins-only kit.
Write-Host "`nSaving presets..."
$skinOnly = @('gameparam-rebalance-v3', 'boss-rebalance-v2', 'chinese-ui-v1', 'sekiro-ultrawide-fix-v1')
& $smm --quiet --staging $staging presets save 'Full Kit' `
    --description 'Everything enabled: overhauls, UI packs and weapon skins.'
if ($LASTEXITCODE -ne 0) { Write-Warning '  preset failed: Full Kit'; $failed++ }
foreach ($id in $skinOnly) { & $smm --quiet --staging $staging disable $id }
& $smm --quiet --staging $staging presets save 'Skins Only' `
    --description 'Weapon skins only - overhauls and UI packs switched off.'
if ($LASTEXITCODE -ne 0) { Write-Warning '  preset failed: Skins Only'; $failed++ }
foreach ($id in $skinOnly) { & $smm --quiet --staging $staging enable $id }

Write-Host "`nPointing the CLI at the demo library..."
& $smm config --staging $staging --game-dir $game
if ($LASTEXITCODE -ne 0) { Write-Error "could not save the configuration" }

Write-Host ''
& $smm --staging $staging list
Write-Host ''
& $smm --staging $staging conflicts

Write-Host ''
if ($failed -gt 0) {
    Write-Warning "$failed step(s) failed."
    exit 1
}
Write-Host 'Done. Launch the GUI with:  build\msvc-x64-debug\src\gui\smm_gui.exe (Qt bin on PATH)'
