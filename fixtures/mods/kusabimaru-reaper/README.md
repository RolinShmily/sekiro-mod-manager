# Kusabimaru Reaper Weapon & Arm

**Author:** Eyedea  
**NexusMods:** https://www.nexusmods.com/sekiro/mods/350  
**License:** Creative Commons Attribution-NonCommercial 4.0 International (CC-BY-NC-4.0)

---

## Overview

Replaces Wolf's default katana (Kusabimaru - `wp_a_0300`) with a dark, sharp Reaper blade and replaces Wolf's prosthetic arm (`am_m_9000`) with an ornate obsidian prosthetic design.

## Directory Structure and Asset Coverage
- `parts/wp_a_0300.partsbnd.dcx`: Wolf's primary katana weapon model, textures, and material bindings.
- `parts/am_m_9000.partsbnd.dcx`: Wolf's default prosthetic arm model and textures.

## Conflict Characteristics
- **Exclusive Slot Collision**: Conflicts with any mod replacing Wolf's default katana slot (`0300`) or prosthetic arm slot (`9000`).
- SMM will detect this as a `Warning` level semantic slot collision and allow priority ordering to select the winning appearance.
