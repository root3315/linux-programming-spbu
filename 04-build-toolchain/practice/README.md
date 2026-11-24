# Практики занятия 4

Все примеры из архива Chapter 04 преподавателя (`hello/`, `broken/`, `stages/`,
`hello-autotools/`). Работал как на слайдах: копировал нужный каталог в `~/build-lab` и
собирал там, исходники архива не трогал. В папках ниже только моё: сессии с командами и
выводом и те файлы, которые я написал или исправил.

Запускал в контейнере с Ubuntu 24.04. Поставить всё нужное:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential cmake meson ninja-build autoconf automake \
    gdb cppcheck file
```

Для gdb в практике 4 контейнер нужно запускать с `--cap-add SYS_PTRACE`.

## Практика 1. Четыре этапа вручную

На `stages/demo.c` пройти сборку по шагам: `gcc -E` (препроцессор), `gcc -S` (компилятор),
`gcc -c` (ассемблер), `gcc demo.o -o demo` (линковщик). Перед каждым шагом предсказать,
что появится. Найти в `demo.i` раскрытые `SQUARE(7)` и `VERSION`, в `demo.s` функцию `main`
и `.rodata`, в `nm demo.o` символы `T` и `U`. Собрать то же одной командой и сравнить
размеры. Критерий: объяснить, почему `demo.o` нельзя запустить и что добавил линковщик.

Решение: [01-stages/session.md](01-stages/session.md). Своих файлов нет, только команды.

## Практика 2. Makefile для проекта

Разобрать Makefile из `hello/`: что делает каждая строка вывода `make`, почему второй
`make` ничего не делает, что пересобирается после `touch greet.h`. Добавить цель `run`
(запуск `./hello "Linux"`) и цель `release` с `-O3 -DNDEBUG` и отдельным именем файла.
Проверить `make -j2`. Критерий: `make clean && make && ./hello` работает с нуля, `make -n`
показывает ожидаемые команды.

Решение: [02-makefile/](02-makefile/): мой [Makefile](02-makefile/Makefile), исправленный
[Makefile-spaces](02-makefile/Makefile-spaces) и [session.md](02-makefile/session.md).

```bash
cp -r /path/to/Chapter04/hello . && cp 02-makefile/Makefile hello/
cd hello && make && make run && make release && ./hello-release
```

## Практика 3. Один проект, четыре сборки

Собрать `hello/` через make, CMake (`cmake -S . -B build`, `cmake --build build`) и Meson
(`meson setup builddir`, `meson compile -C builddir`), а `hello-autotools/` через
`autoreconf --install && ./configure && make`. Перед каждым способом убирать следы
предыдущего, после правки `greet.c` посмотреть, что пересобралось. В конце удалить все
каталоги сборки и собрать заново одной командой. Плюс ответить на шесть вопросов слайда.

Решение: [03-four-builds/session.md](03-four-builds/session.md). Своих файлов нет.

## Практика 4. Найти ошибку

В `broken/memory.c` выход за границу массива. Собрать обычно и запустить, потом с
`-fsanitize=address -g`, прочитать отчёт (строка, размер, граница), исправить цикл и
убедиться, что ASan молчит. Прогнать `cppcheck` по `hello/`, в gdb посмотреть `i` перед
ошибочной записью. Критерий: объяснить разницу между `i <= 4` и `i < 4` и почему без
санитайзера ошибку не видно. Остальные файлы из `broken/` разобрал там же.

Решение: [04-find-bug/session.md](04-find-bug/session.md), исправленные файлы в
[04-find-bug/fixed/](04-find-bug/fixed/).

```bash
gcc -fsanitize=address,undefined -g -O1 04-find-bug/fixed/memory.c -o memory && ./memory
```
