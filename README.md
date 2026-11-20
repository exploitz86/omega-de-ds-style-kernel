# DS Style 8.2 Enhanced

<p align="center">
  <a href="https://github.com/exploitz86/omega-de-ds-style-kernel/releases">
    <img
      alt="GitHub Downloads - All Releases"
      src="https://img.shields.io/github/downloads/exploitz86/omega-de-ds-style-kernel/total?style=social"
    >
  </a>
  <a href="https://github.com/exploitz86/omega-de-ds-style-kernel/releases/latest">
    <img
      alt="GitHub Downloads - Latest Release"
      src="https://img.shields.io/github/downloads/exploitz86/omega-de-ds-style-kernel/latest/total?style=social"
    >
  </a>
</p>

Source for **DS Style 8.2 Enhanced** for the EZ-FLASH OMEGA Definitive Edition.

This is a fork of [FrankieT19's DS Style kernel](https://github.com/FrankieT19/omega-de-ds-style-kernel). It also builds on work from [SkillerCMP's fork](https://github.com/SkillerCMP/omega-de-ds-style-kernel). See [CHANGELOG.md](CHANGELOG.md) for everything this fork changes.

## Current Release

DS Style v8.2. Copy the contents of the download's `SD Card` folder to the root of your microSD card, then boot while holding **R**. Artwork is included; no separate optional-artwork step is needed. Back up your card first. Existing games, saves and settings remain compatible.

Supports the original Omega DE **Rev.A and Rev.B**, not the newer **Omega Definitive Edition B (EZODE-B)**.


## Enhancements in this fork

- Remember cheat selection per game, with an optional setting in Settings > Game (localised label)
- Cheat group collapse fixed: collapsed cheats are hidden instead of dropping to the bottom of the screen
- "Enable BIOS" shows the Game Boy logo when launching from the home screen as well as the file browser
- Builds and runs with both devkitPro r54 and the latest devkitPro (GCC 16.1), including with Enable BIOS on
- Carousel pictures no longer disappear when scrolling in horizontal and vertical views
- The build script asks whether to rebuild the image headers instead of always doing so

## Documentation

These links belong to the upstream project by FrankieT19.

- [DS Style User Guide](https://frankiet19.github.io/omega-de-ds-style-kernel/) - installation, everyday use, artwork, customisation and troubleshooting
- [DS Style homepage](https://frankiet19.github.io/omega-de-ds-style-kernel/project/)
- [DS Style Manager](https://frankiet19.github.io/omega-de-ds-style-kernel/manager/) - guided installation, preferences, artwork and styles

## Features

- Rebind the in-game menu and exit shortcuts in Settings > Games > Addon settings.

- DS-style launcher interface
- Multiple accent colours and Light, Dark or clock-based Auto theme
- Customisable launcher assets through DS Style Customiser
- List, List + Art, horizontal carousel, and vertical carousel file views
- Title and box-art thumbnail modes
- Favourite and recent game support
- Multi-language launcher text, including Chinese and Thai
- Bundled text and Markdown viewer with light and dark modes
- Included title, box and console-folder artwork in the standard installation
- UI sounds
- Smooth carousel animations enabled by default, adjustable side-preview sizes and a launch pulse
- Wrap-around browsing and menus enabled by default, with defensive file checks
- Runtime settings stored in readable `SYSTEM/SETTINGS.TXT`

## Building

Install devkitPro/devkitARM (r54 or the latest release) and Python 3 (available as `python` on PATH), then run:

```bat
Build-DS-Style-Enhanced.cmd
```

The script asks whether to rebuild the image headers in the `images` folder. Answer `N` to skip that step when you haven't changed any artwork. The build log is written to the `build` folder.

When you answer `Y`, the build helper first verifies the editable PNG/BMP graphics by regenerating all 99 compiled image assets with the bundled Grit tool and comparing their image-array data with the known-good headers. If every asset matches, it regenerates the image headers, performs source validation and the native RTS assembly preflight, then builds the kernel. A successful native build produces `ezkernelnew.bin`.

The image verification is intentionally strict: a changed picture that no longer reproduces the current compiled asset stops the build before any header is replaced. `Grit\Build Skin Files.bat` runs the same complete graphics pipeline by itself.

Build support files:

- `Build-DS-Style-Enhanced.ps1`
- `Build-RTS-Assembly-Preflight.ps1`
- `Makefile`

## SD card patches

All packaged GBA compatibility patch profiles are contained under:

```text
SD-Card-Addons/SYSTEM/PATCHES/GBA/
```

The files directly under `SD-Card-Addons/SYSTEM/PATCHES/` document and verify that patch database. Copy the `SD-Card-Addons/SYSTEM` folder to the root of the SD card and merge it with the existing `SYSTEM` folder. At runtime the kernel still loads these profiles from `/SYSTEM/PATCHES/GBA/`.

`SD-Card-Addons/` also contains the optional companion text/Markdown plug-ins.

## Savestates

GBA savestates use RTS3. The source includes the current RTS3 inspector and maintenance tools under `tools/`.
New add-on launches use the existing `/SYSTEM/RTS` location; NOR games already
written with the upstream v8.2 add-on continue to use `/SYSTEM/RTS/DS81`.
Back up savestates before updating or rewriting a NOR game. Incompatible
savestates cannot be converted automatically.

The configurable hold-to-exit shortcut in the RTS3 GBA add-on counts VBlank
interrupts, so it may not work in games that do not generate them. Use the
in-game menu's Exit option instead.

- Original kernel source by EZ-FLASH
- SimpleLight custom kernel by Sterophonick
- DS Style custom kernel by FrankieT19
- Additional work from SkillerCMP's fork
- Enhancements in this fork by exploitz86
- Contributions by Merkin
- Contributions by timdmackey
- MiGoomba by michyprima, based on Jagoomba Color by Jaga, Goomba Color by
  Dwedit, and Goomba by FluBBa
- Thai language support by aidiadayo
- Case-sensitive `HZK12.h` include fix by Kekun
- FatFs by ChaN, as used by the upstream kernel

The bundled GB/GBC emulator is MiGoomba v0.1 (`ezode` build), based on Jagoomba
Color and Goomba Color. Its [pinned upstream source](https://github.com/michyprima/migoomba/tree/4b401e2ac7876aed9f89face867fd43d89ce189d)
is available separately. Existing `.esv` paths are retained; back up saves
before updating because compatibility with old emulator save states is not
established. The build script can optionally check the latest MiGoomba release
and offer to download its `ezode` binary before building. To embed a binary
manually, pass the currently pinned binary to `python tools/embed_migoomba.py`;
use `python tools/embed_migoomba.py --check` to verify the checked-in payload.

## License

See `LICENSE` and the licenses supplied with third-party components in their respective directories.
