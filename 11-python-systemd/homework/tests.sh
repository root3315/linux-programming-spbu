#!/usr/bin/env bash
# Проверки nightcheck.py без systemd: свой маленький сайт, каталог и разные исходы.
#   ./tests.sh
set -uo pipefail

cd "$(dirname "$0")"
work=$(mktemp -d)
trap 'kill "$server" 2>/dev/null; rm -rf "$work"' EXIT

mkdir -p "$work/site" "$work/data"
echo hello > "$work/site/index.html"
head -c $((3 * 1024 * 1024)) /dev/zero > "$work/data/big.bin"

port=18765
python3 -m http.server "$port" --bind 127.0.0.1 --directory "$work/site" >/dev/null 2>&1 &
server=$!
for _ in $(seq 50); do
    python3 -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:$port/')" 2>/dev/null && break
    sleep 0.1
done

site="http://127.0.0.1:$port/"
passed=0 failed=0

# expect <код> <название> <аргументы nightcheck.py...>
expect() {
    local want=$1 name=$2; shift 2
    python3 nightcheck.py --report "$work/report.json" "$@" >"$work/out" 2>&1
    local got=$?
    if [[ $got == "$want" ]]; then
        passed=$((passed + 1)); echo "ok    $name (код $got)"
    else
        failed=$((failed + 1)); echo "FAIL  $name: ждал код $want, получил $got"; cat "$work/out"
    fi
}

# report_has <питоновское условие над r> <название>
report_has() {
    if python3 -c "import json,sys; r=json.load(open('$work/report.json')); sys.exit(0 if $1 else 1)"; then
        passed=$((passed + 1)); echo "ok    $2"
    else
        failed=$((failed + 1)); echo "FAIL  $2"; cat "$work/report.json"
    fi
}

expect 0 "всё в порядке" --dir "$work/data" --max-mb 10 --url "$site"
report_has "r['ok'] and r['date'][:2] == '20' and len(r['checks']) == 2" "отчёт: ok, дата, две проверки"
report_has "r['checks'][0]['size_mb'] == 3.0 and r['checks'][1]['status'] == 200" "отчёт: 3.0 МБ и код 200"

expect 1 "каталог больше лимита" --dir "$work/data" --max-mb 1 --url "$site"
report_has "not r['ok'] and 'лимите' in r['checks'][0]['error']" "отчёт: причина про лимит"

expect 1 "нет каталога" --dir "$work/nope" --max-mb 10 --url "$site"
expect 1 "сайт ответил 404" --dir "$work/data" --max-mb 10 --url "${site}missing"
report_has "r['checks'][1]['status'] == 404" "отчёт: код 404"

expect 1 "сайт не отвечает" --dir "$work/data" --max-mb 10 --url "http://127.0.0.1:9/"
report_has "'не ответил' in r['checks'][1]['error']" "отчёт: причина «не ответил»"

expect 1 "один адрес из двух плохой" --dir "$work/data" --max-mb 10 --url "$site" --url "http://127.0.0.1:9/"
expect 2 "нет обязательного --url" --dir "$work/data" --max-mb 10

echo "пройдено: $passed, упало: $failed"
[[ $failed == 0 ]]
