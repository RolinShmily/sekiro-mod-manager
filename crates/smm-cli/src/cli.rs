use std::path::PathBuf;

use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(
    name = "smm",
    about = "Sekiro Mod Manager (SMM) - High-performance Win32 NTFS Hard-Link Mod Deployment & Management CLI",
    version
)]
pub struct Cli {
    #[command(subcommand)]
    pub command: Commands,
}

#[derive(Subcommand)]
pub enum Commands {
    /// List all mods in the staging area with version, priority, and enabled status
    List {
        /// Path to mods staging directory (default: auto-detected ./staging)
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Deeply scan assets and report path and semantic conflicts
    Scan {
        /// Path to mods staging directory (default: auto-detected ./staging)
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Deploy enabled mods into the Sekiro game directory via high-speed NTFS hard links
    Deploy {
        /// Target game mods directory (e.g. <Sekiro_Install_Dir>/mods)
        #[arg(short, long, required = true)]
        target: PathBuf,

        /// Path to mods staging directory (default: auto-detected ./staging)
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

        /// Path to mods staging directory (default: auto-detected ./staging)
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Disable a specific mod in the staging area
    Disable {
        /// Identifier of the mod to disable
        mod_id: String,

        /// Path to mods staging directory (default: auto-detected ./staging)
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Set deployment priority for a specific mod (lower number = higher precedence)
    Priority {
        /// Identifier of the mod
        mod_id: String,

        /// New priority number (e.g. 10, 50, 100)
        priority: u32,

        /// Path to mods staging directory (default: auto-detected ./staging)
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Import a local mod directory, .zip, or .7z archive into the staging area
    Import {
        /// Path to mod directory, .zip, or .7z archive
        source: PathBuf,

        /// Custom unique mod ID (default: derived from filename)
        #[arg(long)]
        id: Option<String>,

        /// Custom human-readable display name (default: derived from filename)
        #[arg(long)]
        name: Option<String>,

        /// Deployment priority override
        #[arg(short, long)]
        priority: Option<u32>,

        /// Overwrite if a mod with the same ID already exists in staging
        #[arg(short = 'w', long)]
        overwrite: bool,

        /// Path to mods staging directory (default: auto-detected ./staging)
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Inspect Sekiro game directory and ModEngine environment health
    Doctor {
        /// Path to Sekiro game root directory containing sekiro.exe (default: auto-detected or current directory)
        #[arg(short, long)]
        game_dir: Option<PathBuf>,

        /// Path to mods staging directory (default: auto-detected ./staging)
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Deploy Sekiro ModEngine hook (dinput8.dll) and modengine.ini in the game directory.
    /// Requires a user-supplied ModEngine payload; SMM does not redistribute it.
    SetupEngine {
        /// Path to Sekiro game root directory containing sekiro.exe
        #[arg(short, long, required = true)]
        game_dir: PathBuf,

        /// Path to mods staging directory, or a folder containing ModEngine's own dinput8.dll
        /// (default: auto-detected ./staging). SMM ships no ModEngine payload of its own.
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },

    /// Inspect detailed metadata and normalized asset hierarchy for a specific mod
    Info {
        /// Identifier of the mod
        mod_id: String,

        /// Path to mods staging directory (default: auto-detected ./staging)
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

        /// Path to mods staging directory (default: auto-detected ./staging)
        #[arg(short, long)]
        staging: Option<PathBuf>,
    },
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cli_parsing_all_commands() {
        // Parameterized over every subcommand's canonical arguments.
        let cases: &[(&str, &[&str])] = &[
            ("list", &["list", "--staging", "staging"]),
            ("scan", &["scan"]),
            (
                "deploy",
                &["deploy", "--target", "game_mods", "--staging", "staging", "--profile", "custom"],
            ),
            ("restore", &["restore", "--target", "game_mods"]),
            ("enable", &["enable", "kusabimaru-reaper", "--staging", "staging"]),
            ("disable", &["disable", "kusabimaru-reaper"]),
            ("priority", &["priority", "kusabimaru-reaper", "15"]),
            (
                "import",
                &[
                    "import", "pkg.zip", "--id", "my-custom-id", "--name", "My Mod",
                    "--priority", "42", "--overwrite",
                ],
            ),
            ("info", &["info", "kusabimaru-reaper"]),
            ("remove", &["remove", "kusabimaru-reaper", "--yes"]),
            ("doctor", &["doctor", "--game-dir", "C:\\Sekiro", "--staging", "staging"]),
            ("setup-engine", &["setup-engine", "--game-dir", "C:\\Sekiro"]),
        ];

        for (name, args) in cases {
            let argv: Vec<&str> = std::iter::once("smm").chain(args.iter().copied()).collect();
            let cli = Cli::try_parse_from(&argv)
                .unwrap_or_else(|e| panic!("command '{name}' failed to parse: {e}"));
            assert!(
                matches!(cli.command, Commands::List { .. } | Commands::Scan { .. } | Commands::Deploy { .. } | Commands::Restore { .. } | Commands::Enable { .. } | Commands::Disable { .. } | Commands::Priority { .. } | Commands::Import { .. } | Commands::Info { .. } | Commands::Remove { .. } | Commands::Doctor { .. } | Commands::SetupEngine { .. }),
                "command '{name}' resolved to an unexpected variant"
            );
        }
    }

    #[test]
    fn test_cli_flags_are_hoisted_correctly() {
        let cli = Cli::try_parse_from([
            "smm", "deploy", "--target", "game_mods", "--staging", "staging", "--profile", "custom",
        ])
        .unwrap();
        match cli.command {
            Commands::Deploy { target, staging, profile } => {
                assert_eq!(target, PathBuf::from("game_mods"));
                assert_eq!(staging, Some(PathBuf::from("staging")));
                assert_eq!(profile, "custom");
            }
            _ => panic!("unexpected command"),
        }

        let cli = Cli::try_parse_from(["smm", "import", "pkg.zip", "--overwrite"]).unwrap();
        match cli.command {
            Commands::Import {
                source,
                overwrite,
                staging,
                ..
            } => {
                assert_eq!(source, PathBuf::from("pkg.zip"));
                assert!(overwrite);
                assert_eq!(staging, None);
            }
            _ => panic!("unexpected command"),
        }
    }
}