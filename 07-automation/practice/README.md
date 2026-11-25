# Практики занятия 7

Все четыре практики на проекте `hello` из архива Chapter 07 преподавателя: `Makefile`,
`src/`, `include/`, тест `tests/test_greet.c`, `ci.sh`, `packaging/` и `profile/hot.c`. Как на
слайдах, копировал проект в `~/auto-lab` и заводил там репозиторий заново. В папках ниже
только моё: сессии с командами и выводом и пара своих файлов.

Работал в контейнере с Ubuntu 24.04 под обычным пользователем: git 2.43.0, gcc 13.3.0,
dpkg-deb 1.22.6, gprof 2.42. Подготовка:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential git file clang
tar -xzf chapter07.tar.gz
mkdir -p ~/auto-lab && cp -r Chapter07/project/. ~/auto-lab/ && cd ~/auto-lab
find . -type f -exec sed -i 's/\r$//' {} +       # в архиве CRLF, см. практику 1
chmod +x ci.sh packaging/build-deb.sh            # и нет бита выполнения
```

`clang` нужен только для `./ci.sh clang`, `file` — чтобы увидеть CRLF.

## Практика 1. Репозиторий проекта

`git init -b main`, имя и почта, `git status --short` до первого коммита. Собрать `make` и
убедиться, что артефактов в `status` нет. Два коммита: «скелет проекта» и «модуль greet».
Ветка `feature/test` с тестом, слить в `main`. Специально устроить конфликт и разрешить.
Критерий: `status` чистый, в истории нет `hello` и `*.o`, `git log --graph` показывает слияние.

Решение: [01-git-repo/session.md](01-git-repo/session.md). Своих файлов нет. Сверх задания
`git bisect run`, и на нём нашлась моя же поломка, которую `make test` не видит.

## Практика 2. Сломать и починить CI

`./ci.sh` зелёный, потом сломать тест (`greet()` возвращает `Hi`) и посмотреть, какой шаг
упал и с каким кодом. Починить. Убрать `set -e` и повторить. Сломать синтаксис и сравнить с
поломкой теста. Критерий: объяснить разницу между кодом возврата теста и кодом возврата
скрипта и почему CI смотрит на второй.

Решение: [02-break-ci/session.md](02-break-ci/session.md). Все правки временные, файлов нет.
Там же `.gitignore` пополнился `build/`, а `./ci.sh clang` заменил матрицу.

```bash
cd ~/auto-lab && ./ci.sh; echo $?
```

## Практика 3. Собрать .deb

`./packaging/build-deb.sh`, `dpkg-deb -c` и `-I`, распаковать `dpkg-deb -x` в `/tmp/pkg` и
запустить оттуда. Поменять версию на 1.1.0, собрать снова, сравнить `ls -l` двух пакетов.
Критерий: содержимое как ожидалось, владелец `root`, версия в имени файла и в `control`
совпадает.

Решение: [03-deb/](03-deb/): [session.md](03-deb/session.md) и
[build-deb.sh](03-deb/build-deb.sh), правка скрипта из архива: версия берётся из `Makefile`,
архитектура из `dpkg`.

```bash
cp /path/to/03-deb/build-deb.sh ~/auto-lab/packaging/ && cd ~/auto-lab
./packaging/build-deb.sh && dpkg-deb -c dist/*.deb && dpkg-deb -I dist/*.deb
```

## Практика 4. Найти горячую функцию

`profile/hot.c`: сначала `time ./hot 34`, потом `-O0 -pg` и `gprof`. Какая функция первая и
сколько у неё вызовов. Убрать цикл вокруг `sum_squares`, пересобрать с `-O2` без `-pg`,
сравнить время. Критерий: назвать горячую функцию по профилю и объяснить, что дало больше,
алгоритм или флаг.

Решение: [04-gprof/](04-gprof/): [session.md](04-gprof/session.md) и
[hot-fixed.c](04-gprof/hot-fixed.c). У меня вышло не как на слайде, `-O2` дал больше правки,
разбор в сессии.

```bash
cd ~/auto-lab/profile
gcc -O0 -pg -o hot hot.c && ./hot 34 && gprof -b ./hot gmon.out | head -12
gcc -O2 -o hot-fixed /path/to/04-gprof/hot-fixed.c && time ./hot-fixed 34
```
