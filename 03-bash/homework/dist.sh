#!/usr/bin/env bash
# Собирает архив для сдачи: bash-homework.tar.gz с каталогом bash-homework/ внутри.
# bash-homework.txt делается из REPORT.md: разметка Markdown убирается.
set -euo pipefail
export LC_ALL=C.UTF-8

name=bash-homework
here=$(cd -- "$(dirname -- "$0")" && pwd)
cd -- "$here"

for f in REPORT.md report.sh data; do
    [[ -e $f ]] || { printf 'dist.sh: не найден %s\n' "$f" >&2; exit 1; }
done

# Заголовок и строка из "=" или "-" той же длины.
# Длину считает bash: mawk в Ubuntu считает байты, и под кириллицей черта выходит вдвое длиннее.
underline() {
    local title=$1 ch=$2 line
    printf -v line '%*s' "${#title}" ''
    printf '%s\n%s\n' "$title" "${line// /$ch}"
}

# Markdown -> простой текст:
#   "# X" / "## X"  -> X, подчёркнутый "=" / "-"
#   блок ```...```  -> строки блока с отступом в 2 пробела
#   **x**, `x`      -> x;  [текст](ссылка) -> текст (ссылка)
md_to_txt() {
    local line in_code=0 link_re='\[([^]]*)\]\(([^)]*)\)'
    while IFS= read -r line || [[ -n $line ]]; do
        if [[ $line == '```'* ]]; then
            in_code=$((1 - in_code))
        elif ((in_code)); then
            printf '  %s\n' "$line"
        elif [[ $line == '# '* ]]; then
            underline "${line#\# }" '='
        elif [[ $line =~ ^#{2,}\ (.*)$ ]]; then
            underline "${BASH_REMATCH[1]}" '-'
        else
            line=${line//\*\*/}
            line=${line//\`/}
            while [[ $line =~ $link_re ]]; do
                line=${line/"${BASH_REMATCH[0]}"/"${BASH_REMATCH[1]} (${BASH_REMATCH[2]})"}
            done
            printf '%s\n' "$line"
        fi
    done < "$1"
}

stage=$(mktemp -d)
cleanup() { rm -r -- "$stage"; }
trap cleanup EXIT

mkdir -p -- "$stage/$name"
md_to_txt REPORT.md > "$stage/$name/$name.txt"
cp -- report.sh "$stage/$name/"
cp -R -- data "$stage/$name/"
# Права выставляем явно: с диска Windows файлы приходят как 777.
# chmod -R u=rwX тут не годится: X оставляет x у файлов, где он уже был.
find "$stage/$name" -type d -exec chmod 755 {} +
find "$stage/$name" -type f -exec chmod 644 {} +
chmod 755 "$stage/$name/report.sh"

tar -czf "$name.tar.gz" -C "$stage" --owner=0 --group=0 --sort=name "$name"
printf 'Готово: %s\n' "$name.tar.gz"
tar -tzf "$name.tar.gz"
