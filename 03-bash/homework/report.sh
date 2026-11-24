#!/usr/bin/env bash
# Сводка по журналам: сколько строк каждого уровня во всех *.log каталога.
# Формат строки журнала: "ВРЕМЯ УРОВЕНЬ сообщение", уровень во втором поле.
# Использование: ./report.sh DIRECTORY [OUTPUT]   (OUTPUT по умолчанию summary.txt)
set -euo pipefail

prog=$(basename "$0")

usage() {
    printf 'Usage: %s DIRECTORY [OUTPUT]\n' "$prog" >&2
    printf 'Ищет *.log в DIRECTORY (с подкаталогами), сводку пишет в OUTPUT (summary.txt).\n' >&2
}

die() {
    printf '%s: ошибка: %s\n' "$prog" "$1" >&2
    exit 1
}

# Печатает для одного файла: ERROR WARN INFO OTHER
count_levels() {
    awk '
        $2 == "ERROR" { e++; next }
        $2 == "WARN"  { w++; next }
        $2 == "INFO"  { i++; next }
        NF > 0        { o++ }
        END { printf "%d %d %d %d\n", e, w, i, o }
    ' < "$1"
}

check_args() {
    if [[ $# -lt 1 || $# -gt 2 ]]; then
        usage
        exit 2
    fi
    local dir=$1 out_dir
    [[ -e $dir ]] || die "каталог не найден: $dir"
    [[ -d $dir ]] || die "это не каталог: $dir"
    [[ -r $dir && -x $dir ]] || die "нет прав на чтение каталога: $dir"
    [[ ! -d ${2:-summary.txt} ]] || die "OUTPUT это каталог, нужен файл: ${2:-summary.txt}"
    out_dir=$(dirname -- "${2:-summary.txt}")
    [[ -d $out_dir && -w $out_dir ]] || die "нельзя записать сводку в каталог: $out_dir"
}

write_summary() {
    local dir=$1
    shift
    local file counts e w i o
    local total_e=0 total_w=0 total_i=0 total_o=0

    printf 'Сводка по журналам в каталоге %s\n' "$dir"
    printf 'Файлов *.log: %d\n\n' "$#"
    printf '%-30s %6s %6s %6s %6s\n' FILE ERROR WARN INFO OTHER
    for file in "$@"; do
        [[ -r $file ]] || die "нет прав на чтение файла: $file"
        counts=$(count_levels "$file")
        read -r e w i o <<< "$counts"
        printf '%-30s %6d %6d %6d %6d\n' "${file#"$dir"/}" "$e" "$w" "$i" "$o"
        total_e=$((total_e + e))
        total_w=$((total_w + w))
        total_i=$((total_i + i))
        total_o=$((total_o + o))
    done
    printf '%-30s %6d %6d %6d %6d\n' TOTAL "$total_e" "$total_w" "$total_i" "$total_o"
}

main() {
    check_args "$@"
    local dir=${1%/} out=${2:-summary.txt}
    [[ -n $dir ]] || dir=/

    local -a logs=()
    mapfile -d '' -t logs < <(find "$dir" -type f -name '*.log' -print0 | sort -z)
    (( ${#logs[@]} > 0 )) || die "в каталоге $dir нет файлов *.log"

    # Пишем во временный файл: если что-то упадёт, старая сводка останется целой.
    tmp="$out.tmp.$$"
    trap 'rm -f -- "$tmp"' EXIT
    write_summary "$dir" "${logs[@]}" > "$tmp"
    mv -- "$tmp" "$out"
    printf 'Сводка записана в %s (файлов: %d)\n' "$out" "${#logs[@]}"
}

main "$@"
