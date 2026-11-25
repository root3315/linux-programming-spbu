# ДЗ 4. Сборка проекта через Make и CMake

## Условие

Взять `hello` из Chapter 04 и:

1. Добавить третий модуль `stats.c/.h`.
2. Обновить `Makefile`: новая зависимость, цель `release`, `.PHONY`.
3. Обновить `CMakeLists.txt`, чтобы проект собирался и через CMake.
4. Включить `-Werror` и добиться сборки без предупреждений.
5. Собрать с `-fsanitize=address,undefined` и приложить вывод.
6. Описать в `report.txt`, что пересобирается после `touch greet.h`.

Сдаётся архив `build-homework.tar.gz` с проектом и `report.txt`. Структура отчёта со
слайда: команды сборки, вывод make и cmake, что пересобралось и почему, отчёт санитайзера
(или его отсутствие), что было непонятно.

## Критерии

Базовый: обе системы сборки работают с нуля, программа запускается с аргументом и без.
Выполнен: `make` и `cmake -S . -B build && cmake --build build` собирают из чистого
каталога без единого предупреждения, `./hello`, `./hello Linux`, `./hello ""` работают.

Продвинутый: есть цель `check`, которая прогоняет санитайзеры и возвращает ненулевой код
при ошибке. Выполнен: `make check` собирает отдельную копию с ASan и UBSan в `build-san/` и
запускает программу четыре раза. На чистом коде код выхода 0, с подброшенной ошибкой 2.
Проверка с временно сломанным `stats.c` есть в отчёте, раздел 4. В CMake то же через
`-DHELLO_SANITIZE=ON` и `ctest`.

## Что где лежит

| Файл | Что это |
|---|---|
| `main.c` | точка входа, без аргумента приветствует "Linux" |
| `greet.c`, `greet.h` | модуль приветствия из Chapter 04, код без изменений (концы строк переведены в LF) |
| `stats.c`, `stats.h` | новый модуль: длина имени, латинские буквы, цифры, остальное |
| [Makefile](Makefile) | цели `all`, `run`, `release`, `check`, `clean` |
| [CMakeLists.txt](CMakeLists.txt) | библиотеки `greet` и `stats`, программа `hello`, тесты для `ctest` |
| [meson.build](meson.build) | то же для Meson, сверх задания |
| [REPORT.md](REPORT.md) | отчёт |
| [dist.sh](dist.sh) | делает `report.txt` из `REPORT.md` и собирает архив |
| `build-homework.tar.gz` | архив для сдачи |

## Как собрать

В контейнере с Ubuntu 24.04:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential cmake meson
```

```bash
make && ./hello && ./hello "Linux"
make check                     # ASan + UBSan, код выхода 0, если ошибок нет
make release                   # пересборка с -O2 -DNDEBUG
make clean

cmake -S . -B build && cmake --build build && ctest --test-dir build
cmake -S . -B build-asan -DHELLO_SANITIZE=ON && cmake --build build-asan && ctest --test-dir build-asan

meson setup builddir && meson compile -C builddir && ./builddir/hello
```

## Как сделать архив

```bash
./dist.sh
```

Скрипт переводит `REPORT.md` в простой текст `report.txt` (заголовки подчёркнуты `===` и
`---`, код с отступом в два пробела) и пакует `build-homework/` с исходниками и
`report.txt`. Архив я проверил в чистом `ubuntu:24.04`: распаковал, поставил пакеты выше,
`make`, `make check`, CMake с `ctest` и Meson собрались и отработали.
