# Отчёт по ДЗ 7. Автоматическая сборка

Проект `hello` из ДЗ 4 (main, greet, stats) переехал в [hello/](hello/) и получил версию в
`--version`, тест, пакет, `ci.sh` и workflow для GitHub Actions. Для профиля в `stats`
добавил подсчёт разных байт и режим `--bench N`. Профиль снимал в контейнере с
Ubuntu 24.04, `ci.sh` и пакет собирал в Ubuntu 24.04 под WSL: gcc 13.3.0, dpkg-deb 1.22.6,
gprof 2.42.

## 1. git log --oneline --graph

История ДЗ в репозитории курса, только коммиты, которые трогают `07-automation/homework/hello`:

```text
$ git log --oneline --graph --decorate -- 07-automation/homework/hello
* 44de77f (tag: v1.1.0) perf(lec07/hw): count_distinct за один проход вместо O(n^2)
*   4675f35 Merge branch 'feature/version' into feat/19-lec07-automation
|\  
| * 47e2e73 feat(lec07/hw): версия из Makefile в --version, 1.1.0
* | f27f643 fix(lec07/hw): подсказка --help по-русски
|/  
* f21db9b feat(lec07/hw): проект hello с тестом, пакетом и ci.sh
```

## 2. Вывод ./ci.sh

```text
$ ./ci.sh
=== компилятор: gcc ===
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
=== чистая сборка ===
rm -f hello main.o greet.o stats.o tests/test_hello
rm -rf build dist build-prof
gcc -DVERSION=\"1.1.0\" -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -c main.c -o main.o
gcc -DVERSION=\"1.1.0\" -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -c greet.c -o greet.o
gcc -DVERSION=\"1.1.0\" -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -c stats.c -o stats.o
gcc -o hello main.o greet.o stats.o
=== тесты ===
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -I. -o tests/test_hello tests/test_hello.c greet.o stats.o
./tests/test_hello
ok:   greet: обычное имя
ok:   greet: пустая строка
ok:   greet: счётчик вызовов
ok:   stats: буквы, цифры, прочие
ok:   stats: разные байты в "Linux 2026"
ok:   stats: кириллица
ok:   stats: NULL

все проверки прошли
ok:   --version: hello 1.1.0
=== пакет ===
rm -rf build/deb
make --no-print-directory install DESTDIR=build/deb PREFIX=/usr
install -d build/deb/usr/bin
install -m 0755 hello build/deb/usr/bin/hello
sh packaging/build-deb.sh build/deb 1.1.0
dpkg-deb: building package 'hello' in 'dist/hello_1.1.0_amd64.deb'.
=== проверка пакета ===
Package: hello
Version: 1.1.0
Architecture: amd64
Depends: libc6 (>= 2.34)
Hello, пакет!
длина 10: букв 0, цифр 0, прочих 10, разных байт 7
greet() вызван 1 раз
версия совпадает: hello 1.1.0
=== готово ===
total 12
-rw-r--r-- 1 stepan stepan 9030 Nov 25 12:45 hello_1.1.0_amd64.deb
$ echo $?
0
```

Версия записана один раз, `VERSION = 1.1.0` в `Makefile`. Оттуда она уходит в код
(`-DVERSION=\"1.1.0\"`), в `control` и в имя пакета. `make test` сверяет вывод `--version` с
`Makefile`, а `ci.sh` в конце распаковывает готовый `.deb` и сверяет `Version:` из `control`
с тем, что печатает программа из пакета.

Проверка, что CI ловит поломку: в `greet.c` поменял `Hello` на `Hi`.

```text
$ ./ci.sh > red.log 2>&1; echo "код ci.sh: $?"
код ci.sh: 2
$ sed -n '/=== тесты/,$p' red.log
=== тесты ===
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -I. -o tests/test_hello tests/test_hello.c greet.o stats.o
./tests/test_hello
FAIL: greet: обычное имя
FAIL: greet: пустая строка
ok:   greet: счётчик вызовов
ok:   stats: буквы, цифры, прочие
ok:   stats: разные байты в "Linux 2026"
ok:   stats: кириллица
ok:   stats: NULL

2 проверок не прошло
make: *** [Makefile:49: test] Error 1
$ ls dist
ls: cannot access 'dist': No such file or directory
```

