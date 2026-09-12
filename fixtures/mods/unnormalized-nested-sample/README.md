# Unnormalized Nested Sample Mod (Test Fixture)

**Author:** CommunityPacker  
**License:** MIT

---

## Purpose
This test fixture intentionally reproduces the messy, non-standard archive structures frequently created by mod authors on NexusMods.

Instead of placing canonical asset folders (`parts/`, `chr/`, etc.) at the archive root, authors often wrap them inside multiple nested folders:
```text
unnormalized-nested-sample/
├── mod.json
├── README.md
└── SomeMod_v1.0/
    ├── Documentation/
    │   └── HowToInstall.txt
    ├── Extra_Screenshots/
    │   └── preview.jpg
    └── Sekiro_Patch/
        └── mods/
            └── parts/
                ├── wp_a_0300.partsbnd.dcx
                └── bd_m_9000.partsbnd.dcx
```

## Normalizer Algorithm Requirements
1. The SMM Normalizer must traverse the directory hierarchy.
2. It must identify the canonical asset directory `parts/` inside `SomeMod_v1.0/Sekiro_Patch/mods/`.
3. It must deduce that `SomeMod_v1.0/Sekiro_Patch/mods` is the canonical game asset root (`canonical_root`).
4. Non-game asset directories (like `Documentation/` and `Extra_Screenshots/`) must be safely excluded from deployment mappings.
5. Assets must normalize to `parts/wp_a_0300.partsbnd.dcx` and `parts/bd_m_9000.partsbnd.dcx`.
