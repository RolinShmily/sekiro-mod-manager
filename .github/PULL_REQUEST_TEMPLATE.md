## Summary

<!-- What does this change do, and why? Link the related issue: "Closes #123". -->

## Type of change

- [ ] Bug fix
- [ ] New feature
- [ ] Refactor / internal cleanup
- [ ] Documentation
- [ ] Build, CI, or packaging
- [ ] **Breaking change**

## Verification

<!-- How did you confirm this works? Paste relevant commands and output. -->

- [ ] `cargo fmt --all -- --check`
- [ ] `cargo clippy --workspace --all-targets -- -D warnings`
- [ ] `cargo test --workspace`
- [ ] `pnpm --filter smm-desktop build`
- [ ] Manually exercised the affected flow

## Checklist

- [ ] The change is focused; unrelated refactors are in separate commits.
- [ ] Logic lives in `smm-core` where possible, so the CLI and GUI stay in sync.
- [ ] Tests were added or updated for behavior changes.
- [ ] `README.md` **and** `README.zh-CN.md` were updated together (if user-facing docs changed).
- [ ] `LICENSING.md` was updated (if a redistributed dependency or the legal position changed).
- [ ] No ModEngine binaries, game assets, or third-party mod content were added.
- [ ] No build output was committed (`target/`, `dist/`, `*.tsbuildinfo`).
