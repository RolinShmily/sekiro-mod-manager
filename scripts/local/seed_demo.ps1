# Seed a throwaway demo staging library and deployed manifest without requiring a CLI binary.
# Used by scripts/local/capture_screenshots.ps1 to photograph every screen against neutral fixtures.

param(
    [string]$Root = (Join-Path $env:TEMP 'smm-demo')
)

$ErrorActionPreference = 'Stop'

$staging = Join-Path $Root 'staging'
$game    = Join-Path $Root 'game'
$modsDir = Join-Path $game 'mods'
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

$mods = @(
    @{ Id = 'gameparam-rebalance-v3';  Name = 'GameParam Rebalance';   Priority = 60;
       Author = 'Ashina Labs'; Version = '3.2.1'; Category = 'gameplay_overhaul';
       Description = 'Retunes enemy, prosthetic and skill parameters for a harder late-game curve.';
       Tags = @('difficulty', 'balance');
       Files = @('param/gameparam/gameparam.parambnd.dcx') }
    @{ Id = 'boss-rebalance-v2';       Name = 'Boss Rebalance';        Priority = 40;
       Author = 'Ashina Labs'; Version = '2.0.4'; Category = 'gameplay_overhaul';
       Description = 'Reworks boss phase transitions and posture recovery windows.';
       Tags = @('boss', 'balance');
       Files = @('param/gameparam/gameparam.parambnd.dcx') }
    @{ Id = 'kusabimaru-reaper-v1-2';  Name = 'Kusabimaru Reaper';     Priority = 30;
       Author = 'Reaper Workshop'; Version = '1.2.0'; Category = 'weapon_skin';
       Description = 'Blackened blade and bone-wrapped hilt for the Kusabimaru.';
       Tags = @('katana', 'retexture');
       Files = @('chr/c5110.chrbnd.dcx', 'parts/wp_a_0300.partsbnd.dcx') }
    @{ Id = 'mortal-blade-glow-v2';    Name = 'Mortal Blade Glow';     Priority = 70;
       Author = 'Reaper Workshop'; Version = '2.0.0'; Category = 'weapon_skin';
       Description = 'Emissive crimson glow along the Mortal Blade edge.';
       Tags = @('katana', 'vfx');
       Files = @('parts/wp_a_0310.partsbnd.dcx') }
    @{ Id = 'dream-of-the-damned-v1';  Name = 'Dream Of The Damned';   Priority = 20;
       Author = 'Moonlit Studio'; Version = '1.1.3'; Category = 'weapon_skin';
       Description = 'Pale spectral finish for both katanas, with matching sheaths.';
       Tags = @('katana', 'retexture');
       Files = @('parts/wp_a_0300.partsbnd.dcx', 'parts/wp_a_0310.partsbnd.dcx') }
    @{ Id = 'chinese-ui-v1';           Name = 'Chinese UI';            Priority = 80;
       Author = 'Ink Brush'; Version = '1.0.6'; Category = 'ui';
       Description = 'Full Simplified Chinese localisation for menus, items and subtitles.';
       Tags = @('localisation');
       Files = @('menu/hi/01_common.tpf.dcx', 'menu/font/font_zh.gfx') }
    @{ Id = 'sekiro-ultrawide-fix-v1'; Name = 'Sekiro Ultrawide Fix';  Priority = 90;
       Author = 'Wide Ashina'; Version = '0.9.2'; Category = 'ui';
       Description = 'Corrects HUD anchoring and pillarboxing on 21:9 and 32:9 displays.';
       Tags = @('ultrawide', 'hud');
       Files = @('menu/hi/04_hud.tpf.dcx') }
)

Write-Host "Seeding demo library in $Root"
foreach ($dir in @($staging, $modsDir)) {
    if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
}

# Create stub sekiro.exe + modengine.ini so Launch Hub shows a healthy environment.
[System.IO.File]::WriteAllText((Join-Path $game 'sekiro.exe'), 'demo', $utf8NoBom)
[System.IO.File]::WriteAllText((Join-Path $game 'modengine.ini'), "[files]`nuseModOverrideDirectory=1`nmodOverrideDirectory=`"\mods`"`n", $utf8NoBom)

$deployedFiles = @{}
foreach ($mod in ($mods | Sort-Object { $_.Priority })) {
    $modDir = Join-Path $staging $mod.Id
    New-Item -ItemType Directory -Force -Path $modDir | Out-Null

    $meta = [ordered]@{
        id          = $mod.Id
        name        = $mod.Name
        version     = $mod.Version
        author      = $mod.Author
        category    = $mod.Category
        description = $mod.Description
        tags        = @($mod.Tags)
        enabled     = $true
        priority    = $mod.Priority
    }
    [System.IO.File]::WriteAllText(
        (Join-Path $modDir '.smm_mod.json'),
        ($meta | ConvertTo-Json -Depth 5),
        $utf8NoBom
    )

    foreach ($rel in $mod.Files) {
        $winRel = $rel -replace '/', '\'
        $srcFile = Join-Path $modDir $winRel
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $srcFile) | Out-Null
        [System.IO.File]::WriteAllText($srcFile, "$($mod.Name) asset: $rel", $utf8NoBom)

        if (-not $deployedFiles.ContainsKey($rel)) {
            $dstFile = Join-Path $modsDir $winRel
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $dstFile) | Out-Null
            Copy-Item $srcFile $dstFile -Force
            $deployedFiles[$rel] = $mod.Id
        }
    }
}

$presets = [ordered]@{
    presets = @(
        [ordered]@{
            id = 'full-kit'
            name = '全量套装'
            name_en = 'Full Kit'
            description = '包含玩法调整、武器外观与界面优化的完整配置。'
            description_en = 'Everything enabled: overhauls, UI packs and weapon skins.'
            mods = @($mods | Sort-Object { $_.Priority } | ForEach-Object { $_.Id })
        },
        [ordered]@{
            id = 'skins-only'
            name = '纯净外观'
            name_en = 'Skins Only'
            description = '仅启用武器与角色外观模组，保留原版战斗平衡。'
            description_en = 'Weapon skins only - overhauls and UI packs switched off.'
            mods = @('dream-of-the-damned-v1', 'kusabimaru-reaper-v1-2', 'mortal-blade-glow-v2')
        }
    )
}
[System.IO.File]::WriteAllText(
    (Join-Path $staging '.smm_presets.json'),
    ($presets | ConvertTo-Json -Depth 6),
    $utf8NoBom
)

$manifest = [ordered]@{
    version = 1
    files   = @($deployedFiles.Keys | Sort-Object)
}
[System.IO.File]::WriteAllText(
    (Join-Path $modsDir '.smm_manifest.json'),
    ($manifest | ConvertTo-Json -Depth 4),
    $utf8NoBom
)

Write-Host "Done. Launch the GUI with: --staging=`"$staging`" --game-dir=`"$game`""
