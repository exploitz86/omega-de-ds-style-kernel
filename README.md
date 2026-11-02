# DS Style 7.5 Enhanced

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

Source release for **DS Style 7.5 Enhanced** for the EZ-FLASH OMEGA Definitive Edition.

This is a fork of [FrankieT19's DS Style kernel](https://github.com/FrankieT19/omega-de-ds-style-kernel). It also builds on work from [SkillerCMP's fork](https://github.com/SkillerCMP/omega-de-ds-style-kernel). See [CHANGELOG.md](CHANGELOG.md) for everything this fork changes.

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

- DS-style launcher interface
- Multiple colour themes and light/dark presentation support
- Customisable launcher assets through DS Style Customiser
- List, List + Art, horizontal carousel, and vertical carousel file views
- Title and box-art thumbnail modes
- Favourite and recent game support
- Multi-language launcher text, including Chinese and Thai
- Bundled text and Markdown viewer with light and dark modes
- Optional folder artwork for supported emulated systems
- UI sounds
- Smooth carousel animations enabled by default, adjustable side-preview sizes and a launch pulse
- Optional browser wrapping and defensive save-file checks
- Runtime settings stored in readable `SYSTEM/SETTINGS.TXT`

## Building

Install devkitPro/devkitARM (r54 or the latest release), then run:

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

- Original kernel source by EZ-FLASH
- SimpleLight custom kernel by Sterophonick
- DS Style custom kernel by FrankieT19
- Additional work from SkillerCMP's fork
- Enhancements in this fork by exploitz86
- Contributions by Merkin
- Thai language support by aidiadayo
- Case-sensitive `HZK12.h` include fix by Kekun
- FatFs by ChaN, as used by the upstream kernel

## License

See `LICENSE` and the licenses supplied with third-party components in their respective directories.
