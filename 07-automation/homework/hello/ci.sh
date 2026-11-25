#!/bin/sh
# ci.sh — сборка, тест и пакет одной командой, с нуля.
# Те же шаги запускает .github/workflows/ci.yml в корне репозитория.
#
#   ./ci.sh          # gcc
#   ./ci.sh clang    # другой компилятор
set -eu

CC_ARG=${1:-gcc}
cd "$(dirname "$0")"

echo "=== компилятор: $CC_ARG ==="
"$CC_ARG" --version | head -n 1

echo "=== чистая сборка ==="
make clean
make CC="$CC_ARG"

echo "=== тесты ==="
make test CC="$CC_ARG"

echo "=== пакет ==="
make deb

echo "=== проверка пакета ==="
DEB=$(ls dist/*.deb)
dpkg-deb -f "$DEB" Package Version Architecture Depends
rm -rf build/check
dpkg-deb -x "$DEB" build/check
build/check/usr/bin/hello "пакет"

echo "=== готово ==="
ls -l dist/
