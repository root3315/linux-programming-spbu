# Практика 2. Сломать и починить CI

Тот же `~/auto-lab` после практики 1, `ci.sh` из Chapter 07 без изменений (только CRLF
убран). Своих файлов здесь нет, все правки временные и откатывались через `git checkout`.

## Сначала красный, хотя ничего не ломал

Первый же `./ci.sh` упал:

```text
$ ./ci.sh
=== компилятор: gcc ===
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
=== чистая сборка ===
rm -f hello src/*.o tests/test_greet
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/main.c -o src/main.o
src/main.c:2:1: warning: "/*" within comment [-Wcomment]
    2 | /* Аргумент необязателен. */
      |  
src/main.c:3:13: error: expected ‘=’, ‘,’, ‘;’, ‘asm’ or ‘__attribute__’ before ‘командной’
    3 |  * Аргумент командной строки — имя, с которым здороваемся.
      |             ^~~~~~~~~
...
make: *** [Makefile:20: src/main.o] Error 1
$ echo $?
2
```

Это мой коммит «комментарий в main» из практики 1: вставил комментарий внутрь другого
комментария. `make test` его не заметил, потому что `main.c` не собирает, а `ci.sh` начинает
с `make clean` и собирает всё. Откатил:

```text
$ git revert --no-edit 418e23e
[main 26eb939] Revert "комментарий в main"
```

## Зелёный прогон

```text
$ ./ci.sh
=== компилятор: gcc ===
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
=== чистая сборка ===
rm -f hello src/*.o tests/test_greet
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/main.c -o src/main.o
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/greet.c -o src/greet.o
gcc -o hello src/main.o src/greet.o
=== тесты ===
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -o tests/test_greet tests/test_greet.c src/greet.o
./tests/test_greet
ok:   обычное имя
ok:   NULL заменяется на world
ok:   пустая строка заменяется
ok:   счётчик вызовов

все проверки прошли
=== пакет ===
собираю пакет…
dpkg-deb: building package 'hello' in 'dist/hello_1.0.0_amd64.deb'.
=== готово ===
dist/hello_1.0.0_amd64.deb
$ echo $?
0
$ git status --short
?? build/
```

После зелёного прогона в `status` вылез `build/`. Это каталог, где `build-deb.sh` раскладывает
будущий пакет (`build/deb/usr/bin/hello`), а в `.gitignore` из архива его нет. `dist/` не видно
только потому, что в нём один `.deb`, а `*.deb` в списке есть. Дописал:

```text
$ echo "build/" >> .gitignore
$ git status --short
 M .gitignore
$ git commit -qam "игнорировать build/"
$ git log --oneline -1
fcd8978 игнорировать build/
```

## Сломанный тест

В `greet()` вернул `Hi` вместо `Hello`:

```text
$ sed -i 's/"Hello, %s!"/"Hi, %s!"/' src/greet.c
$ git diff --stat
 src/greet.c | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)
$ ls -l dist
total 8
-rw-r--r-- 1 stepan stepan 4394 Nov 23 20:33 hello_1.0.0_amd64.deb
$ ./ci.sh
=== компилятор: gcc ===
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
=== чистая сборка ===
rm -f hello src/*.o tests/test_greet
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/main.c -o src/main.o
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/greet.c -o src/greet.o
gcc -o hello src/main.o src/greet.o
=== тесты ===
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -o tests/test_greet tests/test_greet.c src/greet.o
./tests/test_greet
FAIL: обычное имя
FAIL: NULL заменяется на world
FAIL: пустая строка заменяется
ok:   счётчик вызовов

3 проверок не прошло
make: *** [Makefile:26: test] Error 1
$ echo $?
2
$ ls -l dist
total 8
-rw-r--r-- 1 stepan stepan 4394 Nov 23 20:33 hello_1.0.0_amd64.deb
```

Упал шаг «тесты», до «пакета» не дошло. На слайде два FAIL (обычное имя и счётчик), у меня
три: `NULL` и пустая строка тоже дают `Hi, world!`, а счётчик вызовов от текста не зависит и
проходит. Время у пакета в `dist/` старое, 20:33: это `.deb` от прошлого зелёного прогона.
`make clean` каталог `dist/` не чистит, и если смотреть только на «есть ли пакет», можно
принять его за свежий.

