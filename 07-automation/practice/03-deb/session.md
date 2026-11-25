# Практика 3. Собрать .deb

Тот же `~/auto-lab`. Сначала собирал `packaging/build-deb.sh` из Chapter 07 как есть, потом
заменил его своим [build-deb.sh](build-deb.sh): версия берётся из `Makefile`. Исходник —
`project/packaging/build-deb.sh` из архива Chapter 07.

## Пакет 1.0.0

```text
$ make clean >/dev/null; rm -rf dist
$ ./packaging/build-deb.sh
собираю пакет…
dpkg-deb: building package 'hello' in 'dist/hello_1.0.0_amd64.deb'.
$ ls -l dist/
total 8
-rw-r--r-- 1 stepan stepan 4392 Nov 23 20:37 hello_1.0.0_amd64.deb
$ dpkg-deb -c dist/hello_1.0.0_amd64.deb
drwxr-xr-x root/root         0 2026-11-23 20:37 ./
drwxr-xr-x root/root         0 2026-11-23 20:37 ./usr/
drwxr-xr-x root/root         0 2026-11-23 20:37 ./usr/bin/
-rwxr-xr-x root/root     19440 2026-11-23 20:37 ./usr/bin/hello
$ dpkg-deb -I dist/hello_1.0.0_amd64.deb
 new Debian package, version 2.0.
 size 4392 bytes: control archive=450 bytes.
     495 bytes,    10 lines      control
 Package: hello
 Version: 1.0.0
 Section: utils
 Priority: optional
 Architecture: amd64
 Maintainer: СПбГУ, кафедра технологии программирования <student@spbu.ru>
 Depends: libc6 (>= 2.34)
 Description: учебная утилита приветствия
  Печатает приветствие для имени, переданного в аргументе.
  Пример пакета для практикума по автоматической сборке.
```

Пакет 4392 байта, а бинарник внутри 19440: `data.tar` сжат. Пути без ведущего `/`, владелец
`root/root`, хотя собирал обычный пользователь. Это `--root-owner-group`. Без него:

```text
$ dpkg-deb --build build/deb /tmp/noroot.deb
dpkg-deb: building package 'hello' in '/tmp/noroot.deb'.
$ dpkg-deb -c /tmp/noroot.deb | tail -1
-rwxr-xr-x stepan/stepan 19440 2026-11-23 20:39 ./usr/bin/hello
```

После установки такого пакета `/usr/bin/hello` принадлежал бы пользователю с моим UID.

## Запуск без установки

```text
$ dpkg-deb -x dist/hello_1.0.0_amd64.deb /tmp/pkg
$ /tmp/pkg/usr/bin/hello "пакет"
Hello, пакет!
$ find /tmp/pkg
/tmp/pkg
/tmp/pkg/usr
/tmp/pkg/usr/bin
/tmp/pkg/usr/bin/hello
```

`dpkg-deb -x` распаковывает только данные, без `DEBIAN/control`, и root ему не нужен.

## Версия 1.1.0

Стал искать, где менять версию:

```text
$ grep -rn '1\.0\.0' Makefile packaging/
Makefile:7:VERSION = 1.0.0
packaging/DEBIAN/control:2:Version: 1.0.0
packaging/build-deb.sh:5:#   dpkg-deb -c dist/hello_1.0.0_amd64.deb    # посмотреть содержимое
packaging/build-deb.sh:11:VERSION=1.0.0
```

Три места. `VERSION` в `Makefile` никто не читает, `Version:` в `control` скрипт всё равно
перезаписывает через `sed`, а настоящая версия пакета зашита в `build-deb.sh`. Поменяй я её
только в `Makefile`, пакет остался бы 1.0.0. Переделал скрипт: версию он берёт из строки
`VERSION = ...` в `Makefile`, архитектуру из `dpkg --print-architecture`. Если строки нет,
выходит с ошибкой, а не собирает пакет с пустой версией. Положил его на место
`packaging/build-deb.sh`:

```text
$ git diff --stat
 packaging/build-deb.sh | 26 +++++++++++++++-----------
 1 file changed, 15 insertions(+), 11 deletions(-)
$ sed -i 's/^VERSION = 1.0.0/VERSION = 1.1.0/' Makefile
$ ./packaging/build-deb.sh
собираю пакет 1.1.0…
dpkg-deb: building package 'hello' in 'dist/hello_1.1.0_amd64.deb'.
$ ls -l dist/
total 16
-rw-r--r-- 1 stepan stepan 4392 Nov 23 20:37 hello_1.0.0_amd64.deb
-rw-r--r-- 1 stepan stepan 4384 Nov 23 20:39 hello_1.1.0_amd64.deb
$ dpkg-deb -f dist/hello_1.1.0_amd64.deb Version Architecture
Version: 1.1.0
Architecture: amd64
```

## Чем отличаются два пакета

Разница 8 байт. Разобрал оба:

```text
$ dpkg-deb -R dist/hello_1.0.0_amd64.deb /tmp/p1
$ dpkg-deb -R dist/hello_1.1.0_amd64.deb /tmp/p2
$ cmp /tmp/p1/usr/bin/hello /tmp/p2/usr/bin/hello && echo "бинарники одинаковые"
бинарники одинаковые
$ diff /tmp/p1/DEBIAN/control /tmp/p2/DEBIAN/control
2c2
< Version: 1.0.0
---
> Version: 1.1.0
$ for f in dist/*.deb; do echo "$f"; ar tv $f; done
dist/hello_1.0.0_amd64.deb
rw-r--r-- 0/0      4 Nov 23 20:37 2026 debian-binary
rw-r--r-- 0/0    450 Nov 23 20:37 2026 control.tar.zst
rw-r--r-- 0/0   3750 Nov 23 20:37 2026 data.tar.zst
dist/hello_1.1.0_amd64.deb
rw-r--r-- 0/0      4 Nov 23 20:39 2026 debian-binary
rw-r--r-- 0/0    449 Nov 23 20:39 2026 control.tar.zst
rw-r--r-- 0/0   3742 Nov 23 20:39 2026 data.tar.zst
```

Программа внутри та же, байт в байт: `make` в скрипте ничего не пересобрал. Отличается строка
`Version:` и даты файлов внутри tar, от этого чуть иначе сжались `control.tar.zst` и
`data.tar.zst`. Сам `.deb` — это `ar`-архив из трёх частей, на слайде показана только
распакованная структура.

И главное: пакет называется 1.1.0, а программа о версии ничего не знает.

```text
$ ./hello --version
Hello, --version!
```

Это и есть задача ДЗ: передать `VERSION` из `Makefile` в код.

## Что понял

Пакет проверяется без установки: `-c` для путей и владельцев, `-I` для заголовка, `-x` чтобы
запустить. Версия должна жить в одном месте, а остальные её читают. В архиве их было три, и
работала только одна, причём не та, которую естественно править первой.
