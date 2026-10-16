#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

source_file="$repo_root/source/ezkernelnew.c"
if [ "$#" -gt 1 ]; then
	echo "usage: $0 [git-ref]" >&2
	exit 2
fi
if [ "$#" -eq 1 ]; then
	git -C "$repo_root" show "$1:source/ezkernelnew.c" > "$test_tmp/ezkernelnew.c"
	source_file="$test_tmp/ezkernelnew.c"
fi

cp -R "$repo_root/source/ff15" "$test_tmp/ff15"
sed -i 's/#define FF_USE_MKFS[[:space:]]*0/#define FF_USE_MKFS 1/' "$test_tmp/ff15/ffconf.h"
awk '/^u32 SavefileWrite\(/{p=1} p{print} p&&/^}$/{exit}' \
	"$source_file" > "$test_tmp/savefilewrite.impl.c"
test -s "$test_tmp/savefilewrite.impl.c"
awk '/^u32 IWRAM_CODE Save_savefile\(/{p=1} p{print} p&&/^}$/{exit}' \
	"$source_file" > "$test_tmp/savesavefile.impl.c"
test -s "$test_tmp/savesavefile.impl.c"

gcc -std=gnu11 -O2 -Wall -Wextra -Wno-sign-compare \
	-I"$test_tmp/ff15" -I"$repo_root/source" \
	-DSAVEFILEWRITE_IMPL="\"$test_tmp/savefilewrite.impl.c\"" \
	-DSAVESAVEFILE_IMPL="\"$test_tmp/savesavefile.impl.c\"" \
	"$repo_root/tests/savefile-host.c" \
	"$test_tmp/ff15/ff.c" "$test_tmp/ff15/ffunicode.c" \
	-o "$test_tmp/savefile-host"

"$test_tmp/savefile-host"
