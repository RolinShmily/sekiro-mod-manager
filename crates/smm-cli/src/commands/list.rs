//! `list` / `scan` command implementations.

use std::path::Path;

use colored::Colorize;
use comfy_table::{Attribute, Cell, Color};

use smm_core::{ConflictEngine, ConflictSeverity, ModLoader};

use crate::output::{new_table, status_cell};
use crate::resolve::resolve_staging_dir;

pub fn run_list(staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg, false)?;
    println!(
        "\n{} {}",
        "Scanning staging directory:".bold().cyan(),
        staging_path.display().to_string().bold()
    );

    let mods = ModLoader::scan_mods_directory(&staging_path)
        .map_err(|e| format!("Failed to read mods from {}: {}", staging_path.display(), e))?;

    if mods.is_empty() {
        println!(
            "{}",
            "No valid Sekiro mod packages (with mod.json) found."
                .yellow()
                .bold()
        );
        return Ok(());
    }

    let mut table = new_table(&[
        "Pri",
        "Mod ID",
        "Display Name",
        "Version",
        "Author",
        "Category",
        "Status",
        "Assets",
    ]);

    for (info, assets) in &mods {
        table.add_row(vec![
            Cell::new(info.priority.to_string()).fg(Color::Cyan),
            Cell::new(&info.id).add_attribute(Attribute::Bold),
            Cell::new(&info.name),
            Cell::new(&info.version),
            Cell::new(&info.author),
            Cell::new(&info.category).fg(Color::Blue),
            status_cell(info.enabled),
            Cell::new(assets.len().to_string()).fg(Color::Yellow),
        ]);
    }

    println!("{table}");
    println!(
        "{} {} mod(s) registered in staging.",
        "Total:".bold(),
        mods.len().to_string().green().bold()
    );
    Ok(())
}

pub fn run_scan(staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg, false)?;
    println!(
        "\n{} {}",
        "Performing deep asset scan in:".bold().cyan(),
        staging_path.display().to_string().bold()
    );

    let mods = ModLoader::scan_mods_directory(&staging_path)
        .map_err(|e| format!("Failed to read mods from {}: {}", staging_path.display(), e))?;

    let total_assets: usize = mods.iter().map(|(_, a)| a.len()).sum();
    let report = ConflictEngine::scan_conflicts(&mods);

    println!(
        "{} {} mods scanned, {} total normalized assets indexed.\n",
        "Summary:".bold(),
        mods.len().to_string().green().bold(),
        total_assets.to_string().cyan().bold()
    );

    if report.total_conflicts == 0 {
        println!(
            "{}",
            "✓ No conflicts detected across all active mods! Clean deployment guaranteed."
                .green()
                .bold()
        );
        return Ok(());
    }

    let mut table = new_table(&[
        "Severity",
        "Asset Path",
        "Winner (Priority)",
        "Shadowed Mod(s)",
        "Conflict Guidance",
    ]);

    for record in &report.records {
        let sev_cell = match record.severity {
            ConflictSeverity::Critical => Cell::new("CRITICAL")
                .fg(Color::Red)
                .add_attribute(Attribute::Bold),
            ConflictSeverity::Warning => Cell::new("WARNING")
                .fg(Color::Yellow)
                .add_attribute(Attribute::Bold),
            ConflictSeverity::Info => Cell::new("INFO").fg(Color::Cyan),
        };

        let winner_pri = mods
            .iter()
            .find(|(m, _)| m.id == record.winner_mod_id)
            .map(|(m, _)| m.priority)
            .unwrap_or(0);

        table.add_row(vec![
            sev_cell,
            Cell::new(&record.relative_path).add_attribute(Attribute::Bold),
            Cell::new(format!("{} (pri {})", record.winner_mod_id, winner_pri)).fg(Color::Green),
            Cell::new(record.shadowed_mod_ids.join(", ")).fg(Color::DarkGrey),
            Cell::new(&record.message),
        ]);
    }

    println!("{table}\n");

    if report.has_critical_conflict {
        println!("{}", "=".repeat(80).red().bold());
        println!("{}", " [!] CRITICAL PARAM CONFLICT DETECTED:".red().bold());
        println!(
            "{}",
            "     Colliding mods both modify 'gameparam.parambnd.dcx' or core game logic.".red()
        );
        println!(
            "{}",
            "     Only the winning mod's parameters will apply. Merge with DSMapStudio if needed."
                .red()
        );
        println!("{}", "=".repeat(80).red().bold());
    } else if report.has_warning_conflict {
        println!(
            "{}",
            "[!] Warning: Exclusive weapon/character mesh slot collisions detected."
                .yellow()
                .bold()
        );
        println!(
            "{}",
            "    Only the winning mod's model/texture will display in-game.".yellow()
        );
    }

    println!(
        "\n{} Total colliding asset paths: {}",
        "Collision Count:".bold(),
        report.total_conflicts.to_string().yellow().bold()
    );

    Ok(())
}
