#!/usr/bin/env bash
# Делает report.txt из REPORT.md и собирает архив docker-homework.tar.gz:
# внутри каталог docker-homework/ с приложением, Dockerfile, compose.yml и report.txt.
#   ./dist.sh
set -euo pipefail
export LC_ALL=C.UTF-8

cd "$(dirname "$0")"

NAME=docker-homework
FILES=(Dockerfile Dockerfile.naive Dockerfile.scratch .dockerignore compose.yml check.sh)
APP=(app/Makefile app/main.c app/greet.c app/greet.h app/stats.c app/stats.h)

# Строка из символа $2 той же длины, что и $1 (длина в символах, не в байтах).
underline() {
    local text=$1 ch=$2 line=""
    for ((i = 0; i < ${#text}; i++)); do line+=$ch; done
    printf '%s\n' "$line"
}

# Убирает разметку внутри строки: **жирный**, `код`, [текст](ссылка).
# Шаблон **/ из .dockerignore — не жирный, его прячем и возвращаем.
strip_inline() {
    sed -e 's#\*\*/#\x01#g' -e 's/\*\*//g' -e 's#\x01#**/#g' \
        -e 's/`//g' -e 's/\[\([^]]*\)\]([^)]*)/\1/g'
}

# Таблица markdown -> выровненные колонки без |. Строки копятся в table_rows,
# печатаются, когда таблица кончилась. Под шапкой — линия из дефисов.
table_rows=()

flush_table() {
    ((${#table_rows[@]})) || return 0
    local -a widths=() cells
    local row cell i n line
    for row in "${table_rows[@]}"; do
        IFS='|' read -r -a cells <<<"$row"
        for ((i = 0; i < ${#cells[@]}; i++)); do
            n=${#cells[i]}
            ((n > ${widths[i]:-0})) && widths[i]=$n
        done
    done
    n=0
    for row in "${table_rows[@]}"; do
        IFS='|' read -r -a cells <<<"$row"
        line=""
        for ((i = 0; i < ${#cells[@]}; i++)); do
            cell=${cells[i]}
            if ((i < ${#cells[@]} - 1)); then
                line+=$cell$(printf '%*s' $((widths[i] - ${#cell} + 2)) '')
            else
                line+=$cell
            fi
        done
        printf '%s\n' "$line"
        if ((n++ == 0)); then
            line=""
            for ((i = 0; i < ${#widths[@]}; i++)); do
                line+=$(underline "$(printf '%*s' "${widths[i]}" '')" '-')
                ((i < ${#widths[@]} - 1)) && line+="  "
            done
            printf '%s\n' "$line"
        fi
    done
    table_rows=()
}

# Строка таблицы: убрать крайние |, обрезать пробелы у ячеек, снять разметку.
add_table_row() {
    local row=$1 cell out="" first=1
    local -a cells
    row=${row#|}; row=${row%|}
    IFS='|' read -r -a cells <<<"$row"
    for cell in "${cells[@]}"; do
        cell=$(printf '%s' "$cell" | sed -e 's/^ *//' -e 's/ *$//' | strip_inline)
        if ((first)); then out=$cell; first=0; else out+="|$cell"; fi
    done
    table_rows+=("$out")
}

md_to_txt() {
    local in_code=0 line text
    local fence='^[[:space:]]*```'
    local table_sep='^\|[-|: ]+\|$'
    while IFS= read -r line || [[ -n $line ]]; do
        if ((!in_code)) && [[ $line == '|'* ]]; then
            [[ $line =~ $table_sep ]] || add_table_row "$line"
            continue
        fi
        flush_table
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
    flush_table
}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/$NAME/app"
cp "${FILES[@]}" "$tmp/$NAME/"
cp "${APP[@]}" "$tmp/$NAME/app/"
md_to_txt < REPORT.md > "$tmp/$NAME/report.txt"
# Права задаю явно: с Windows-ФС файлы приходят с битом выполнения.
find "$tmp/$NAME" -type d -exec chmod 755 {} +
find "$tmp/$NAME" -type f -exec chmod 644 {} +
chmod 755 "$tmp/$NAME/check.sh"
tar --sort=name --owner=0 --group=0 -czf "$NAME.tar.gz" -C "$tmp" "$NAME"

echo "готово: $NAME.tar.gz"
tar tzf "$NAME.tar.gz"
