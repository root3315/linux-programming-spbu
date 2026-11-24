#!/usr/bin/env bash
# Считает файлы *.txt в каталоге (без подкаталогов) и печатает их имена.
set -euo pipefail

if [[ $# -ne 1 || ! -d "$1" ]]; then
    printf 'Usage: %s DIRECTORY\n' "$0" >&2
    exit 1
fi

dir="$1"
count=0
for file in "$dir"/*.txt; do
    [[ -f "$file" ]] || continue
    printf 'Found: %s\n' "$file"
    ((count += 1))
done
printf 'Found: %d\n' "$count"
