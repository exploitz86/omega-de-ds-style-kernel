#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

source_file="$repo_root/source/showcht.c"
if [ "$#" -gt 1 ]; then
	echo "usage: $0 [git-ref]" >&2
	exit 2
fi
if [ "$#" -eq 1 ]; then
	git -C "$repo_root" show "$1:source/showcht.c" > "$test_tmp/showcht.c"
	source_file="$test_tmp/showcht.c"
fi

awk '/^unsigned long str2hex\(/{p=1} p{print} p&&/^}$/{exit}' \
	"$source_file" > "$test_tmp/cheat.impl.c"
awk '/^void Analyze_KEYVAL\(/{p=1} p{print} p&&/^}$/{exit}' \
	"$source_file" >> "$test_tmp/cheat.impl.c"
test -s "$test_tmp/cheat.impl.c"

gcc -std=gnu11 -O2 -Wall -Wextra \
	-I"$repo_root/source/ff15" \
	-DCHEAT_IMPL="\"$test_tmp/cheat.impl.c\"" \
	"$repo_root/tests/cheat-row-host.c" \
	-o "$test_tmp/cheat-row-host"

"$test_tmp/cheat-row-host"
