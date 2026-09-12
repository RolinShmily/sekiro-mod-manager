# Sekiro: Dream of the Damned (DotD)

**Author:** Nuffly  
**Repository:** https://github.com/nuffly/DotD  
**NexusMods:** https://www.nexusmods.com/sekiro/mods/793  
**License:** Apache License 2.0 (Apache-2.0)

---

## Overview

Sekiro: Dream of the Damned is an extensive gameplay overhaul mod for *Sekiro: Shadows Die Twice*. It introduces revised combat pacing, enemy AI behaviors, reworked boss encounters (e.g. Sword Saint Isshin `c5110`), rebalanced status effects, new prosthetic combinations, and extensive parameter table redesigns.

## Directory Structure and Asset Coverage
- `chr/`: Character binders for player and bosses (`c0000.chrbnd.dcx`, `c5110.chrbnd.dcx`).
- `event/`: EMEVD event scripts altering boss arena boundaries, quest progression, and flags (`common.emevd.dcx`).
- `map/`: Scene definitions and collision maps (`m10_00_00_00.msb.dcx`).
- `msg/engus/`: English localization string binders (`item.msgbnd.dcx`) with revised item descriptions.
- `param/gameparam/`: Core `gameparam.parambnd.dcx` containing weapon stats, player attributes, SpEffectParams, and behavior parameters.
- `script/`: Custom Lua scripting (`talk.common.lua`) handling dialogues and scripted triggers.

## Compatibility & Conflict Notice
- **Critical File**: Modifies `param/gameparam/gameparam.parambnd.dcx`. This file will conflict with any other mod altering game parameters.
- **Priority**: Must be placed at high priority if playing the full overhaul experience.

## License
Licensed under the Apache License, Version 2.0. See `LICENSE` for details.
