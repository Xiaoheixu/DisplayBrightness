#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output=$(mktemp -d "${TMPDIR:-/tmp}/displaybrightness-tests.XXXXXX")
trap 'rm -rf "$output"' EXIT HUP INT TERM
for source in "$root"/tests/*-test.cpp; do
 name=$(basename "$source" .cpp)
 "${CXX:-clang++}" -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I "$root/src" "$source" -o "$output/$name"
 "$output/$name"
done
python3 "$root/tests/check-brightness-only.py"
python3 "$root/tests/diagnostic-export-test.py"
