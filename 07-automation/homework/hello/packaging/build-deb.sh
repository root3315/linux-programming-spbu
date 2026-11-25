#!/bin/sh
# build-deb.sh — собрать .deb из каталога, куда уже сделан make install.
# Вызывается из make deb, версия приходит из Makefile:
#
#   sh packaging/build-deb.sh build/deb 1.1.0
#
# Пишет dist/hello_<версия>_<архитектура>.deb.
set -eu

if [ $# -ne 2 ]; then
    echo "usage: build-deb.sh STAGE VERSION" >&2
    exit 2
fi

STAGE=$1
VERSION=$2
ARCH=$(dpkg --print-architecture)
HERE=$(dirname "$0")

mkdir -p "$STAGE/DEBIAN" dist
sed -e "s/@VERSION@/$VERSION/" -e "s/@ARCH@/$ARCH/" \
    "$HERE/control" > "$STAGE/DEBIAN/control"

# --root-owner-group: владелец файлов в пакете root, даже если собирает
# обычный пользователь.
dpkg-deb --root-owner-group --build "$STAGE" "dist/hello_${VERSION}_${ARCH}.deb"
