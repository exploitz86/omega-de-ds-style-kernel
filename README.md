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
- [DS Style Manager](https://frankiet19.github.io/omega-de-ds-style-kernel/manager/) - guided installation, starting preferences, artwork and style tools

## Features

- DS-style launcher interface
- Multiple colour themes and light/dark presentation support
- Customisable launcher assets through DS Style Customiser
- List, List + Art, horizontal carousel, and vertical carousel file views
- Title and box-art thumbnail modes
- Favourite and recent game support
- Multi-language launcher text, including Chinese and Thai
- Bundled text and Markdown viewer with light and dark presentation modes
- Optional folder artwork for every supported emulated system
- UI sounds
- Runtime settings stored in readable `SYSTEM/SETTINGS.TXT`

## Transparent mirrored thumbnails

The forked DS Style Thumbnail Scraper can create mirrored thumbnails with selective transparency. Keep the `DST1` marker it writes into BMP header bytes 6–9; the launcher uses that marker to make zero-valued RGB555 pixels transparent while drawing the other pixels normally. This lets the artwork blend with the launcher background. Without the marker, the thumbnail is drawn as an ordinary opaque image.

Marked thumbnails use the launcher's standard 16-bit, uncompressed, top-down BMP layout: 120 × 80 pixels for title artwork or 80 × 80 for box artwork. The marker does not provide a full alpha channel; pure black (RGB555 value `0x0000`) is transparent in a marked image, so use a nonzero near-black value where black must remain visible. The companion scraper creates the marker and pixel data together; do not remove or edit the marker when copying its mirrored thumbnails.

## Building

On Windows with the existing devkitARM toolchain installed, run:

```bat
build.bat
```

The build script outputs:

```text
Build-DS-Style-Enhanced.cmd
```

The build helper first verifies the editable PNG/BMP graphics by regenerating all 99 compiled image assets with the bundled Grit tool and comparing their image-array data with the known-good headers. If every asset matches, it regenerates the image headers, performs source validation and the native RTS assembly preflight, then builds the kernel. A successful native build produces `ezkernelnew.bin`.

The image verification is intentionally strict: a changed picture that no longer reproduces the current compiled asset stops the build before any header is replaced. `Grit\Build Skin Files.bat` runs the same complete graphics pipeline by itself.

For GitHub Codespaces, rebuild the container to install the configured devkitPro environment, then run `make` from the repository root. This builds the `.gba` target; the Windows `build.bat` workflow performs the additional copy to `ezkernelnew.bin`.

## Skin Assets

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

```text
python tools/inspect_rts3.py "Game.rts"
```

## Documentation

The user guide is:

```text
docs/DS Style User Guide.pdf
```

## License

See `LICENSE` and the licenses supplied with third-party components in their respective directories.
