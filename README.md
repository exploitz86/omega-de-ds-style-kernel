# DS Style 7.4 Enhanced
</p>
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

Source release for **DS Style 7.4 Enhanced** for the EZ-FLASH OMEGA Definitive Edition.

The launcher displays **DS Style 7.4 Enhanced** on screen. GBA savestates use the RTS3 hardware-checkpoint core.

## Build

Read the complete [DS Style User Guide](https://frankiet19.github.io/omega-de-ds-style-kernel/) for installation, everyday use, artwork, customisation and troubleshooting.

## Web

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

Install devkitPro/devkitARM, then run:

```bat
build.bat
```

The build script outputs:

```text
Build-DS-Style-Enhanced.cmd
```

The build helper first verifies the editable PNG/BMP graphics by regenerating all 99 compiled image assets with the bundled Grit tool and comparing their image-array data with the known-good headers. If every asset matches, it regenerates the image headers, performs source validation and the native RTS assembly preflight, then builds the kernel. A successful native build produces `ezkernelnew.bin`.

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
- Contributions by Merkin
- Thai language support by aidiadayo
- Case-sensitive `HZK12.h` include fix by Kekun
- FatFs by ChaN, as used by the upstream kernel

## License

See `LICENSE` and the licenses supplied with third-party components in their respective directories.
