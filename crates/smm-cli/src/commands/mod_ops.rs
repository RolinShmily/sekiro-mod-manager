//! `enable` / `disable` / `priority` / `info` / `remove` command implementations.

use std::io::Write;
use std::path::Path;

use colored::Colorize;
use comfy_table::{Attribute, Cell, Color};

use smm_core::{delete_mod, find_mod_dir, get_mod_details, set_mod_enabled, set_mod_priority};

use crate::output::{format_bytes, kv_table};
use crate::resolve::resolve_staging_dir;

pub fn run_enable(mod_id: &str, staging_arg: Option<&Path>) -> Result<(), String> {
    set_mod_state(mod_id, staging_arg, true)?;
    println!(
        "{} Mod '{}' is now {}.",
        "✓".green().bold(),
        mod_id.bold(),
        "ENABLED".green().bold()
    );
    Ok(())
}

pub fn run_disable(mod_id: &str, staging_arg: Option<&Path>) -> Result<(), String> {
    set_mod_state(mod_id, staging_arg, false)?;
    println!(
        "{} Mod '{}' is now {}.",
        "✓".yellow().bold(),
        mod_id.bold(),
        "DISABLED".dimmed().bold()
    );
    Ok(())
}

fn set_mod_state(mod_id: &str, staging_arg: Option<&Path>, enabled: bool) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg, false)?;
    set_mod_enabled(&staging_path, mod_id, enabled)
        .map_err(|e| format!("Failed to set mod '{}' state: {}", mod_id, e))?;
    Ok(())
}

pub fn run_priority(mod_id: &str, priority: u32, staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg, false)?;
    let info = set_mod_priority(&staging_path, mod_id, priority)
        .map_err(|e| format!("Failed to update priority for mod '{}': {}", mod_id, e))?;

    println!(
        "{} Mod '{}' ({}) priority updated to {}.",
        "✓".green().bold(),
        info.id.bold(),
        info.name,
        info.priority.to_string().cyan().bold()
    );
    Ok(())
}

pub fn run_info(mod_id: &str, staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg, false)?;
    let (info, assets) = get_mod_details(&staging_path, mod_id)
        .map_err(|e| format!("Failed to retrieve mod info for '{}': {}", mod_id, e))?;

    let total_size: u64 = assets.iter().map(|a| a.file_size).sum();

    println!(
        "\n{} {}",
        "Mod Information:".bold().cyan(),
        info.name.bold()
    );

    let mut meta_table = kv_table();
    meta_table.add_row(vec![
        Cell::new("Mod ID"),
        Cell::new(&info.id).fg(Color::Cyan),
    ]);
    meta_table.add_row(vec![Cell::new("Display Name"), Cell::new(&info.name)]);
    meta_table.add_row(vec![Cell::new("Version"), Cell::new(&info.version)]);
    meta_table.add_row(vec![Cell::new("Author"), Cell::new(&info.author)]);
    meta_table.add_row(vec![
        Cell::new("Category"),
        Cell::new(&info.category).fg(Color::Blue),
    ]);
    meta_table.add_row(vec![
        Cell::new("Deployment Priority"),
        Cell::new(info.priority.to_string()).fg(Color::Yellow),
    ]);
    meta_table.add_row(vec![
        Cell::new("Status"),
        Cell::new(if info.enabled { "Enabled" } else { "Disabled" }).fg(if info.enabled {
            Color::Green
        } else {
            Color::DarkGrey
        }),
    ]);
    if let Some(desc) = &info.description {
        meta_table.add_row(vec![Cell::new("Description"), Cell::new(desc)]);
    }
    if let Some(lic) = &info.license {
        meta_table.add_row(vec![Cell::new("License"), Cell::new(lic)]);
    }
    if let Some(source) = info.source_url.as_ref().or(info.homepage.as_ref()) {
        meta_table.add_row(vec![Cell::new("Source URL"), Cell::new(source)]);
    }
    if !info.tags.is_empty() {
        meta_table.add_row(vec![Cell::new("Tags"), Cell::new(info.tags.join(", "))]);
    }
    if let Some(root) = &info.root_path {
        meta_table.add_row(vec![
            Cell::new("Root Path"),
            Cell::new(root.display().to_string()).fg(Color::Cyan),
        ]);
    }

    println!("{meta_table}\n");

    println!(
        "{} ({} files, {})",
        "Normalized Assets Hierarchy:".bold().cyan(),
        assets.len().to_string().yellow().bold(),
        format_bytes(total_size).bold()
    );

    let mut asset_table =
        crate::output::new_table(&["Subsystem", "Normalized Relative Path", "Size", "Flags"]);

    for asset in &assets {
        let flags = if asset.is_critical {
            Cell::new("CRITICAL PARAM")
                .fg(Color::Red)
                .add_attribute(Attribute::Bold)
        } else if asset.is_exclusive_slot {
            Cell::new("EXCLUSIVE SLOT")
                .fg(Color::Yellow)
                .add_attribute(Attribute::Bold)
        } else {
            Cell::new("-").fg(Color::DarkGrey)
        };

        asset_table.add_row(vec![
            Cell::new(format!("{:?}", asset.category)).fg(Color::Blue),
            Cell::new(&asset.relative_path),
            Cell::new(format_bytes(asset.file_size)),
            flags,
        ]);
    }

    println!("{asset_table}\n");
    Ok(())
}

pub fn run_remove(mod_id: &str, yes: bool, staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg, false)?;

    // Verify mod exists first
    let _ = find_mod_dir(&staging_path, mod_id)
        .map_err(|e| format!("Cannot remove mod '{}': {}", mod_id, e))?;

    if !yes {
        print!(
            "Are you sure you want to permanently delete mod '{}' from staging? [y/N]: ",
            mod_id
        );
        let _ = std::io::stdout().flush();
        let mut input = String::new();
        if std::io::stdin().read_line(&mut input).is_ok() {
            let trimmed = input.trim().to_ascii_lowercase();
            if trimmed != "y" && trimmed != "yes" {
                println!("{}", "Operation cancelled. Mod was not deleted.".yellow());
                return Ok(());
            }
        } else {
            return Err("Failed to read input from terminal.".into());
        }
    }

    delete_mod(&staging_path, mod_id)
        .map_err(|e| format!("Failed to delete mod '{}': {}", mod_id, e))?;

    println!(
        "{} Mod '{}' permanently removed from staging.",
        "✓".green().bold(),
        mod_id.bold()
    );
    Ok(())
}