Пакета нет вообще: `make clean` удаляет и `dist/`, так что старый `.deb` от прошлого прогона не
выдаст себя за новый. В практике 2 с архивом преподавателя было именно так.

Тот же `ci.sh` запускает GitHub Actions: workflow `.github/workflows/ci.yml` в корне
репозитория срабатывает на push и pull request, если менялись `07-automation/homework/hello/`
или сам workflow. Шаги: checkout, версии инструментов, `cd 07-automation/homework/hello &&
./ci.sh`, загрузка `dist/*.deb` как артефакта `hello-deb`.
Прогон на коммите с тегом зелёный: [run 45570133867](https://github.com/root3315/linux-programming-spbu/actions/runs/45570133867).

## 3. dpkg-deb -c и -I

```text
$ dpkg-deb -c dist/hello_1.1.0_amd64.deb
drwxr-xr-x root/root         0 2026-11-25 12:45 ./
drwxr-xr-x root/root         0 2026-11-25 12:45 ./usr/
drwxr-xr-x root/root         0 2026-11-25 12:45 ./usr/bin/
-rwxr-xr-x root/root     26984 2026-11-25 12:45 ./usr/bin/hello
$ dpkg-deb -I dist/hello_1.1.0_amd64.deb
 new Debian package, version 2.0.
 size 9030 bytes: control archive=425 bytes.
     429 bytes,    10 lines      control
 Package: hello
 Version: 1.1.0
 Section: utils
 Priority: optional
 Architecture: amd64
 Maintainer: Stepan <root3315@users.noreply.github.com>
 Depends: libc6 (>= 2.34)
 Description: учебная утилита приветствия
  Печатает приветствие и статистику по имени из аргумента.
  Домашнее задание 7 курса «Программирование в Linux».
```

Владелец `root/root` даёт `dpkg-deb --root-owner-group`, собирал обычный пользователь.
Версия 1.1.0 одна и та же в имени файла, в `control` и в `hello --version`. Архитектуру
`build-deb.sh` берёт из `dpkg --print-architecture`, а не пишет руками.

Нижнюю границу в `Depends` проверил по самому бинарнику, а не взял со слайда:

```text
$ objdump -T hello | grep -o "GLIBC_[0-9.]*" | sort -uV
GLIBC_2.2.5
GLIBC_2.3
GLIBC_2.3.4
GLIBC_2.4
GLIBC_2.34
```

Самая новая версия символов, которую требует программа, GLIBC_2.34. Значит, `libc6 (>= 2.34)`.

## 4. gprof: горячая функция

Нагрузка — `./hello --bench N`: строка из N байт (повторяю `"Stepan 2026, Привет! "`) и
`stats_collect` по ней. `make profile` собирает отдельную копию с `-pg` в `build-prof/`,
запускает `--bench 50000` и печатает плоский профиль.

### До

Подсчёт разных байт я сначала написал в лоб: для каждого байта прохожу все предыдущие и
смотрю, был ли такой.

```c
for (size_t i = 0; i < len; i++) {
    size_t seen = 0;

    for (size_t j = 0; j < i; j++)
        if (s[j] == s[i])
            seen++;
    if (seen == 0)
        distinct++;
}
```

```text
$ make profile
rm -rf build-prof
mkdir -p build-prof
gcc -DVERSION=\"1.1.0\" -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -pg -fno-inline -o build-prof/hello main.c greet.c stats.c
cd build-prof && ./hello --bench 50000
длина 50000: букв 11112, цифр 7408, прочих 31480, разных байт 20
cd build-prof && gprof -b -p hello gmon.out > flat.txt
head -n 10 build-prof/flat.txt
Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total
 time   seconds   seconds    calls   s/call   s/call  name
100.00      1.11     1.11        1     1.11     1.11  count_distinct
  0.00      1.11     0.00        1     0.00     0.00  bench_text
  0.00      1.11     0.00        1     0.00     1.11  stats_collect
  0.00      1.11     0.00        1     0.00     0.00  stats_print
```

Горячая функция `count_distinct`: один вызов и всё время. Она квадратичная, на 50 000 байт это
около 1.25 млрд сравнений. Сам проход `stats_collect` по строке до одного отсчёта в 10 мс не
дотянул. Время обычной сборки (`-O2`, без `-pg`), `TIMEFORMAT="real %R s"`:

```text
$ for i in 1 2 3; do time ./hello --bench 50000 >/dev/null; done
real 1.002 s
real 1.022 s
real 1.014 s
```

### Что сделал

Разных значений байта всего 256, поэтому хватает таблицы «уже видел» и одного прохода:

```c
unsigned char seen[UCHAR_MAX + 1] = {0};
size_t distinct = 0;

for (size_t i = 0; i < len; i++) {
    if (!seen[s[i]]) {
        seen[s[i]] = 1;
        distinct++;
    }
}
```

### После

Результат тот же, тест зелёный:

```text
$ ./hello --bench 50000 > after.txt; diff before.txt after.txt && echo "вывод тот же"
вывод тот же
$ for i in 1 2 3; do time ./hello --bench 50000 >/dev/null; done
real 0.001 s
real 0.001 s
real 0.001 s
```

| `--bench 50000`, `-O2` | время |
|---|---|
| до, O(n²) | 1.00–1.02 с |
| после, O(n) | 0.001 с |

`make profile` на тех же 50 000 байт показал пустой профиль (`no time accumulated`): программа
стала быстрее шага таймера. Чтобы увидеть, где теперь время, поднял нагрузку в тысячу раз:

```text
$ make profile BENCH=50000000 | tail -9
Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total
 time   seconds   seconds    calls  ms/call  ms/call  name
 41.67      0.10     0.10        1   100.00   100.00  bench_text
 37.50      0.19     0.09        1    90.00   140.00  stats_collect
 20.83      0.24     0.05        1    50.00    50.00  count_distinct
  0.00      0.24     0.00        1     0.00     0.00  stats_print
$ time ./hello --bench 50000000
длина 50000000: букв 11111112, цифр 7407408, прочих 31481480, разных байт 20
real 0.355 s
```

50 млн байт теперь обрабатываются за треть секунды, а раньше 50 тысяч шли секунду.
`count_distinct` больше не горячая, время примерно поровну делят построение строки и сам
разбор. Дальше оптимизировать незачем: обычный запуск разбирает одно имя.

В отличие от практики 4, здесь `-O2` не помог бы: квадратичный цикл компилятор в линейный не
превратит, это смена алгоритма.

## 5. Что было непонятно

1. После смены `VERSION = 1.0.0` на `1.1.0` в `Makefile` команда `make` ответила
   `Nothing to be done for 'all'`, а `./hello --version` печатал `hello 1.0.0`. Версия
   вкомпилирована в `main.o`, а `main.o` от `Makefile` не зависел. Поймал это `make test`:
   `FAIL: --version: 'hello 1.0.0', ждал 'hello 1.1.0'`. Добавил `Makefile` в зависимости
   `main.o`. В `ci.sh` этой проблемы не было бы из-за `make clean`, а руками легко пропустить.
2. Первый профиль с `-O2 -pg` показал 100% у `stats_collect`, и `count_distinct` в нём не было
   совсем. Gcc встроил статическую функцию с одним вызовом в `stats_collect`, и gprof не может
   отличить её код. Для профиля собираю с `-fno-inline`, остальное как в обычной сборке.
3. После оптимизации `gprof` написал `no time accumulated`. Сначала решил, что профиль
   сломался, а это программа отработала быстрее 10 мс, и таймер ни разу не сработал. Помогла
   нагрузка в тысячу раз больше, через `make profile BENCH=50000000`.
