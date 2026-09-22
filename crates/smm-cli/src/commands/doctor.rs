//! `doctor` / `setup-engine` command implementations.

use std::path::Path;

use colored::Colorize;
use comfy_table::{Attribute, Cell, Color};

use smm_core::{diagnose_environment, install_mod_engine, DiagnosticStatus, OverallHealth};

use crate::output::new_table;
use crate::resolve::{resolve_game_dir, resolve_staging_dir};

pub fn run_doctor(game_dir_arg: Option<&Path>, staging_arg: Option<&Path>) -> Result<(), String> {
    let game_dir = resolve_game_dir(game_dir_arg);
    let staging_path = staging_arg
        .map(|p| p.to_path_buf())
        .or_else(|| resolve_staging_dir(None, false).ok());

    println!(
        "\n{} {}",
        "Diagnosing Sekiro Game Environment & ModEngine Setup..."
            .bold()
            .cyan(),
        format!("(Target: {})", game_dir.display()).dimmed()
    );

    let report = diagnose_environment(&game_dir, staging_path.as_deref());

    let mut table = new_table(&[
        "Category",
        "Item",
        "Status",
        "Details",
        "Remediation / Suggestion",
    ]);

    for item in &report.items {
        let status_cell = match item.status {
            DiagnosticStatus::Pass => Cell::new("PASS")
                .fg(Color::Green)
                .add_attribute(Attribute::Bold),
            DiagnosticStatus::Warning => Cell::new("WARN")
                .fg(Color::Yellow)
                .add_attribute(Attribute::Bold),
            DiagnosticStatus::Fail => Cell::new("FAIL")
                .fg(Color::Red)
                .add_attribute(Attribute::Bold),
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

pub fn run_setup_engine(game_dir: &Path, staging_arg: Option<&Path>) -> Result<(), String> {
    println!(
        "\n{} {}",
        "Setting up Sekiro ModEngine:".bold().cyan(),
        game_dir.display().to_string().bold()
    );

    let staging_path = staging_arg
        .map(|p| p.to_path_buf())
        .or_else(|| resolve_staging_dir(None, false).ok());

    install_mod_engine(game_dir, staging_path.as_deref())
        .map_err(|e| format!("ModEngine setup failed: {}", e))?;

    let mut table = new_table(&["Installed Component", "Status", "Configuration / Details"]);

    table.add_row(vec![
        Cell::new("Hook Loader (dinput8.dll)").fg(Color::Cyan),
        Cell::new("INSTALLED")
            .fg(Color::Green)
            .add_attribute(Attribute::Bold),
        Cell::new(format!(
            "Placed at {}",
            game_dir.join("dinput8.dll").display()
        )),
    ]);

    table.add_row(vec![
        Cell::new("Configuration (modengine.ini)").fg(Color::Cyan),
        Cell::new("CONFIGURED")
            .fg(Color::Green)
            .add_attribute(Attribute::Bold),
        Cell::new("enabled=1, modOverrideDirectory=\"\\mods\", loadLooseParams=1"),
    ]);

    table.add_row(vec![
        Cell::new("Mod Root Directory (mods/)").fg(Color::Cyan),
        Cell::new("INITIALIZED")
            .fg(Color::Green)
            .add_attribute(Attribute::Bold),
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