Коды возврата по цепочке:

```text
$ ./tests/test_greet >/dev/null; echo "код теста: $?"
код теста: 1
$ make test >/dev/null 2>&1; echo "код make test: $?"
код make test: 2
```

Тест возвращает 1. Make видит ненулевой код рецепта, пишет `Error 1` (это код команды) и сам
выходит с 2. `set -e` в `ci.sh` останавливает скрипт на первой упавшей команде, и скрипт
возвращает тот же код 2. CI смотрит на код всего скрипта: только он говорит, дошли ли
мы до конца.

## Без set -e

```text
$ sed -i 's/^set -eu$/set -u/' ci.sh
$ git diff ci.sh
...
-set -eu
+set -u
 
 CC_ARG=${1:-gcc}
$ rm -rf dist
$ ./ci.sh
...
=== тесты ===
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -o tests/test_greet tests/test_greet.c src/greet.o
./tests/test_greet
FAIL: обычное имя
FAIL: NULL заменяется на world
FAIL: пустая строка заменяется
ok:   счётчик вызовов

3 проверок не прошло
make: *** [Makefile:26: test] Error 1
=== пакет ===
собираю пакет…
dpkg-deb: building package 'hello' in 'dist/hello_1.0.0_amd64.deb'.
=== готово ===
dist/hello_1.0.0_amd64.deb
$ echo $?
0
$ dpkg-deb -x dist/hello_1.0.0_amd64.deb /tmp/bad && /tmp/bad/usr/bin/hello "релиз"
Hi, релиз!
```

Тесты упали, а скрипт вернул 0 и собрал пакет со сломанным кодом. Код возврата скрипта без
`set -e` — это код последней команды, здесь `ls`. В CI такой прогон был бы зелёным.

## Сломанная сборка

Вернул `set -eu` и `greet.c` и убрал точку с запятой после `calls++`:

```text
$ git checkout ci.sh src/greet.c
Updated 2 paths from the index
$ sed -i 's/    calls++;/    calls++/' src/greet.c
$ ./ci.sh
=== компилятор: gcc ===
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
=== чистая сборка ===
rm -f hello src/*.o tests/test_greet
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/main.c -o src/main.o
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/greet.c -o src/greet.o
src/greet.c: In function ‘greet’:
src/greet.c:15:12: error: expected ‘;’ before ‘snprintf’
   15 |     calls++
      |            ^
      |            ;
   16 |     snprintf(buffer, sizeof buffer, "Hello, %s!", name);
      |     ~~~~~~~~
make: *** [Makefile:20: src/greet.o] Error 1
$ echo $?
2
```

Код тот же, 2, потому что в обоих случаях падает make. Разница в шаге: тут не дошло даже до
`=== тесты ===`, и ошибку пишет компилятор, а не тест. По коду возврата их не отличить, по
логу сразу.

## Сверх задания: другой компилятор

```text
$ git checkout src/greet.c
$ ./ci.sh clang
=== компилятор: clang ===
Ubuntu clang version 18.1.3 (1ubuntu1)
=== чистая сборка ===
rm -f hello src/*.o tests/test_greet
clang -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/main.c -o src/main.o
clang -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/greet.c -o src/greet.o
clang -o hello src/main.o src/greet.o
=== тесты ===
...
все проверки прошли
=== пакет ===
собираю пакет…
dpkg-deb: building package 'hello' in 'dist/hello_1.0.0_amd64.deb'.
=== готово ===
dist/hello_1.0.0_amd64.deb
$ echo $?
0
```

Это локальный аналог `matrix: cc: [gcc, clang]` из `ci.yml`. Первый раз запустил через
`./ci.sh clang | head -6` и получил код 141: `head` закрыл трубу, и скрипт убило SIGPIPE.
Для проверки кода вывод надо не обрезать, а сохранять в файл.

## Что понял

CI верит только коду возврата. Тест обязан возвращать не 0 при ошибке, а скрипт обязан этот
код не потерять: без `set -e` упавший шаг просто печатает ошибку, и дальше всё идёт как ни в
чём не бывало. Ещё полезно перед прогоном удалять `dist/`, иначе старый пакет выглядит как
результат нового прогона.
