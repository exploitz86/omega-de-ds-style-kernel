#!/usr/bin/env bash
set -euo pipefail

# Install devkitPro's package manager and the GBA toolchain group. This script
# can run as root or as a Codespaces user with sudo access.

DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
DEVKITARM="${DEVKITARM:-${DEVKITPRO}/devkitARM}"
if [[ -x "${DEVKITARM}/bin/arm-none-eabi-gcc" && -f "${DEVKITARM}/gba_rules" ]]; then
	printf 'devkitARM and the GBA build rules are already available at %s.\n' "$DEVKITARM"
	exit 0
fi

if [[ "$(id -u)" -eq 0 ]]; then
	privilege=()
elif command -v sudo >/dev/null 2>&1; then
	privilege=(sudo)
else
	printf 'This installer needs root privileges (run it with sudo).\n' >&2
	exit 1
fi

if ! command -v wget >/dev/null 2>&1; then
	printf 'wget is required to download the official devkitPro pacman installer.\n' >&2
	exit 1
fi

if ! command -v dkp-pacman >/dev/null 2>&1; then
	installer="$(mktemp)"
	trap 'rm -f "$installer"' EXIT
	printf 'Downloading the official devkitPro pacman installer...\n'
	if ! wget --https-only --output-document="$installer" \
		https://apt.devkitpro.org/install-devkitpro-pacman; then
		printf 'The devkitPro installer endpoint is unreachable. Rebuild this Codespace to use the configured official devkitPro image instead.\n' >&2
		exit 1
	fi
	chmod +x "$installer"
	"${privilege[@]}" "$installer"
fi

printf 'Updating devkitPro package metadata...\n'
if ! "${privilege[@]}" dkp-pacman -Syu --noconfirm; then
	printf 'devkitPro package downloads failed. Rebuild this Codespace to use the configured official devkitPro image instead.\n' >&2
	exit 1
fi
printf 'Installing the GBA development tool group...\n'
if ! "${privilege[@]}" dkp-pacman -S --needed --noconfirm gba-dev; then
	printf 'The GBA packages could not be downloaded. Rebuild this Codespace to use the configured official devkitPro image instead.\n' >&2
	exit 1
fi

if [[ ! -x "${DEVKITARM}/bin/arm-none-eabi-gcc" ]]; then
	printf 'Install finished, but arm-none-eabi-gcc was not found under %s.\n' "$DEVKITARM" >&2
	exit 1
fi
if [[ ! -f "${DEVKITARM}/gba_rules" ]]; then
	printf 'Install finished, but the Makefile rules file is missing: %s/gba_rules\n' "$DEVKITARM" >&2
	exit 1
fi

printf '\nInstalled compiler:\n'
"${DEVKITARM}/bin/arm-none-eabi-gcc" --version | head -n 1
printf 'GBA rules: %s/gba_rules\n' "$DEVKITARM"
printf '\nTo use the updated environment in this terminal, run:\n'
printf '  source /etc/profile.d/devkit-env.sh\n'
printf 'Codespaces will also receive DEVKITPRO and DEVKITARM after a container rebuild.\n'
