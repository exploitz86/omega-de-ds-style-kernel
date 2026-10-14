#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"
cd "$repo_root"

if [[ ! -f CODEX_KERNEL_AUDIT_PROGRESS.md ]]; then
	printf 'Audit checkpoint is missing: %s/CODEX_KERNEL_AUDIT_PROGRESS.md\n' "$repo_root" >&2
	exit 1
fi

printf 'Repository: %s\n' "$repo_root"
git status --short --branch

devkitarm="${DEVKITARM:-${DEVKITPRO:-/opt/devkitpro}/devkitARM}"
compiler="${devkitarm}/bin/arm-none-eabi-gcc"

if [[ ! -x "$compiler" || ! -f "${devkitarm}/gba_rules" ]]; then
	if [[ -z "${CODESPACE_NAME:-}" ]]; then
		cat >&2 <<'EOF'
devkitARM/GBA rules are not available, and CODESPACE_NAME is unset.
Open a terminal inside the target Codespace and run this script there.
EOF
		exit 1
	fi
	if ! command -v gh >/dev/null 2>&1; then
		printf 'GitHub CLI (gh) is required to request the Codespace rebuild.\n' >&2
		exit 1
	fi
	if ! gh auth status >/dev/null 2>&1; then
		printf 'GitHub CLI is not authenticated. Run `gh auth login`, then rerun this script.\n' >&2
		exit 1
	fi
	if [[ ! -f .devcontainer/Dockerfile || ! -f .devcontainer/devcontainer.json ]]; then
		printf 'The repository devcontainer configuration is missing.\n' >&2
		exit 1
	fi

	cat <<EOF

devkitARM is not installed in this container. Requesting a Codespace rebuild
for ${CODESPACE_NAME} using this repository's devcontainer configuration.
GitHub documents that rebuild preserves the working directory and its changes.
The terminal may disconnect while the container is replaced.
EOF
	gh codespace rebuild --codespace "$CODESPACE_NAME"
	cat <<'EOF'

When the Codespace reconnects, open a new terminal and run:
  scripts/rebuild-and-verify-audit.sh

The second run will verify devkitARM, run the production Makefile build without
`make clean`, and execute the available regression checks.
EOF
	exit 0
fi

printf '\nFound devkitARM compiler:\n'
"$compiler" --version | head -n 1
printf 'GBA rules: %s/gba_rules\n' "$devkitarm"

printf '\nRunning production Makefile build (without cleanup)...\n'
make

printf '\nRunning FatFs settings persistence fault-injection scenarios...\n'
scripts/test-settings-store-host.sh

printf '\nRunning browser installer regression scenarios...\n'
node tests/browser-installer.mjs

printf '\nChecking JavaScript syntax...\n'
for js_file in docs/manager/js/*.js; do
	node --check "$js_file"
done

printf '\nChecking whitespace errors...\n'
git diff --check

if command -v cppcheck >/dev/null 2>&1; then
	printf '\nRunning Cppcheck on modified firmware files...\n'
	cppcheck --enable=warning,performance,portability \
		--suppress=missingIncludeSystem \
		--suppress=arithOperationsOnVoidPointer \
		--inline-suppr --template=gcc \
		source/Ezcard_OP.c source/GBApatch.c source/showcht.c source/ezkernelnew.c source/settings_store.c
else
	printf '\nCppcheck is not installed; skipping that optional check.\n'
fi

printf '\nChecks completed. Review generated build artifacts and git status; this script does not clean, stage, commit, or push.\n'
git status --short --branch
