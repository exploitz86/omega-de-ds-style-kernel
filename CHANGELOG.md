# Enhancement and Change Log for Enhanced 7.5

Changes in the upstream DS Style 7.5 release, followed by the changes this fork (Enhanced) makes on top of it (EZ-Flash Omega Definitive Edition).

## DS Style v7.5 (upstream)

- Added smooth carousel animations, enabled by default, with small or equal-size side previews and an option to turn animations off.
- Added a selection pulse before directly launching from an animated carousel.
- Completed square artwork borders in animated carousel views.
- Refreshed the startup screen and emulated-system icons, contributed by Merkin.
- Added an optional Wrap list setting for cycling between the first and last browser entries.
- Added defensive save and real-time-save file checks so incomplete SD-card reads or writes are reported instead of being treated as successful.
- Improved SD-card error handling, cheat-file bounds checking, and filename/path validation.

## Enhanced fork changes

### 1. Build system

- **Assembler include path** – GCC's `-I` does not reach GNU `as`, so `.include "gba_rts3_defs.inc"` failed.
  - `Build-RTS-Assembly-Preflight.ps1`: added `-Xassembler -I -Xassembler <source dir>`.
  - `Makefile`: `ASFLAGS` now includes `-I$(TOPDIR)/source -Xassembler -I -Xassembler $(TOPDIR)/source`.

### 2. Launcher fixes

- **Last Played tile** (`source\ezkernelnew.c`, `SD_list_MENU`)
  - A/Start did not always open the Clean/Add-on boot popup.
  - `Recent_GetLoadedPathAt` was passed `game_total_SD`, which is 0 when the browsed folder is empty. It now uses `LAUNCHER_MAX_RECENTS`.
- **Top bar colour**
  - `Grit\Build Skin Files.ps1` dropped the high byte of every top-bar pixel (PowerShell `[byte] -shl 8`), so palettes in `source\launcher_topbar_patterns.h` were wrong.
  - Fixed the script and regenerated the header and image headers.
- **Colour picker popup**
  - Pressing A on Settings > Colour opens a list of all 16 colours (`Launcher_ThemePickerPopup`).
- **Cheat group collapse** (`source\showcht.c`)
  - Collapsing a group left the old cheat rows on screen (they appeared to move to the bottom) because only the visible rows were redrawn, and the scroll position was not corrected for the shorter list.
  - `Show_KEY_val` now redraws all 10 rows, clearing unused ones, and the new `ClampCheatViewport` corrects the cursor and scroll offset after collapse or expand.
- **Enable BIOS from the start screen** (`source\Ezcard_OP.c`)
  - `key_L` was declared `u32` in `Ezcard_OP.c` but is a `u8` in `ezkernelnew.c`, so the neighbouring `gl_clock_dirty` flag was read as part of it. Launching from the start screen forced a soft reset and skipped the BIOS logo. It is now declared `u8`.
- **Remember cheats label** – now uses the multi-language system (`DSTEXT_SETTINGS_REMEMBER_CHEATS`) in all language packs and `launcher_text_languages.json`. The Thai entry uses the English text.
- **Build script** (`Build-DS-Style-Enhanced.cmd`) – asks whether to rebuild the image headers instead of always doing so.
- **Latest devkitPro (GCC 16.1) with Enable BIOS on** (`source\Ezcard_OP.c`)
  - `SetRompageWithHardReset` runs after the game ROM is mapped, so it must not call code stored in ROM. The new compiler used a jump-table helper from libgcc and `memset` calls, both in ROM, which crashed.
  - The function is now built with `no-jump-tables` and `no-tree-loop-distribute-patterns`. The `bootmode 4` path still calls `LZ77UnCompWram`.

### 3. Cheat selection memory (user enhancement)

- **Remember cheat state option** – can be turned on or off in the Game menu in Settings.
- **Cheats on by default** – if cheats are selected when starting a game, they are turned on automatically. There is no need to open the add-on menu and enable them.
- `source\showcht.c`: per-game cheat selection is saved to `/SYSTEM/CHEATSTATE/<hash>.cst` and restored.

### 4. RTS / savestates

- **Savestates independent of cheats** (`source\rts3_identity.c`)
  - The savestate header included a CRC and size of the injected runtime, which changes with cheats, hotkeys and engine variant.
  - The CRC field is now a constant derived from the RTS3 magic and kernel ABI; the size field is 0.
  - A state is still bound to game code, ROM size, ROM header CRC, install address and ABI.
- **In-game menu follows add-on settings** (`source\gba_rts_patch.s`, `source\GBApatch.c`, `source\GBApatch.h`)
  - RTS-only used a separate engine with instant L+R+Select save and L+R+Start load (no menu). That engine is no longer selected (code left in place, unused); RTS always uses the menu engine.
  - New runtime variable `RTS_menu_min`: RESET is hidden and unselectable unless Reset is enabled.
  - With no active cheats the menu lists SAVE and LOAD only. Cheat ON/OFF entries appear only when cheats are active.
  - The runtime cheat list is built only when Cheats is on.

### 5. Compatibility notes

- Savestates made by the original kernel, or by earlier builds of this one, are rejected. The format changed, and the RTS-only format tag `EZRTSO03` is no longer used.
- Normal in-game save files are unaffected.
- Avoid switching between this kernel and the original on the same SD card for savestates.

### 6. Files modified

- `Makefile`
- `Build-RTS-Assembly-Preflight.ps1`
- `Grit\Build Skin Files.ps1`
- `source\ezkernelnew.c`
- `source\showcht.c`
- `source\Ezcard_OP.c`
- `source\launcher_text.h`
- `source\launcher_runtime_text.h`
- `source\launcher_text_languages.json`
- `Build-DS-Style-Enhanced.cmd`
- `Build-DS-Style-Enhanced.ps1`
- `source\GBApatch.c`
- `source\GBApatch.h`
- `source\gba_rts_patch.s`
- `source\rts3_identity.c`
- `source\launcher_topbar_patterns.h`
