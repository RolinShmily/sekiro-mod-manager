//! `import` command implementation.

use std::path::{Path, PathBuf};

use colored::Colorize;
use comfy_table::{Cell, Color};

use smm_core::{import_mod, ImportOptions, ModLoader};

use crate::output::{format_bytes, kv_table};
use crate::resolve::resolve_staging_dir;

pub fn run_import(
    source: &Path,
    custom_id: Option<String>,
    custom_name: Option<String>,
    priority: Option<u32>,
    overwrite: bool,
    staging_arg: Option<&Path>,
) -> Result<(), String> {
    let clean_str = source.to_string_lossy();
    let trimmed = clean_str.trim().trim_matches('"').trim_matches('\'');
    let source_clean = PathBuf::from(trimmed);

    let staging_path = resolve_staging_dir(staging_arg, true)?;
    println!(
        "\n{} {}",
        "Importing Mod Package:".bold().cyan(),
        source_clean.display().to_string().bold()
    );

    let opts = ImportOptions {
        custom_id,
        custom_name,
        priority,
        overwrite,
        source_url: None,
    };

    let imported = import_mod(&source_clean, &staging_path, &opts)
        .map_err(|e| format!("Mod import failed: {}", e))?;

    let root_path = imported
        .root_path
        .as_ref()
        .map(|p| p.display().to_string())
        .unwrap_or_default();

    let (_, assets) = ModLoader::scan_mod(
        imported
            .root_path
            .as_ref()
            .unwrap_or(&staging_path.join(&imported.id)),
    )
    .map_err(|e| format!("Failed to inspect imported mod assets: {}", e))?;

    let total_size: u64 = assets.iter().map(|a| a.file_size).sum();

    println!(
        "{} Successfully imported '{}' into staging area!",
        "✓ SUCCESS:".green().bold(),
        imported.name.bold()
    );

    let mut table = kv_table();
    table.add_row(vec![
        Cell::new("Mod ID"),
        Cell::new(&imported.id).fg(Color::Cyan),
    ]);
    table.add_row(vec![Cell::new("Display Name"), Cell::new(&imported.name)]);
    table.add_row(vec![Cell::new("Version"), Cell::new(&imported.version)]);
    table.add_row(vec![
        Cell::new("Category"),
        Cell::new(&imported.category).fg(Color::Blue),
    ]);
    table.add_row(vec![
        Cell::new("Deployment Priority"),
        Cell::new(imported.priority.to_string()).fg(Color::Yellow),
    ]);
    table.add_row(vec![
        Cell::new("Status"),
        Cell::new(if imported.enabled {
            "Enabled"
        } else {
            "Disabled"
        })
        .fg(Color::Green),
    ]);
    table.add_row(vec![
        Cell::new("Normalized Assets"),
        Cell::new(assets.len().to_string()).fg(Color::Yellow),
    ]);
    table.add_row(vec![
        Cell::new("Total Size"),
        Cell::new(format_bytes(total_size)),
    ]);
    table.add_row(vec![
        Cell::new("Staged Root Path"),
        Cell::new(&root_path).fg(Color::Cyan),
    ]);

    println!("{table}\n");
    Ok(())
}
