#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

cp -R "$repo_root/source/ff15" "$test_tmp/ff15"
sed -i 's/#define FF_USE_MKFS[[:space:]]*0/#define FF_USE_MKFS 1/' "$test_tmp/ff15/ffconf.h"

gcc -std=c99 -O2 -Wall -Wextra \
    -I"$test_tmp/ff15" -I"$repo_root/source" -I"$repo_root/tests" \
    "$repo_root/source/settings_store.c" \
    "$test_tmp/ff15/ff.c" "$test_tmp/ff15/ffunicode.c" \
    "$repo_root/tests/settings-store-host.c" \
    -o "$test_tmp/settings-store-host"

"$test_tmp/settings-store-host"
