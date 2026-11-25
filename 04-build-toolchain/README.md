# Занятие 4. Сборка программ в Linux

Лекция-практикум о том, что происходит между исходником и запущенной программой. Сначала
разбирали четыре этапа сборки (препроцессор, компилятор, ассемблер, линковщик) и типичные
ошибки каждого, потом Make с зависимостями и инкрементальной сборкой, потом CMake, Meson и
autotools на одном и том же проекте, в конце предупреждения, санитайзеры, cppcheck и
минимум gdb. Все примеры из архива Chapter 04 преподавателя. В ДЗ нужно было расширить
проект hello третьим модулем и собрать его через Make и CMake с `-Werror` и санитайзерами.

## Что внутри

- [notes.md](notes.md): конспект лекции;
- [notes.pdf](notes.pdf): он же в PDF;
- [practice/](practice/): четыре практики, условия в [practice/README.md](practice/README.md);
- [homework/](homework/): ДЗ, условие и критерии в [homework/README.md](homework/README.md),
  отчёт в [homework/REPORT.md](homework/REPORT.md), архив `build-homework.tar.gz`.

## Как запустить

Всё проверял в контейнере с Ubuntu 24.04 (gcc 13.3.0, GNU Make 4.3, CMake 3.28.3,
Meson 1.3.2). Из папки занятия:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential cmake meson ninja-build autoconf automake \
    gdb cppcheck file
```

Дальше, например, ДЗ:

```bash
cd homework
make && ./hello "Linux" && make check
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

Для практик нужен ещё архив Chapter 04 с примерами (его в репозитории нет, это материалы
преподавателя), команды по шагам в `session.md` каждой практики. Для gdb в практике 4
контейнер запускается с `--cap-add SYS_PTRACE`. Подробнее про контейнер написано в
[CONTRIBUTING.md](../CONTRIBUTING.md).
