# Практика 2. Makefile для проекта hello

Взял `hello/` из Chapter 04 в `~/build-lab`. Сначала гонял его родной Makefile, потом
заменил его своим: [Makefile](Makefile), это исходный Makefile из `Chapter 04/hello`, к
которому я добавил цели `run` и `release`. Отдельно починил ловушку с пробелами:
[Makefile-spaces](Makefile-spaces), исходник в `Chapter 04/broken/Makefile-spaces`.
Всё в контейнере с Ubuntu 24.04, GNU Make 4.3.

## 1. Первая сборка

```text
$ make
gcc -std=c11 -Wall -Wextra -O2 -g -c main.c -o main.o
gcc -std=c11 -Wall -Wextra -O2 -g -c greet.c -o greet.o
gcc -o hello main.o greet.o
$ ./hello "СПбГУ"
Hello, СПбГУ!
greet() вызван 1 раз
```

Три команды: два объектных файла по шаблонному правилу `%.o: %.c` (флаги из `CFLAGS`:
стандарт C11, предупреждения, `-O2`, отладочная информация `-g`) и линковка `$(CC) -o $@ $^`,
где `$@` — это `hello`, а `$^` оба `.o`.

## 2. Второй запуск

```text
$ make
make: Nothing to be done for 'all'.
```

`hello` новее обоих `.o`, а они новее своих `.c` и `greet.h`. Make сравнил время изменения
и решил, что делать нечего.

## 3. touch greet.h

Предсказание: пересоберутся оба объекта, потому что в Makefile есть `main.o: greet.h` и
`greet.o: greet.h`.

```text
$ touch greet.h && make
gcc -std=c11 -Wall -Wextra -O2 -g -c main.c -o main.o
gcc -std=c11 -Wall -Wextra -O2 -g -c greet.c -o greet.o
gcc -o hello main.o greet.o
```

Так и вышло. Для сравнения, если тронуть только `main.c`, пересобирается один объект.
Смотрел через `make -n`, он только печатает команды:

```text
$ touch main.c && make -n
gcc -std=c11 -Wall -Wextra -O2 -g -c main.c -o main.o
gcc -o hello main.o greet.o
$ make
gcc -std=c11 -Wall -Wextra -O2 -g -c main.c -o main.o
gcc -o hello main.o greet.o
```

## 4. Цель run

В исходном Makefile `run` уже был, и сразу правильный: в `.PHONY` и зависит от `$(TARGET)`.

```text
$ make run
./hello "Linux"
Hello, Linux!
greet() вызван 1 раз
```

Проверил, зачем `.PHONY`: сделал копию Makefile, где `run` из `.PHONY` убран, и создал
пустой файл с именем `run`.

```text
$ sed 's/^.PHONY: all clean run/.PHONY: all clean/' Makefile > NoPhony.mk
$ touch run && make -f NoPhony.mk run
make: 'run' is up to date.
$ make run
./hello "Linux"
Hello, Linux!
greet() вызван 1 раз
```

Без `.PHONY` make нашёл файл `run`, который новее `hello`, и решил, что цель собрана. С
`.PHONY` файл не мешает.

## 5. Цель release

На слайде шаблон `release: CFLAGS += -O3 -DNDEBUG` и `release: clean $(TARGET)`, но там
релиз пишется в тот же `hello`. В задании просили отдельное имя файла, поэтому сделал
`hello-release`, который собирается прямо из `.c` одной командой. Через общие `main.o` и
`greet.o` нельзя: они собраны с `-O2 -g`, и make не заметит, что флаги другие.

```make
RELEASE        = hello-release
RELEASE_CFLAGS = -std=c11 -Wall -Wextra -O3 -DNDEBUG

release: $(RELEASE)

$(RELEASE): main.c greet.c greet.h
	$(CC) $(RELEASE_CFLAGS) -o $@ main.c greet.c
```

```text
$ make -n release
gcc -std=c11 -Wall -Wextra -O3 -DNDEBUG -o hello-release main.c greet.c
$ make release
gcc -std=c11 -Wall -Wextra -O3 -DNDEBUG -o hello-release main.c greet.c
$ ./hello-release
Hello, Linux!
greet() вызван 1 раз
$ ls -l hello hello-release
-rwxrwxr-x 1 stepan stepan 19624 Nov 23 19:33 hello
-rwxrwxr-x 1 stepan stepan 16176 Nov 23 19:33 hello-release
```

Релиз меньше, хотя `-O3`: в нём нет отладочной информации от `-g`.

## 6. make -j2 и сборка с нуля

```text
$ make clean
rm -f main.o greet.o hello hello-release
$ make -j2
gcc -std=c11 -Wall -Wextra -O2 -g -c main.c -o main.o
gcc -std=c11 -Wall -Wextra -O2 -g -c greet.c -o greet.o
gcc -o hello main.o greet.o
$ make clean && make && ./hello
rm -f main.o greet.o hello hello-release
gcc -std=c11 -Wall -Wextra -O2 -g -c main.c -o main.o
gcc -std=c11 -Wall -Wextra -O2 -g -c greet.c -o greet.o
gcc -o hello main.o greet.o
Hello, Linux!
greet() вызван 1 раз
```

С `-j2` два объекта компилируются параллельно, но линковка всё равно идёт последней: она
ждёт обе зависимости. Критерий со слайда выполнен: `make clean && make && ./hello` работает
с нуля.

## Ловушка табуляции

```text
$ make -f Makefile-spaces
Makefile-spaces:6: *** missing separator (did you mean TAB instead of 8 spaces?).  Stop.
$ cat -A Makefile-spaces | tail -2
hello: main.c^M$
        gcc main.c -o hello^M$
```

Make 4.3 сам подсказывает, что там 8 пробелов. Заодно увидел `^M` в конце строк: в архиве
Chapter 04 все файлы с CRLF, так они там и лежат, `file` на свежераспакованных файлах это
показывает. Make это пережил, а вот autotools нет (см. практику 3).

Заменил пробелы на табуляцию:

```text
$ sed -i 's/^        /\t/' Makefile-spaces
$ cat -A Makefile-spaces | tail -1
^Igcc main.c -o hello^M$
$ cd hello
$ make -B -n -f ../Makefile-spaces
gcc main.c -o hello
```

Ошибки больше нет. Запускал с `-n`: этот Makefile собирает один `main.c` без `greet.c`, и
настоящая сборка упала бы на линковке. В исправленном файле в репо концы строк уже LF.

## Что понял

Make сравнивает только время файлов, поэтому всё, от чего зависит результат (заголовки,
флаги), должно быть видно ему как зависимость или как отдельный файл. Иначе он честно
скажет «Nothing to be done» и оставит старый бинарник.
