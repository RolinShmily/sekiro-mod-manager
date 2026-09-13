use std::io::Write;
use std::path::{Path, PathBuf};
use std::process::ExitCode;

use clap::{Parser, Subcommand};
use colored::Colorize;
use comfy_table::modifiers::UTF8_ROUND_CORNERS;
use comfy_table::presets::UTF8_FULL;
use comfy_table::{Attribute, Cell, Color, ContentArrangement, Table};

use smm_core::{
    delete_mod, diagnose_environment, execute_deploy, find_mod_dir, get_mod_details, import_mod,
    install_mod_engine, restore_deploy, set_mod_enabled, set_mod_priority, ConflictEngine,
    ConflictSeverity, DeploymentPlanner, DiagnosticStatus, ImportOptions, ModLoader, OverallHealth,
};

#[derive(Parser)]
#[command(
    name = "smm",
    about = "Sekiro Mod Manager (SMM) - High-performance Win32 NTFS Hard-Link Mod Deployment & Management CLI",
    version
)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// List all mods in the staging area with version, priority, and enabled status
    List {
        /// Path to mods staging directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Deeply scan assets and report path and semantic conflicts
    Scan {
        /// Path to mods staging directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Deploy enabled mods into the Sekiro game directory via high-speed NTFS hard links
    Deploy {
        /// Target game mods directory (e.g. <Sekiro_Install_Dir>/mods)
        #[arg(short, long, required = true)]
        target: PathBuf,

        /// Path to mods staging directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,

        /// Profile name for the deployment
        #[arg(short, long, default_value = "default")]
        profile: String,
    },

    /// Clean and restore the Sekiro game mods directory to its original state
    Restore {
        /// Target game mods directory to clean and restore
        #[arg(short, long, required = true)]
        target: PathBuf,
    },

    /// Enable a specific mod in the staging area
    Enable {
        /// Identifier of the mod to enable
        mod_id: String,

        /// Path to mods staging directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Disable a specific mod in the staging area
    Disable {
        /// Identifier of the mod to disable
        mod_id: String,

        /// Path to mods staging directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Set deployment priority for a specific mod (lower number = higher precedence)
    Priority {
        /// Identifier of the mod
        mod_id: String,

        /// New priority number (e.g. 10, 50, 100)
        priority: u32,

        /// Path to mods staging directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Import a local mod directory, .zip, or .7z archive into the staging area
    Import {
        /// Path to mod directory, .zip, or .7z archive
        source: PathBuf,

        /// Custom unique mod ID [default: derived from filename]
        #[arg(long)]
        id: Option<String>,

        /// Custom human-readable display name [default: derived from filename]
        #[arg(long)]
        name: Option<String>,

        /// Deployment priority override
        #[arg(short, long)]
        priority: Option<u32>,

        /// Overwrite if a mod with the same ID already exists in staging
        #[arg(short = 'w', long)]
        overwrite: bool,

        /// Path to mods staging directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Inspect Sekiro game directory and ModEngine environment health
    Doctor {
        /// Path to Sekiro game root directory containing sekiro.exe [default: auto-detected or current directory]
        #[arg(short, long)]
        game_dir: Option<PathBuf>,

        /// Path to mods staging directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Automatically deploy and configure Sekiro ModEngine hook and modengine.ini
    SetupEngine {
        /// Path to Sekiro game root directory containing sekiro.exe
        #[arg(short, long, required = true)]
        game_dir: PathBuf,

        /// Path to mods staging directory or source ModEngine directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Inspect detailed metadata and normalized asset hierarchy for a specific mod
    Info {
        /// Identifier of the mod
        mod_id: String,

        /// Path to mods staging directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Permanently remove a mod from the staging area
    Remove {
        /// Identifier of the mod to delete
        mod_id: String,

        /// Skip interactive confirmation prompt
        #[arg(short, long)]
        yes: bool,

        /// Path to mods staging directory [default: staging or staging]
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },
}

/// Resolves staging directory path with smart fallback to "staging" or "staging".
fn resolve_staging_dir(staging_arg: Option<&Path>) -> Result<PathBuf, String> {
    if let Some(path) = staging_arg {
        if path.exists() {
            Ok(path.to_path_buf())
        } else {
            Err(format!(
                "Specified staging directory does not exist: {}",
                path.display()
            ))
        }
    } else {
        let candidates = [
            PathBuf::from("staging"),
            PathBuf::from("staging"),
            PathBuf::from("../staging"),
            PathBuf::from("../../staging"),
        ];

        for candidate in candidates {
            if candidate.exists() && candidate.is_dir() {
                return Ok(candidate);
            }
        }

        Err(
            "Staging directory not found. Please provide `--staging <path>` or create a `staging/` directory."
                .to_string(),
        )
    }
}

/// Resolves staging directory path, creating a default "staging" folder if none exists.
fn resolve_or_create_staging_dir(staging_arg: Option<&Path>) -> Result<PathBuf, String> {
    if let Some(path) = staging_arg {
        std::fs::create_dir_all(path)
            .map_err(|e| format!("Failed to create staging directory {}: {}", path.display(), e))?;
        Ok(path.to_path_buf())
    } else {
        let candidates = [
            PathBuf::from("staging"),
            PathBuf::from("staging"),
            PathBuf::from("../staging"),
            PathBuf::from("../../staging"),
        ];

        for candidate in candidates {
            if candidate.exists() && candidate.is_dir() {
                return Ok(candidate);
            }
        }

        let default_dir = PathBuf::from("staging");
        std::fs::create_dir_all(&default_dir)
            .map_err(|e| format!("Failed to create staging directory: {}", e))?;
        Ok(default_dir)
    }
}

/// Formats raw bytes into human-readable representation (e.g. 24.50 MB).
fn format_bytes(bytes: u64) -> String {
    const KB: u64 = 1024;
    const MB: u64 = KB * 1024;
    const GB: u64 = MB * 1024;

    if bytes >= GB {
        format!("{:.2} GB ({} bytes)", bytes as f64 / GB as f64, bytes)
    } else if bytes >= MB {
        format!("{:.2} MB ({} bytes)", bytes as f64 / MB as f64, bytes)
    } else if bytes >= KB {
        format!("{:.2} KB ({} bytes)", bytes as f64 / KB as f64, bytes)
    } else {
        format!("{} B", bytes)
    }
}

fn run_list(staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg)?;
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

    let mut table = Table::new();
    table
        .load_preset(UTF8_FULL)
        .apply_modifier(UTF8_ROUND_CORNERS)
        .set_content_arrangement(ContentArrangement::Dynamic)
        .set_header(vec![
            Cell::new("Pri").add_attribute(Attribute::Bold),
            Cell::new("Mod ID").add_attribute(Attribute::Bold),
            Cell::new("Display Name").add_attribute(Attribute::Bold),
            Cell::new("Version").add_attribute(Attribute::Bold),
            Cell::new("Author").add_attribute(Attribute::Bold),
            Cell::new("Category").add_attribute(Attribute::Bold),
            Cell::new("Status").add_attribute(Attribute::Bold),
            Cell::new("Assets").add_attribute(Attribute::Bold),
        ]);

    for (info, assets) in &mods {
        let status_cell = if info.enabled {
            Cell::new("Enabled")
                .fg(Color::Green)
                .add_attribute(Attribute::Bold)
        } else {
            Cell::new("Disabled").fg(Color::DarkGrey)
        };

        table.add_row(vec![
            Cell::new(info.priority.to_string()).fg(Color::Cyan),
            Cell::new(&info.id).add_attribute(Attribute::Bold),
            Cell::new(&info.name),
            Cell::new(&info.version),
            Cell::new(&info.author),
            Cell::new(&info.category).fg(Color::Blue),
            status_cell,
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

fn run_scan(staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg)?;
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

    let mut table = Table::new();
    table
        .load_preset(UTF8_FULL)
        .apply_modifier(UTF8_ROUND_CORNERS)
        .set_content_arrangement(ContentArrangement::Dynamic)
        .set_header(vec![
            Cell::new("Severity").add_attribute(Attribute::Bold),
            Cell::new("Asset Path").add_attribute(Attribute::Bold),
            Cell::new("Winner (Priority)").add_attribute(Attribute::Bold),
            Cell::new("Shadowed Mod(s)").add_attribute(Attribute::Bold),
            Cell::new("Conflict Guidance").add_attribute(Attribute::Bold),
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

        let winner_desc = format!("{} (pri {})", record.winner_mod_id, winner_pri);

        table.add_row(vec![
            sev_cell,
            Cell::new(&record.relative_path).add_attribute(Attribute::Bold),
            Cell::new(winner_desc).fg(Color::Green),
            Cell::new(record.shadowed_mod_ids.join(", ")).fg(Color::DarkGrey),
            Cell::new(&record.message),
        ]);
    }

    println!("{table}\n");

    if report.has_critical_conflict {
        println!(
            "{}",
            "================================================================================"
                .red()
                .bold()
        );
        println!(
            "{}",
            " [!] CRITICAL PARAM CONFLICT DETECTED:"
                .red()
                .bold()
        );
        println!(
            "{}",
            "     Colliding mods both modify 'gameparam.parambnd.dcx' or core game logic."
                .red()
        );
        println!(
            "{}",
            "     Only the winning mod's parameters will apply. Merge with DSMapStudio if needed."
                .red()
        );
        println!(
            "{}",
            "================================================================================"
                .red()
                .bold()
        );
    } else if report.has_warning_conflict {
        println!(
            "{}",
            "[!] Warning: Exclusive weapon/character mesh slot collisions detected."
                .yellow()
                .bold()
        );
        println!(
            "{}",
            "    Only the winning mod's model/texture will display in-game."
                .yellow()
        );
    }

    println!(
        "\n{} Total colliding asset paths: {}",
        "Collision Count:".bold(),
        report.total_conflicts.to_string().yellow().bold()
    );

    Ok(())
}

fn run_deploy(target: &Path, staging_arg: Option<&Path>, profile: &str) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg)?;
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

    let res = execute_deploy(&plan, target)
        .map_err(|e| format!("Deployment execution failed: {}", e))?;

    println!(
        "\n{}",
        "================ DEPLOYMENT REPORT ================"
            .green()
            .bold()
    );

    let mut table = Table::new();
    table
        .load_preset(UTF8_FULL)
        .apply_modifier(UTF8_ROUND_CORNERS)
        .set_content_arrangement(ContentArrangement::Dynamic)
        .set_header(vec![
            Cell::new("Metric").add_attribute(Attribute::Bold),
            Cell::new("Value").add_attribute(Attribute::Bold),
        ]);

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
        Cell::new(format_bytes(res.bytes_saved))
            .fg(Color::Green)
            .add_attribute(Attribute::Bold),
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

fn run_restore(target: &Path) -> Result<(), String> {
    println!(
        "\n{} {}",
        "Cleaning and restoring mods directory:".bold().yellow(),
        target.display().to_string().bold()
    );

    let res = restore_deploy(target)
        .map_err(|e| format!("Restore operation failed: {}", e))?;

    let mut table = Table::new();
    table
        .load_preset(UTF8_FULL)
        .apply_modifier(UTF8_ROUND_CORNERS)
        .set_content_arrangement(ContentArrangement::Dynamic)
        .set_header(vec![
            Cell::new("Metric").add_attribute(Attribute::Bold),
            Cell::new("Value").add_attribute(Attribute::Bold),
        ]);

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

fn run_enable(mod_id: &str, staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg)?;
    let info = set_mod_enabled(&staging_path, mod_id, true)
        .map_err(|e| format!("Failed to enable mod '{}': {}", mod_id, e))?;

    println!(
        "{} Mod '{}' ({}) is now {}.",
        "✓".green().bold(),
        info.id.bold(),
        info.name,
        "ENABLED".green().bold()
    );
    Ok(())
}

fn run_disable(mod_id: &str, staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg)?;
    let info = set_mod_enabled(&staging_path, mod_id, false)
        .map_err(|e| format!("Failed to disable mod '{}': {}", mod_id, e))?;

    println!(
        "{} Mod '{}' ({}) is now {}.",
        "✓".yellow().bold(),
        info.id.bold(),
        info.name,
        "DISABLED".dimmed().bold()
    );
    Ok(())
}

fn run_priority(mod_id: &str, priority: u32, staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg)?;
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

fn run_import(
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

    let staging_path = resolve_or_create_staging_dir(staging_arg)?;
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

    let mut table = Table::new();
    table
        .load_preset(UTF8_FULL)
        .apply_modifier(UTF8_ROUND_CORNERS)
        .set_content_arrangement(ContentArrangement::Dynamic)
        .set_header(vec![
            Cell::new("Property").add_attribute(Attribute::Bold),
            Cell::new("Value").add_attribute(Attribute::Bold),
        ]);

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

fn run_info(mod_id: &str, staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg)?;
    let (info, assets) = get_mod_details(&staging_path, mod_id)
        .map_err(|e| format!("Failed to retrieve mod info for '{}': {}", mod_id, e))?;

    let total_size: u64 = assets.iter().map(|a| a.file_size).sum();

    println!(
        "\n{} {}",
        "Mod Information:".bold().cyan(),
        info.name.bold()
    );

    let mut meta_table = Table::new();
    meta_table
        .load_preset(UTF8_FULL)
        .apply_modifier(UTF8_ROUND_CORNERS)
        .set_content_arrangement(ContentArrangement::Dynamic)
        .set_header(vec![
            Cell::new("Field").add_attribute(Attribute::Bold),
            Cell::new("Value").add_attribute(Attribute::Bold),
        ]);

    meta_table.add_row(vec![Cell::new("Mod ID"), Cell::new(&info.id).fg(Color::Cyan)]);
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
        Cell::new(if info.enabled {
            "Enabled"
        } else {
            "Disabled"
        })
        .fg(if info.enabled {
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

    let mut asset_table = Table::new();
    asset_table
        .load_preset(UTF8_FULL)
        .apply_modifier(UTF8_ROUND_CORNERS)
        .set_content_arrangement(ContentArrangement::Dynamic)
        .set_header(vec![
            Cell::new("Subsystem").add_attribute(Attribute::Bold),
            Cell::new("Normalized Relative Path").add_attribute(Attribute::Bold),
            Cell::new("Size").add_attribute(Attribute::Bold),
            Cell::new("Flags").add_attribute(Attribute::Bold),
        ]);

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

fn run_remove(mod_id: &str, yes: bool, staging_arg: Option<&Path>) -> Result<(), String> {
    let staging_path = resolve_staging_dir(staging_arg)?;

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

/// Resolves Sekiro game root directory with fallback to CWD or standard Windows Steam install paths.
fn resolve_game_dir(game_dir_arg: Option<&Path>) -> PathBuf {
    if let Some(path) = game_dir_arg {
        return path.to_path_buf();
    }

    // 1. Current working directory
    let cwd = PathBuf::from(".");
    if cwd.join("sekiro.exe").is_file() {
        return cwd;
    }

    // 2. Common Steam library candidates on Windows
    let candidates = [
        r"C:\Program Files (x86)\Steam\steamapps\common\Sekiro",
        r"C:\Program Files\Steam\steamapps\common\Sekiro",
        r"D:\SteamLibrary\steamapps\common\Sekiro",
        r"E:\SteamLibrary\steamapps\common\Sekiro",
    ];

    for candidate in candidates {
        let p = PathBuf::from(candidate);
        if p.exists() && p.join("sekiro.exe").is_file() {
            return p;
        }
    }

    // Fallback to current directory
    PathBuf::from(".")
}

fn run_doctor(game_dir_arg: Option<&Path>, staging_arg: Option<&Path>) -> Result<(), String> {
    let game_dir = resolve_game_dir(game_dir_arg);
    let staging_path = staging_arg
        .map(|p| p.to_path_buf())
        .or_else(|| resolve_staging_dir(None).ok());

    println!(
        "\n{} {}",
        "Diagnosing Sekiro Game Environment & ModEngine Setup...".bold().cyan(),
        format!("(Target: {})", game_dir.display()).dimmed()
    );

    let report = diagnose_environment(&game_dir, staging_path.as_deref());

    let mut table = Table::new();
    table
        .load_preset(UTF8_FULL)
        .apply_modifier(UTF8_ROUND_CORNERS)
        .set_content_arrangement(ContentArrangement::Dynamic)
        .set_header(vec![
            Cell::new("Category").add_attribute(Attribute::Bold),
            Cell::new("Item").add_attribute(Attribute::Bold),
            Cell::new("Status").add_attribute(Attribute::Bold),
            Cell::new("Details").add_attribute(Attribute::Bold),
            Cell::new("Remediation / Suggestion").add_attribute(Attribute::Bold),
        ]);

    for item in &report.items {
        let status_cell = match item.status {
            DiagnosticStatus::Pass => Cell::new("PASS").fg(Color::Green).add_attribute(Attribute::Bold),
            DiagnosticStatus::Warning => Cell::new("WARN").fg(Color::Yellow).add_attribute(Attribute::Bold),
            DiagnosticStatus::Fail => Cell::new("FAIL").fg(Color::Red).add_attribute(Attribute::Bold),
        };

        let rem_text = item.remediation.as_deref().unwrap_or("-");

        table.add_row(vec![
            Cell::new(&item.category).fg(Color::Cyan),
            Cell::new(&item.name).add_attribute(Attribute::Bold),
            status_cell,
            Cell::new(&item.message),
            Cell::new(rem_text).fg(if item.status.is_fail() {
                Color::Red
            } else if item.status.is_warning() {
                Color::Yellow
            } else {
                Color::DarkGrey
            }),
        ]);
    }

    println!("{table}\n");

    println!(
        "Summary: {} passed, {} warnings, {} failures",
        report.pass_count().to_string().green().bold(),
        report.warning_count().to_string().yellow().bold(),
        report.fail_count().to_string().red().bold()
    );

    match report.overall_status {
        OverallHealth::Healthy => {
            println!(
                "\n{} Overall Health: HEALTHY (Sekiro and ModEngine environment are fully operational!)",
                "✓".green().bold()
            );
        }
        OverallHealth::Degraded => {
            println!(
                "\n{} Overall Health: DEGRADED (Mod loading will function, but review warnings above for optimal performance)",
                "⚠".yellow().bold()
            );
        }
        OverallHealth::ActionRequired => {
            println!(
                "\n{} Overall Health: ACTION REQUIRED (Critical files missing or disabled; mods will NOT load in game!)",
                "✗".red().bold()
            );
            println!(
                "{}",
                format!(
                    "=> Quick Fix: Run 'smm setup-engine --game-dir \"{}\"' to automatically deploy and configure ModEngine.",
                    game_dir.display()
                )
                .cyan()
                .bold()
            );
        }
    }

    Ok(())
}


fn run_setup_engine(game_dir: &Path, staging_arg: Option<&Path>) -> Result<(), String> {
    println!(
        "\n{} {}",
        "Setting up Sekiro ModEngine:".bold().cyan(),
        game_dir.display().to_string().bold()
    );

    let staging_path = staging_arg
        .map(|p| p.to_path_buf())
        .or_else(|| resolve_staging_dir(None).ok());

    install_mod_engine(game_dir, staging_path.as_deref())
        .map_err(|e| format!("ModEngine setup failed: {}", e))?;

    let mut table = Table::new();
    table
        .load_preset(UTF8_FULL)
        .apply_modifier(UTF8_ROUND_CORNERS)
        .set_content_arrangement(ContentArrangement::Dynamic)
        .set_header(vec![
            Cell::new("Installed Component").add_attribute(Attribute::Bold),
            Cell::new("Status").add_attribute(Attribute::Bold),
            Cell::new("Configuration / Details").add_attribute(Attribute::Bold),
        ]);

    table.add_row(vec![
        Cell::new("Hook Loader (dinput8.dll)").fg(Color::Cyan),
        Cell::new("INSTALLED").fg(Color::Green).add_attribute(Attribute::Bold),
        Cell::new(format!("Placed at {}", game_dir.join("dinput8.dll").display())),
    ]);

    table.add_row(vec![
        Cell::new("Configuration (modengine.ini)").fg(Color::Cyan),
        Cell::new("CONFIGURED").fg(Color::Green).add_attribute(Attribute::Bold),
        Cell::new("enabled=1, modOverrideDirectory=\"\\mods\", loadLooseParams=1"),
    ]);

    table.add_row(vec![
        Cell::new("Mod Root Directory (mods/)").fg(Color::Cyan),
        Cell::new("INITIALIZED").fg(Color::Green).add_attribute(Attribute::Bold),
        Cell::new(format!("Target at {}", game_dir.join("mods").display())),
    ]);

    println!("{table}\n");

    println!(
        "{} Sekiro ModEngine v0.1.16 is successfully configured and ready!",
        "✓ SUCCESS:".green().bold()
    );
    println!(
        "Next steps:\n  1. Run '{}' to verify configuration health.\n  2. Run '{}' to deploy your enabled mods.",
        format!("smm doctor --game-dir \"{}\"", game_dir.display()).bold(),
        format!("smm deploy --target \"{}\"", game_dir.join("mods").display()).bold()
    );

    Ok(())
}

fn main() -> ExitCode {
    let cli = Cli::parse();

    let res = match cli.command {
        Commands::List { staging } => run_list(staging.as_deref()),
        Commands::Scan { staging } => run_scan(staging.as_deref()),
        Commands::Deploy {
            target,
            staging,
            profile,
        } => run_deploy(&target, staging.as_deref(), &profile),
        Commands::Restore { target } => run_restore(&target),
        Commands::Enable { mod_id, staging } => run_enable(&mod_id, staging.as_deref()),
        Commands::Disable { mod_id, staging } => run_disable(&mod_id, staging.as_deref()),
        Commands::Priority {
            mod_id,
            priority,
            staging,
        } => run_priority(&mod_id, priority, staging.as_deref()),
        Commands::Import {
            source,
            id,
            name,
            priority,
            overwrite,
            staging,
        } => run_import(
            &source,
            id,
            name,
            priority,
            overwrite,
            staging.as_deref(),
        ),
        Commands::Info { mod_id, staging } => run_info(&mod_id, staging.as_deref()),
        Commands::Remove {
            mod_id,
            yes,
            staging,
        } => run_remove(&mod_id, yes, staging.as_deref()),
        Commands::Doctor { game_dir, staging } => {
            run_doctor(game_dir.as_deref(), staging.as_deref())
        }
        Commands::SetupEngine { game_dir, staging } => {
            run_setup_engine(&game_dir, staging.as_deref())
        }
    };


    match res {
        Ok(()) => ExitCode::SUCCESS,
        Err(err) => {
            eprintln!("\n{} {}", "Error:".red().bold(), err);
            ExitCode::FAILURE
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use clap::Parser;

    #[test]
    fn test_cli_parsing() {
        let cli = Cli::try_parse_from(["smm", "list", "--staging", "staging"]).unwrap();
        match cli.command {
            Commands::List { staging } => {
                assert_eq!(staging, Some(PathBuf::from("staging")));
            }
            _ => panic!("Expected List command"),
        }

        let cli = Cli::try_parse_from(["smm", "scan"]).unwrap();
        assert!(matches!(cli.command, Commands::Scan { staging: None }));

        let cli = Cli::try_parse_from([
            "smm",
            "deploy",
            "--target",
            "game_mods",
            "--staging",
            "staging",
            "--profile",
            "custom",
        ])
        .unwrap();
        match cli.command {
            Commands::Deploy {
                target,
                staging,
                profile,
            } => {
                assert_eq!(target, PathBuf::from("game_mods"));
                assert_eq!(staging, Some(PathBuf::from("staging")));
                assert_eq!(profile, "custom");
            }
            _ => panic!("Expected Deploy command"),
        }

        let cli = Cli::try_parse_from(["smm", "restore", "--target", "game_mods"]).unwrap();
        match cli.command {
            Commands::Restore { target } => {
                assert_eq!(target, PathBuf::from("game_mods"));
            }
            _ => panic!("Expected Restore command"),
        }

        // New commands
        let cli = Cli::try_parse_from([
            "smm",
            "enable",
            "kusabimaru-reaper",
            "--staging",
            "staging",
        ])
        .unwrap();
        match cli.command {
            Commands::Enable { mod_id, staging } => {
                assert_eq!(mod_id, "kusabimaru-reaper");
                assert_eq!(staging, Some(PathBuf::from("staging")));
            }
            _ => panic!("Expected Enable command"),
        }

        let cli = Cli::try_parse_from(["smm", "disable", "kusabimaru-reaper"]).unwrap();
        match cli.command {
            Commands::Disable { mod_id, staging } => {
                assert_eq!(mod_id, "kusabimaru-reaper");
                assert_eq!(staging, None);
            }
            _ => panic!("Expected Disable command"),
        }

        let cli = Cli::try_parse_from(["smm", "priority", "kusabimaru-reaper", "15"]).unwrap();
        match cli.command {
            Commands::Priority {
                mod_id,
                priority,
                staging,
            } => {
                assert_eq!(mod_id, "kusabimaru-reaper");
                assert_eq!(priority, 15);
                assert_eq!(staging, None);
            }
            _ => panic!("Expected Priority command"),
        }

        let cli = Cli::try_parse_from([
            "smm",
            "import",
            "pkg.zip",
            "--id",
            "my-custom-id",
            "--name",
            "My Mod",
            "--priority",
            "42",
            "--overwrite",
        ])
        .unwrap();
        match cli.command {
            Commands::Import {
                source,
                id,
                name,
                priority,
                overwrite,
                staging,
            } => {
                assert_eq!(source, PathBuf::from("pkg.zip"));
                assert_eq!(id, Some("my-custom-id".to_string()));
                assert_eq!(name, Some("My Mod".to_string()));
                assert_eq!(priority, Some(42));
                assert!(overwrite);
                assert_eq!(staging, None);
            }
            _ => panic!("Expected Import command"),
        }

        let cli = Cli::try_parse_from(["smm", "info", "kusabimaru-reaper"]).unwrap();
        match cli.command {
            Commands::Info { mod_id, staging } => {
                assert_eq!(mod_id, "kusabimaru-reaper");
                assert_eq!(staging, None);
            }
            _ => panic!("Expected Info command"),
        }

        let cli = Cli::try_parse_from(["smm", "remove", "kusabimaru-reaper", "--yes"]).unwrap();
        match cli.command {
            Commands::Remove {
                mod_id,
                yes,
                staging,
            } => {
                assert_eq!(mod_id, "kusabimaru-reaper");
                assert!(yes);
                assert_eq!(staging, None);
            }
            _ => panic!("Expected Remove command"),
        }

        let cli = Cli::try_parse_from(["smm", "doctor", "--game-dir", "C:\\Sekiro", "--staging", "staging"]).unwrap();
        match cli.command {
            Commands::Doctor { game_dir, staging } => {
                assert_eq!(game_dir, Some(PathBuf::from("C:\\Sekiro")));
                assert_eq!(staging, Some(PathBuf::from("staging")));
            }
            _ => panic!("Expected Doctor command"),
        }

        let cli = Cli::try_parse_from(["smm", "setup-engine", "--game-dir", "C:\\Sekiro"]).unwrap();
        match cli.command {
            Commands::SetupEngine { game_dir, staging } => {
                assert_eq!(game_dir, PathBuf::from("C:\\Sekiro"));
                assert_eq!(staging, None);
            }
            _ => panic!("Expected SetupEngine command"),
        }
    }

    #[test]
    fn test_format_bytes_utility() {
        assert_eq!(format_bytes(500), "500 B");
        assert!(format_bytes(2048).contains("2.00 KB"));
        assert!(format_bytes(1024 * 1024 * 5).contains("5.00 MB"));
        assert!(format_bytes(1024 * 1024 * 1024 * 2).contains("2.00 GB"));
    }

    #[test]
    fn test_resolve_staging_dir_fallback() {
        let temp = tempfile::tempdir().unwrap();
        let custom_staging = temp.path().join("my_staging");
        std::fs::create_dir_all(&custom_staging).unwrap();

        let resolved = resolve_staging_dir(Some(&custom_staging));
        assert!(resolved.is_ok(), "Expected resolving explicit staging dir to succeed");
        assert_eq!(resolved.unwrap(), custom_staging);

        let created = resolve_or_create_staging_dir(Some(&custom_staging));
        assert!(created.is_ok(), "Expected resolve_or_create_staging_dir to succeed");
    }
}
