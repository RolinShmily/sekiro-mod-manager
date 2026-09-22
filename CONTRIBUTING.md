# Contributing to SMM

Thanks for taking the time to contribute. This document covers everything you need to build, test,
and submit a change.

By participating you agree to the [Code of Conduct](CODE_OF_CONDUCT.md). Security issues should be
reported privately per [SECURITY.md](SECURITY.md) — not as a public issue.

---

## Prerequisites

| Tool | Version | Notes |
| :--- | :--- | :--- |
| [Rust](https://www.rust-lang.org/tools/install) | 1.80+ (stable) | `rust-toolchain.toml` requests `rustfmt` and `clippy` |
| [Node.js](https://nodejs.org/) | 20+ | CI uses 22 |
| [pnpm](https://pnpm.io/) | 9+ | CI uses 10; npm/yarn are not supported |

Windows is required to run the full test suite and to build release artifacts, because deployment is
implemented with Win32 NTFS hard links.

## Setup

```bash
git clone https://github.com/RolinShmily/sekiro-mod-manager.git
cd sekiro-mod-manager
pnpm install
```

## Everyday commands

| Command | Purpose |
| :--- | :--- |
| `cargo test --workspace` | Run every Rust unit and integration test |
| `cargo fmt --all` | Format Rust sources |
| `cargo clippy --workspace --all-targets -- -D warnings` | Lint Rust (warnings are errors in CI) |
| `pnpm --filter smm-desktop build` | Type-check and bundle the frontend |
| `pnpm run desktop:dev` | Run the desktop app in dev mode (hot reload) |
| `pnpm run package` | Full release packaging pipeline into `dist-installer/` |

## Before you open a pull request

Run the same gates CI runs and make sure all of them pass:

```bash
cargo fmt --all -- --check
cargo clippy --workspace --all-targets -- -D warnings
cargo test --workspace
pnpm --filter smm-desktop build
```

## Project layout

| Path | Contents |
| :--- | :--- |
| `crates/smm-core/` | Engine library — the only place mod logic should live |
| `crates/smm-cli/` | Headless CLI; one module per subcommand under `src/commands/` |
| `apps/smm-desktop/src/` | React UI (`api/`, `components/`, `utils/`) |
| `apps/smm-desktop/src-tauri/` | Tauri native layer and IPC commands |
| `scripts/` | Packaging pipeline and font subsetting |

## Guidelines

- **Keep logic in `smm-core`.** The CLI and the Tauri layer should stay thin wrappers so both stay
  in sync. Business rules added to one front-end but not the other are a bug.
- **No new runtime dependency without discussion.** Open an issue first. When you do add one, prefer
  an `--ignore-scripts` install, and update `LICENSING.md` if the dependency is redistributed.
- **Never bundle ModEngine or game assets.** See [LICENSING.md](LICENSING.md) for why.
- **Test with fixtures, not real mods.** Synthesize archives at runtime in a `tempfile` directory;
  do not commit third-party mod content.
- **Match the surrounding style.** Rust is formatted by `rustfmt` with default settings; keep
  comments explaining *why*, not *what*.

## Commit messages

This project follows [Conventional Commits](https://www.conventionalcommits.org/). The history uses
`feat`, `fix`, `refactor`, `docs`, `chore`, `ci`, `build`, and `test`, optionally scoped to a
subsystem, e.g.:

```text
fix(normalizer): support UI subdirectories under menu
docs(licensing): document the ModEngine redistribution decision
```

Use `!` or a `BREAKING CHANGE:` footer for incompatible changes. Keep commits focused; avoid
mixing unrelated refactors with behavior changes.

## Pull requests

1. Fork the repository and branch from `master`.
2. Make your change, add or update tests, and keep the diff focused.
3. Ensure every gate above passes.
4. Fill in the pull request template, including how you verified the change.

Maintainers may ask for a rebase. Do not commit build output (`target/`, `dist/`, `*.tsbuildinfo`)
or `staging/` content — these are ignored by `.gitignore` for a reason.

## Documentation & translations

`README.md` is the source of truth; `README.zh-CN.md` is its Simplified Chinese mirror. **Update both
in the same pull request** or explicitly note that the translation is out of date. License facts live
in `LICENSING.md` — change it only when the legal position actually changes.

## License

By contributing you agree that your contributions are licensed under the [MIT License](LICENSE),
without any additional terms or conditions. There is no CLA and no copyright assignment.
