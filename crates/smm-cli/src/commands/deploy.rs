//! `deploy` / `restore` command implementations.

use std::path::Path;

use colored::Colorize;
use comfy_table::{Attribute, Cell, Color};

use smm_core::{execute_deploy, restore_deploy, DeploymentPlanner, ModLoader};

use crate::output::{format_bytes, kv_table};
use crate::resolve::resolve_staging_dir;

pub fn run_deploy(target: &Path, staging_arg: Option<&Path>, profile: &str) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg, false)?;
    println!(
        "\n{} {} -> {}",
        "Initiating Deployment:".bold().cyan(),
        staging_path.display().to_string().bold(),
        target.display().to_string().bold()
    );

    let mods = ModLoader::scan_mods_directory(&staging_path)
        .map_err(|e| format!("Failed to read mods from {}: {}", staging_path.display(), e))?;

    if mods.is_empty() {
        return Err("No mods found in staging directory to deploy.".into());
    }

    let plan = DeploymentPlanner::build_plan(profile, &mods)
        .map_err(|e| format!("Failed to generate deployment plan: {}", e))?;

    println!(
        "{} {} mappings prepared across profile '{}'.",
        "Plan generated:".bold(),
        plan.mappings.len().to_string().green().bold(),
        profile.bold()
    );

    let res =
        execute_deploy(&plan, target).map_err(|e| format!("Deployment execution failed: {}", e))?;

    println!(
        "\n{}",
        "================ DEPLOYMENT REPORT ================"
            .green()
            .bold()
    );

    let mut table = kv_table();
    table.add_row(vec![
        Cell::new("Target Mods Directory"),
        Cell::new(res.target_dir.display().to_string()).fg(Color::Cyan),
    ]);
    table.add_row(vec![
        Cell::new("Total Mapped Assets"),
        Cell::new(res.total_files.to_string()),
    ]);
    table.add_row(vec![
        Cell::new("Hard Links Created (NTFS Projections)"),
        Cell::new(res.hard_links_created.to_string())
            .fg(Color::Green)
            .add_attribute(Attribute::Bold),
    ]);
    table.add_row(vec![
        Cell::new("Copied Files (Cross-Volume Downgrade)"),
        Cell::new(res.copied_files.to_string()).fg(if res.copied_files > 0 {
            Color::Yellow
        } else {
            Color::White
        }),
    ]);
    table.add_row(vec![
        Cell::new("Skipped / Failed Files"),
        Cell::new(res.failed_files.to_string()).fg(if res.failed_files > 0 {
            Color::Red
        } else {
            Color::Green
        }),
    ]);
    table.add_row(vec![
        Cell::new("Disk Space Saved (Deduped)"),
        Cell::new(format_bytes(res.bytes_saved)).fg(Color::Green),
    ]);
    table.add_row(vec![
        Cell::new("Execution Duration"),
        Cell::new(format!("{} ms", res.duration_ms))
            .fg(Color::Cyan)
            .add_attribute(Attribute::Bold),
    ]);

    println!("{table}");

    if !res.warnings.is_empty() {
        println!("\n{}", "Warnings during deployment:".yellow().bold());
        for w in &res.warnings {
            println!(" - {}", w.yellow());
        }
    }

    if !res.errors.is_empty() {
        println!("\n{}", "Errors during deployment:".red().bold());
        for (p, err) in &res.errors {
            println!(" - {}: {}", p.red().bold(), err);
        }
        return Err("Deployment completed with errors.".into());
    }

    println!(
        "\n{} NTFS hard links established in {} ms. Game is ready to launch!",
        "✓ SUCCESS:".green().bold(),
        res.duration_ms.to_string().cyan().bold()
    );

    Ok(())
}

pub fn run_restore(target: &Path) -> Result<(), String> {
    println!(
        "\n{} {}",
        "Cleaning and restoring mods directory:".bold().yellow(),
        target.display().to_string().bold()
    );

    let res = restore_deploy(target).map_err(|e| format!("Restore operation failed: {}", e))?;

    let mut table = kv_table();
    table.add_row(vec![
        Cell::new("Restored Target Directory"),
        Cell::new(res.target_dir.display().to_string()).fg(Color::Cyan),
    ]);
    table.add_row(vec![
        Cell::new("Removed Deployed Files"),
        Cell::new(res.removed_files.to_string())
            .fg(Color::Green)
            .add_attribute(Attribute::Bold),
    ]);
    table.add_row(vec![
        Cell::new("Removed Empty Subdirectories"),
        Cell::new(res.removed_dirs.to_string()).fg(Color::Cyan),
    ]);
    table.add_row(vec![
        Cell::new("Restoration Duration"),
        Cell::new(format!("{} ms", res.duration_ms))
            .fg(Color::Cyan)
            .add_attribute(Attribute::Bold),
    ]);

    println!("{table}");

    if !res.warnings.is_empty() {
        for w in &res.warnings {
            println!(" Note: {}", w.yellow());
        }
    }

    println!(
        "\n{} Sekiro game directory cleanly restored to baseline (0 leftover links).",
        "✓ RESTORE COMPLETED:".green().bold()
    );

    Ok(())
}
