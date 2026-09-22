# Security Policy

## Supported versions

Only the latest release on the `master` branch is supported. Fixes are shipped as a new tagged
release; older tags are not patched.

| Version | Supported |
| :--- | :--- |
| Latest release (`v0.1.x`) | ✅ |
| Anything older | ❌ |

## Reporting a vulnerability

**Do not open a public issue.** Use GitHub's private vulnerability reporting:

1. Go to the [Security tab](https://github.com/RolinShmily/sekiro-mod-manager/security) of the
   repository.
2. Click **Report a vulnerability** and describe the issue.

If that is unavailable, contact the maintainer [@RolinShmily](https://github.com/RolinShmily)
directly through GitHub.

Please include, where possible:

- Affected version or commit.
- Steps to reproduce, ideally a minimal proof of concept.
- Impact: what an attacker gains and under what preconditions.
- Any suggested fix.

Reports are acknowledged on a best-effort basis; there is no formal SLA. Please give a reasonable
window to ship a fix before public disclosure, and credit will be given in the release notes unless
you prefer to stay anonymous.

## Scope

In scope:

- The desktop application (`Sekiro-Mod-Manager.exe`), the CLI (`smm.exe`), and the Rust crates in
  `crates/`.
- Archive extraction (`zip` / `7z` / `rar`), path normalization, and hard-link deployment.
- Anything that could write outside the game or staging directory, such as path traversal or
  zip-slip style bugs.
- The packaging pipeline in `scripts/`.

Out of scope:

- **Sekiro Mod Engine (ModEngine).** SMM does not ship or bundle it — report issues to
  [katalash/ModEngine](https://github.com/katalash/ModEngine).
- **Third-party mods.** Content you import is untrusted by design; a malicious mod is not an SMM
  vulnerability, although a failure to *contain* one is.
- **The game itself** (*Sekiro: Shadows Die Twice*) or any FromSoftware/Activision service.
- Vulnerabilities that require an already-compromised machine or pre-existing local administrator
  rights.

## Handling of untrusted input

SMM extracts archives and creates hard links. When reporting, assume archives, mod metadata, and
`modengine.ini` contents are attacker-controlled, and prefer to demonstrate an escape from the
intended `mods/` or staging directory.
