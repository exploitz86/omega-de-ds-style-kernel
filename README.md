# DS Style Kernel for EZ-FLASH OMEGA Definitive Edition

DS Style is a custom kernel for the **EZ-FLASH OMEGA Definitive Edition**. It is based on the official EZ-FLASH OMEGA DE kernel source and Sterophonick's SimpleLight custom kernel and replaces the stock launcher with a Nintendo DS-inspired interface, theme system, thumbnail views, language options, and related quality-of-life changes.

> [!WARNING]
> This repository is for the **EZ-FLASH OMEGA Definitive Edition only**. Do not flash this build to the original EZ-FLASH OMEGA.

## User Guide

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

On Windows, install devkitPro/devkitARM and run:

```bat
build.bat
```

The build script outputs:

```text
ezkernelnew.bin
```

Copy `ezkernelnew.bin` to the root of the SD card and boot the cartridge while holding **R** to update the kernel.

For GitHub Codespaces, rebuild the container to install the configured devkitPro environment, then run `make` from the repository root. This builds the `.gba` target; the Windows `build.bat` workflow performs the additional copy to `ezkernelnew.bin`.

## Skin Assets

Theme image headers are generated from the files in `images/` using:

```bat
Grit\Build Skin Files.bat
```

Run this before building if you edit the theme BMP/PNG/JPG assets.

## Customising

For normal users, the recommended route is **DS Style Customiser**, which creates a private project copy, edits the assets/settings, and builds the kernel without modifying this source tree directly.

## Credits

- Original kernel source by EZ-FLASH
- SimpleLight custom kernel by Sterophonick
- DS Style custom kernel by FrankieT19
- Thai language support by aidiadayo
- Case-sensitive `HZK12.h` include fix by Kekun
- FatFs by ChaN, as used by the upstream kernel

## License

This project follows the license terms provided with the original EZ-FLASH kernel source. See `LICENSE`.
