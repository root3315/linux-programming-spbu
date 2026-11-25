#!/bin/sh
# build-deb.sh — собрать .deb из уже собранного проекта.
# Правка build-deb.sh из Chapter 07: версия берётся из Makefile,
# а архитектура из dpkg, чтобы не менять их в трёх местах.
#
#   ./packaging/build-deb.sh
#   dpkg-deb -c dist/hello_*.deb    # посмотреть содержимое
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

VERSION=$(sed -n 's/^VERSION *= *//p' Makefile)
ARCH=$(dpkg --print-architecture)
NAME=hello
STAGE=build/deb          # относительный путь: пробелы в пути проекта ломают make

if [ -z "$VERSION" ]; then
    echo "build-deb.sh: в Makefile нет строки VERSION = ..." >&2
    exit 1
fi

make >/dev/null
rm -rf "$STAGE"
make install DESTDIR="$STAGE" PREFIX=/usr >/dev/null

mkdir -p "$STAGE/DEBIAN"
cp packaging/DEBIAN/control "$STAGE/DEBIAN/control"
sed -i "s/^Version:.*/Version: $VERSION/" "$STAGE/DEBIAN/control"
sed -i "s/^Architecture:.*/Architecture: $ARCH/" "$STAGE/DEBIAN/control"

mkdir -p dist
echo "собираю пакет $VERSION…"
dpkg-deb --root-owner-group --build "$STAGE" "dist/${NAME}_${VERSION}_${ARCH}.deb"
