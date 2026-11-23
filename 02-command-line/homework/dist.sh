#!/bin/sh
# Делает report.txt из REPORT.md и собирает cli-homework.tar.gz.
set -eu

cd "$(dirname "$0")"
name=cli-homework
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
mkdir "$stage/$name"

# Длина подчёркивания считается в символах: байты-продолжения UTF-8 (0x80-0xBF) выкидываем.
awk '
function underline(s, ch) { gsub(/[\200-\277]/, "", s); gsub(/./, ch, s); return s }
/^```/ { incode = !incode; next }
incode { print "  " $0; next }
/^# / { t = substr($0, 3); print t; print underline(t, "="); next }
/^## / { t = substr($0, 4); print t; print underline(t, "-"); next }
{ gsub(/`/, ""); gsub(/\*\*/, ""); gsub(/\[|\]\([^)]*\)/, ""); print }
' REPORT.md > "$stage/$name/report.txt"

cp history.txt ls-la-project.txt "$stage/$name/"
tar czf "$name.tar.gz" -C "$stage" "$name"
echo "$name.tar.gz:"
tar tzf "$name.tar.gz"
