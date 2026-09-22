use std::path::PathBuf;
use thiserror::Error;

#[derive(Debug, Error)]
pub enum SmmError {
    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),

    #[error("JSON serialization/deserialization error: {0}")]
    Json(#[from] serde_json::Error),

    #[error("Mod metadata file (mod.json) not found at {0}")]
    MetadataNotFound(PathBuf),

    #[error("Invalid mod metadata at {path}: {message}")]
    InvalidMetadata { path: PathBuf, message: String },

    #[error("Normalization failed for {path}: {message}")]
    NormalizationError { path: PathBuf, message: String },

    #[error("No valid Sekiro game assets found in {0}")]
    NoAssetsFound(PathBuf),

    #[error("Deploy plan generation error: {0}")]
    DeployError(String),

    #[error("Mod with ID '{0}' not found")]
    ModNotFound(String),

    #[error("Mod with ID '{0}' already exists in staging")]
    ModAlreadyExists(String),

    #[error("ZIP archive error: {0}")]
    Zip(#[from] zip::result::ZipError),

    #[error("7z archive error: {0}")]
    SevenZ(String),

    #[error("Archive extraction error: {0}")]
    ExtractionError(String),

    #[error("Unsupported archive format: {0}")]
    UnsupportedArchive(PathBuf),

    #[error("Environment error: {0}")]
    EnvironmentError(String),

    #[error("Export error: {0}")]
    ExportError(String),

    #[error("ModEngine source unavailable: {0}")]
    ModEngineSourceMissing(String),
}

impl From<sevenz_rust::Error> for SmmError {
    fn from(err: sevenz_rust::Error) -> Self {
        SmmError::SevenZ(err.to_string())
    }
}

pub type Result<T> = std::result::Result<T, SmmError>;
