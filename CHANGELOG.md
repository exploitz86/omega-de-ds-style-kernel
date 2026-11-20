# Enhancement and Change Log for Enhanced 8.2

Changes in upstream DS Style releases, followed by the changes this fork (Enhanced) makes on top of them (EZ-Flash Omega Definitive Edition).

## Enhanced 8.2

- Replaced the bundled GB/GBC emulator with the MiGoomba v0.1 `ezode` build,
  based on Jagoomba Color and Goomba Color. The existing `.esv` paths are
  retained; back up saves before updating because old emulator save-state
  compatibility is unverified.
- Added an optional build-time check for new MiGoomba releases, with an explicit
  prompt before downloading and embedding a newer `ezode` binary.

## DS Style v8.2

- Added rebindable in-game menu and hold-to-exit shortcuts for GBA add-ons and GB/GBC games.

## DS Style v8.1

- Added a redesigned in-game add-on menu with Light and Dark palettes.
- Press Start + Select to open the menu, or hold Select to exit. The old configurable add-on and sleep hotkey settings have been removed.
- Exit game returns to the original screen and selection. The launcher stays covered until it is ready, and holding Select through the transition no longer changes the browser view.
- Moved pending save work behind the startup splash.
- Updated GB/GBC emulation to Jagoomba Color v0.5 with matching menu and exit controls and no decorative borders.

## DS Style v8.0

- Added smooth animated horizontal and vertical carousels, enabled by default, with adjustable side-art size and a direct-launch selection pulse.
- Refined the Start screen with pale selection-corner highlights and responsive easing animations.
- Refined menu sounds, square-framed pop-ups, startup transitions and theme-matched launch fades.
- Added a transparent loading ring centred on the selected artwork while preparing a game.
- Simplified Settings with contextual View options, Auto theme (dark from 20:00 to 08:00), and Open SYSTEM for file management.
- Made clean list names standard and kept the Start screen available independently of the Boot to choice.
- Enabled wrap-around browsing by default, including settings and pop-up menus; saved Off preferences remain unchanged.
- Made double-tap Select add or remove favourites without a confirmation popup.
- Refreshed splash screens and system icons, with contributions by Merkin.
- Added defensive save and real-time-save I/O checks, SD-card error handling, cheat bounds checking and filename/path validation.
- Prevented fragmented-file tables from overwriting adjacent data when their capacity is exceeded.
- Included artwork in the standard SYSTEM folder instead of a separate optional-extras folder.
- Updated in-kernel guidance, language support, Customiser compatibility, manager and user guide.

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
