#!/usr/bin/env bash
# Собирает архив для сдачи automation-homework.tar.gz:
#   automation-homework/report.txt          REPORT.md без разметки
#   automation-homework/hello-repo.bundle   весь репозиторий с историей и тегами
#   automation-homework/hello/              исходники проекта
#
#   ./dist.sh ПУТЬ_К_КОРНЮ_РЕПОЗИТОРИЯ
#
# Bundle — это .git в одном файле. Восстановить репозиторий:
#   git clone hello-repo.bundle repo
set -euo pipefail
export LC_ALL=C.UTF-8

if [[ $# -ne 1 ]]; then
    echo "usage: $0 REPO_ROOT" >&2
    exit 2
fi

REPO=$(cd "$1" && pwd)
cd "$(dirname "$0")"

NAME=automation-homework
HELLO_FILES=(.gitignore Makefile ci.sh main.c greet.c greet.h stats.c stats.h
             tests/test_hello.c packaging/build-deb.sh packaging/control)
EXECUTABLE=(ci.sh packaging/build-deb.sh)

if ! git -C "$REPO" rev-parse --git-dir >/dev/null 2>&1; then
    echo "dist.sh: $REPO — не git-репозиторий" >&2
    exit 1
fi

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
    local fence='^[[:space:]]*```'
    local comment='^[[:space:]]*<!--.*-->[[:space:]]*$'
    while IFS= read -r line || [[ -n $line ]]; do
        if [[ $line =~ $fence ]]; then
            in_code=$((1 - in_code))
            continue
        fi
        if ((in_code)); then
            if [[ -z $line ]]; then echo; else printf '  %s\n' "$line"; fi
        elif [[ $line =~ $comment ]]; then
            continue
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
out="$tmp/$NAME"
mkdir -p "$out/hello"

md_to_txt < REPORT.md > "$out/report.txt"

(cd hello && cp --parents "${HELLO_FILES[@]}" "$out/hello/")

git -C "$REPO" bundle create "$out/hello-repo.bundle" --all
if ! git -C "$REPO" bundle verify "$out/hello-repo.bundle" >/dev/null 2>&1; then
    echo "dist.sh: bundle не прошёл git bundle verify" >&2
    exit 1
fi

# Права задаю явно: с Windows-ФС файлы приходят с битом выполнения.
find "$out" -type d -exec chmod 755 {} +
find "$out" -type f -exec chmod 644 {} +
for f in "${EXECUTABLE[@]}"; do chmod 755 "$out/hello/$f"; done

tar --sort=name --owner=0 --group=0 -czf "$NAME.tar.gz" -C "$tmp" "$NAME"

echo "готово: $NAME.tar.gz"
tar tzf "$NAME.tar.gz"
