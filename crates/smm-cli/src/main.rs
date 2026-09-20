mod cli;
mod commands;
mod output;
mod resolve;

use std::process::ExitCode;

use clap::Parser;
use colored::Colorize;

use cli::{Cli, Commands};

fn main() -> ExitCode {
    let cli = Cli::parse();

    let res = match cli.command {
        Commands::List { staging } => commands::run_list(staging.as_deref()),
        Commands::Scan { staging } => commands::run_scan(staging.as_deref()),
        Commands::Deploy {
            target,
            staging,
            profile,
        } => commands::run_deploy(&target, staging.as_deref(), &profile),
        Commands::Restore { target } => commands::run_restore(&target),
        Commands::Enable { mod_id, staging } => commands::run_enable(&mod_id, staging.as_deref()),
        Commands::Disable { mod_id, staging } => commands::run_disable(&mod_id, staging.as_deref()),
        Commands::Priority {
            mod_id,
            priority,
            staging,
        } => commands::run_priority(&mod_id, priority, staging.as_deref()),
        Commands::Import {
            source,
            id,
            name,
            priority,
            overwrite,
            staging,
        } => commands::run_import(
            &source,
            id,
            name,
            priority,
            overwrite,
            staging.as_deref(),
        ),
        Commands::Info { mod_id, staging } => commands::run_info(&mod_id, staging.as_deref()),
        Commands::Remove {
            mod_id,
            yes,
            staging,
        } => commands::run_remove(&mod_id, yes, staging.as_deref()),
        Commands::Doctor { game_dir, staging } => {
            commands::run_doctor(game_dir.as_deref(), staging.as_deref())
        }
        Commands::SetupEngine { game_dir, staging } => {
            commands::run_setup_engine(&game_dir, staging.as_deref())
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