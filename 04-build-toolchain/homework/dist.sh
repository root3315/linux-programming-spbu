#!/usr/bin/env bash
# Делает report.txt из REPORT.md и собирает архив build-homework.tar.gz:
# внутри каталог build-homework/ с исходниками и report.txt.
#   ./dist.sh
set -euo pipefail
export LC_ALL=C.UTF-8

cd "$(dirname "$0")"

NAME=build-homework
SOURCES=(CMakeLists.txt Makefile meson.build main.c greet.c greet.h stats.c stats.h)

# Строка из символа $2 той же длины, что и $1 (длина в символах, не в байтах).
underline() {
    local text=$1 ch=$2 line=""
    for ((i = 0; i < ${#text}; i++)); do line+=$ch; done
    printf '%s\n' "$line"
}

# Убирает разметку внутри строки: **жирный**, `код`, [текст](ссылка).
strip_inline() {
    sed -e 's/\*\*//g' -e 's/`//g' -e 's/\[\([^]]*\)\]([^)]*)/\1/g'
}

md_to_txt() {
    local in_code=0 line text
    local fence='^[[:space:]]*```'   # ``` может стоять с отступом внутри списка
    while IFS= read -r line || [[ -n $line ]]; do
        if [[ $line =~ $fence ]]; then
            in_code=$((1 - in_code))
            continue
        fi
        if ((in_code)); then
            if [[ -z $line ]]; then echo; else printf '  %s\n' "$line"; fi
        elif [[ $line == '# '* ]]; then
            text=$(printf '%s' "${line#\# }" | strip_inline)
            printf '%s\n' "$text"; underline "$text" '='
        elif [[ $line == '## '* || $line == '### '* ]]; then
            text=$(printf '%s' "${line#\#* }" | strip_inline)
            printf '%s\n' "$text"; underline "$text" '-'
        else
            printf '%s\n' "$line" | strip_inline
        fi
    done
}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/$NAME"
cp "${SOURCES[@]}" "$tmp/$NAME/"
md_to_txt < REPORT.md > "$tmp/$NAME/report.txt"
# Права задаю явно: с Windows-ФС файлы приходят с битом выполнения.
chmod 755 "$tmp/$NAME"
chmod 644 "$tmp/$NAME"/*
tar --sort=name --owner=0 --group=0 -czf "$NAME.tar.gz" -C "$tmp" "$NAME"

echo "готово: $NAME.tar.gz"
tar tzf "$NAME.tar.gz"
