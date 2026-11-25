#!/usr/bin/env bash
# Собирает три образа с нуля, печатает размеры, запускает каждый
# и проверяет, что сервис корректно завершается по SIGTERM.
#   ./check.sh            сборка с кэшем
#   ./check.sh --no-cache сборка с нуля
set -euo pipefail
export MSYS_NO_PATHCONV=1   # Git Bash на Windows не должен переписывать /пути

cd "$(dirname "$0")"

NAME=hello-hw
LIMIT=$((20 * 1000 * 1000))   # критерий: финальный образ меньше 20 МБ

build() {
    local tag=$1 file=$2
    shift 2
    echo "== сборка $NAME:$tag ($file)"
    docker build -q "$@" -f "$file" -t "$NAME:$tag" . >/dev/null
}

build naive Dockerfile.naive "$@"
build multi Dockerfile "$@"
build scratch Dockerfile.scratch "$@"

echo "== размеры"
docker images "$NAME" --format '{{.Tag}}\t{{.Size}}'

for tag in naive multi scratch; do
    echo "== docker run --rm $NAME:$tag"
    docker run --rm "$NAME:$tag"
done

# docker images печатает размер строкой вида 12.2MB или 93.4kB.
to_bytes() {
    awk '{ n = $1 + 0; u = $1; sub(/^[0-9.]+/, "", u)
           m = (u == "GB") ? 1e9 : (u == "MB") ? 1e6 : (u == "kB") ? 1e3 : 1
           printf "%d\n", n * m }' <<<"$1"
}

for tag in multi scratch; do
    size=$(docker images "$NAME:$tag" --format '{{.Size}}')
    if (($(to_bytes "$size") >= LIMIT)); then
        echo "ОШИБКА: $NAME:$tag весит $size, это не меньше 20 МБ" >&2
        exit 1
    fi
done

echo "== SIGTERM: docker stop -t 10"
for tag in multi scratch; do
    docker run -d --name "$NAME-stop" "$NAME:$tag" --serve test >/dev/null
    sleep 1
    start=$(date +%s.%N)
    docker stop -t 10 "$NAME-stop" >/dev/null
    end=$(date +%s.%N)
    code=$(docker inspect "$NAME-stop" --format '{{.State.ExitCode}}')
    last=$(docker logs "$NAME-stop" | tail -1)
    docker rm "$NAME-stop" >/dev/null
    awk -v t="$tag" -v s="$start" -v e="$end" -v c="$code" -v l="$last" \
        'BEGIN { printf "%s: %.1f с, код %s, \"%s\"\n", t, e - s, c, l }'
    [[ $code == 0 ]] || { echo "ОШИБКА: код выхода $code" >&2; exit 1; }
done

echo "check: всё в порядке"
