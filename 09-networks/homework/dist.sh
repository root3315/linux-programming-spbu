#!/usr/bin/env bash
# Делает report.txt из REPORT.md и собирает архив net-homework.tar.gz:
# внутри каталог net-homework/ с исходниками и report.txt.
#   ./dist.sh
set -euo pipefail
export LC_ALL=C.UTF-8

cd "$(dirname "$0")"

NAME=net-homework
SOURCES=(Makefile chat.h chat_server.c chat_client.c)

# Строка из символа $2 той же длины, что и $1 (длина в символах, не в байтах).
underline() {
    local text=$1 ch=$2 line=""
    for ((i = 0; i < ${#text}; i++)); do line+=$ch; done
    printf '%s\n' "$line"
}

# Убирает разметку внутри строки: **жирный**, `код`, [текст](ссылка).
strip_inline() {
    sed -e 's/\*\*\([^*]*\)\*\*/\1/g' -e 's/`//g' -e 's/\[\([^]]*\)\]([^)]*)/\1/g'
}

# Таблица markdown -> колонки, выровненные пробелами (ширина в символах).
# Строки таблицы копятся в TABLE_ROWS, печатаются целиком, строка |---| выбрасывается.
TABLE_ROWS=()

flush_table() {
    ((${#TABLE_ROWS[@]})) || return 0
    local -a widths=() cells
    local row cell i out
    for row in "${TABLE_ROWS[@]}"; do
        IFS=$'\t' read -r -a cells <<< "$row"
        for i in "${!cells[@]}"; do
            ((${#cells[i]} > ${widths[i]:-0})) && widths[i]=${#cells[i]}
        done
    done
    for row in "${TABLE_ROWS[@]}"; do
        IFS=$'\t' read -r -a cells <<< "$row"
        out=""
        for i in "${!cells[@]}"; do
            cell=${cells[i]}
            if ((i < ${#cells[@]} - 1)); then
                while ((${#cell} < widths[i])); do cell+=" "; done
                cell+="  "
            fi
            out+=$cell
        done
        printf '%s\n' "$out"
    done
    TABLE_ROWS=()
}

# "| a | b |" -> "a<TAB>b" без разметки внутри ячеек.
table_row() {
    local line=$1
    line=${line#|}; line=${line%|}
    printf '%s\n' "$line" | strip_inline | sed -e 's/[[:space:]]*|[[:space:]]*/\t/g' -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//'
}

md_to_txt() {
    local in_code=0 line text
    local fence='^[[:space:]]*```'   # ``` может стоять с отступом внутри списка
    while IFS= read -r line || [[ -n $line ]]; do
        if [[ $line =~ $fence ]]; then
            in_code=$((1 - in_code))
            continue
        fi
        if ((!in_code)) && [[ $line == '|'* ]]; then
            [[ $line =~ ^\|[-:|[:space:]]+\|$ ]] || TABLE_ROWS+=("$(table_row "$line")")
            continue
        fi
        flush_table
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
    flush_table
}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir "$tmp/$NAME"
cp -r "${SOURCES[@]}" "$tmp/$NAME/"
md_to_txt < REPORT.md > "$tmp/$NAME/report.txt"
# Права задаю явно: с Windows-ФС файлы приходят с битом выполнения.
find "$tmp/$NAME" -type d -exec chmod 755 {} +
find "$tmp/$NAME" -type f -exec chmod 644 {} +
tar --sort=name --owner=0 --group=0 -czf "$NAME.tar.gz" -C "$tmp" "$NAME"

echo "готово: $NAME.tar.gz"
tar tzf "$NAME.tar.gz"
