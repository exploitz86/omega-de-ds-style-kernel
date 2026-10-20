# Enhancement and Change Log for 7.4 Enhanced

Changes made on top of the original DS Style 7.4c source (EZ-Flash Omega Definitive Edition).

## 1. Build system

- **Assembler include path** – GCC's `-I` does not reach GNU `as`, so `.include "gba_rts3_defs.inc"` failed.
  - `Build-RTS-Assembly-Preflight.ps1`: added `-Xassembler -I -Xassembler <source dir>`.
  - `Makefile`: `ASFLAGS` now includes `-I$(TOPDIR)/source -Xassembler -I -Xassembler $(TOPDIR)/source`.

## 2. Launcher fixes

- **Last Played tile** (`source\ezkernelnew.c`, `SD_list_MENU`)
  - A/Start did not always open the Clean/Add-on boot popup.
  - `Recent_GetLoadedPathAt` was passed `game_total_SD`, which is 0 when the browsed folder is empty. It now uses `LAUNCHER_MAX_RECENTS`.
- **Top bar colour**
  - `Grit\Build Skin Files.ps1` dropped the high byte of every top-bar pixel (PowerShell `[byte] -shl 8`), so palettes in `source\launcher_topbar_patterns.h` were wrong.
  - Fixed the script and regenerated the header and image headers.
- **Colour picker popup**
  - Pressing A on Settings > Colour opens a list of all 16 colours (`Launcher_ThemePickerPopup`).

## 3. Cheat selection memory (user enhancement)

- **Remember cheat state option** – can be turned on or off in the Game menu in Settings.
- **Cheats on by default** – if cheats are selected when starting a game, they are turned on automatically. There is no need to open the add-on menu and enable them.
- `source\showcht.c`: per-game cheat selection is saved to `/SYSTEM/CHEATSTATE/<hash>.cst` and restored.

## 4. RTS / savestates

- **Savestates independent of cheats** (`source\rts3_identity.c`)
  - The savestate header included a CRC and size of the injected runtime, which changes with cheats, hotkeys and engine variant.
  - The CRC field is now a constant derived from the RTS3 magic and kernel ABI; the size field is 0.
  - A state is still bound to game code, ROM size, ROM header CRC, install address and ABI.
- **In-game menu follows add-on settings** (`source\gba_rts_patch.s`, `source\GBApatch.c`, `source\GBApatch.h`)
  - RTS-only used a separate engine with instant L+R+Select save and L+R+Start load (no menu). That engine is no longer selected (code left in place, unused); RTS always uses the menu engine.
  - New runtime variable `RTS_menu_min`: RESET is hidden and unselectable unless Reset is enabled.
  - With no active cheats the menu lists SAVE and LOAD only. Cheat ON/OFF entries appear only when cheats are active.
  - The runtime cheat list is built only when Cheats is on.

## 5. Compatibility notes

- Savestates made by the original kernel, or by earlier builds of this one, are rejected. The format changed, and the RTS-only format tag `EZRTSO03` is no longer used.
- Normal in-game save files are unaffected.
- Avoid switching between this kernel and the original on the same SD card for savestates.

## 6. Files modified

- `Makefile`
- `Build-RTS-Assembly-Preflight.ps1`
- `Grit\Build Skin Files.ps1`
- `source\ezkernelnew.c`
- `source\showcht.c`
- `source\GBApatch.c`
- `source\GBApatch.h`
- `source\gba_rts_patch.s`
- `source\rts3_identity.c`
- `source\launcher_topbar_patterns.h`

## DS Style 7.4c

Initial source release.

- DS Style 7.4c launcher branding.
- External GBA compatibility database under `SYSTEM/PATCHES/GBA/`.
- Addon and cheat support.
- RTS3 GBA savestate core and RTS3 inspection/maintenance tools.
- Windows/devkitARM build helpers.
- Complete Grit image pipeline: verifies all 99 PNG/BMP-backed image headers against the known-good arrays, then regenerates the headers before the kernel build.
