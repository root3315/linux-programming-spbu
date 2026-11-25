#!/usr/bin/env bash
# Переводит Markdown в простой текст для report.txt:
# заголовки подчёркнуты === и ---, блоки кода с отступом в два пробела.
#   ./md2txt.sh <REPORT.md >report.txt
set -euo pipefail
export LC_ALL=C.UTF-8

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

in_code=0
fence='^[[:space:]]*```'
while IFS= read -r line || [[ -n $line ]]; do
    if [[ $line =~ $fence ]]; then
        in_code=$((1 - in_code))
        continue
    fi
    if ((in_code)); then
        if [[ -z $line ]]; then echo; else printf '  %s\n' "$line"; fi
    elif [[ $line == '# '* ]]; then
        text=$(printf '%s' "${line#\# }" | strip_inline)
        printf '%s\n' "$text"
        underline "$text" '='
    elif [[ $line == '## '* || $line == '### '* ]]; then
        text=$(printf '%s' "${line#\#* }" | strip_inline)
        printf '%s\n' "$text"
        underline "$text" '-'
    else
        printf '%s\n' "$line" | strip_inline
    fi
done
