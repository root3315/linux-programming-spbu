# Практика 4. Найти ошибку в коде

Главное задание: ошибка с памятью в `broken/memory.c` из Chapter 04, найти её тремя
способами (обычный запуск, ASan, gdb) и прогнать cppcheck. Заодно прошёл остальные файлы
из `broken/` и для каждого записал причину и исправление. Исправленные файлы лежат в
[fixed/](fixed/), исходники в `Chapter 04/broken/`, один файл мой: `dup-main.c`.
Контейнер с Ubuntu 24.04, gcc 13.3.0; для gdb запускал его с `--cap-add SYS_PTRACE`.

## memory.c

В коде блок на 4 `int`, а цикл `for (int i = 0; i <= 4; i++) data[i] = i * i;` делает пять
шагов. Последний пишет `data[4]`, это уже за концом блока.

### 1. Обычная сборка

```text
$ gcc memory.c -o memory && ./memory; echo "код выхода: $?"
last=9
код выхода: 0
$ gcc -Wall -Wextra memory.c -o memory
```

Программа «работает», и даже `-Wall -Wextra` молчат. Проверил отдельно: на 16 байт glibc
отдаёт кусок, где реально можно писать 24 (`malloc_usable_size` вернул 24). Так что
`data[4]` попал в этот запас и ничего не задел. Повезло, а не «работает».

### 2. AddressSanitizer

```text
$ gcc -fsanitize=address -g memory.c -o memory && ./memory; echo "код выхода: $?"
=================================================================
==41==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000020 at pc 0x63d98849e2d2 bp 0x7fffa68f2ab0 sp 0x7fffa68f2aa0
WRITE of size 4 at 0x502000000020 thread T0
    #0 0x63d98849e2d1 in main /home/stepan/build-lab/broken/memory.c:20
    #1 0x7e246c37f1c9  (/lib/x86_64-linux-gnu/libc.so.6+0x2a1c9) (BuildId: 8cebda42d140fc2b09639f8b13a5d5f2e617ef09)
    #2 0x7e246c37f28a in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x2a28a) (BuildId: 8cebda42d140fc2b09639f8b13a5d5f2e617ef09)
    #3 0x63d98849e184 in _start (/home/stepan/build-lab/broken/memory+0x1184) (BuildId: 5ae51e727bd57c263156b71a46f5d3d81047ca4e)

0x502000000020 is located 0 bytes after 16-byte region [0x502000000010,0x502000000020)
allocated by thread T0 here:
    #0 0x7e246c6659c7 in malloc ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:69
    #1 0x63d98849e25e in main /home/stepan/build-lab/broken/memory.c:15
...
SUMMARY: AddressSanitizer: heap-buffer-overflow /home/stepan/build-lab/broken/memory.c:20 in main
Shadow bytes around the buggy address:
...
=>0x502000000000: fa fa 00 00[fa]fa fa fa fa fa fa fa fa fa fa fa
...
==41==ABORTING
код выхода: 1
```

### 3. Что в отчёте

- `WRITE of size 4`: запись четырёх байт, то есть одного `int`.
- `memory.c:20`: строка `data[i] = i * i;`.
- `0 bytes after 16-byte region`: блок 16 байт (4 × `sizeof(int)`), и запись идёт ровно за
  его концом. Выделен он в `memory.c:15`, это `malloc`.
- В теневой памяти `00 00` — наши 16 байт, а `[fa]` красная зона кучи, в которую я попал.

### 4. gdb: i перед записью

Пересобрал без оптимизации и поставил условную точку останова на строку 20, когда `i == 4`:

```text
$ gcc -O0 -g memory.c -o memory-dbg
$ gdb -q -batch -ex 'break memory.c:20 if i == 4' -ex run -ex 'print i' -ex 'print data[3]' -ex 'bt' ./memory-dbg
Breakpoint 1 at 0x11ba: file memory.c, line 20.
...
Breakpoint 1, main () at memory.c:20
20	        data[i] = i * i;
$1 = 4
$2 = 9
#0  main () at memory.c:20
```

Остановились перед записью в `data[4]`: `i` уже 4, а последний законный элемент `data[3]`
равен 9. Без условия пришлось бы четыре раза нажимать `continue`.

### 5. Исправление

`i <= 4` → `i < 4`. При `i < 4` цикл проходит индексы 0, 1, 2, 3, ровно столько, сколько
выделено. `<=` добавляет пятый шаг. Файл: [fixed/memory.c](fixed/memory.c).

```text
$ cp /path/to/practice/04-find-bug/fixed/memory.c memory.c
$ gcc -fsanitize=address,undefined -g -O1 memory.c -o memory && ./memory; echo "код выхода: $?"
last=9
код выхода: 0
```

Вывод тот же `last=9`, но теперь ASan и UBSan молчат.

### 6. cppcheck

```text
$ cppcheck --enable=warning,style ../hello/
Checking ../hello/greet.c ...
1/2 files checked 58% done
Checking ../hello/main.c ...
2/2 files checked 100% done
$ cp /path/to/Chapter04/broken/memory.c memory-orig.c
$ cppcheck --enable=warning,style memory-orig.c
Checking memory-orig.c ...
```

По `hello/` замечаний нет. Ради интереса прогнал cppcheck и по исходному `memory.c`. Он
ошибку тоже не нашёл: размер блока известен только через `malloc`, а такие случаи
cppcheck 2.13 тут не отследил. Значит, статический анализ санитайзер не заменяет.

Ответ на вопрос со слайда: «программа работает» значит только, что на этом запуске порча
памяти ни на что не повлияла. Здесь запись попала в запас, который оставил аллокатор. Выйди
цикл чуть дальше, он бы попал в служебный заголовок следующего блока или в чужие данные, и
программа упала бы позже и в другом месте.

## Остальные файлы из broken/

### undefined.c: undefined reference

```text
$ gcc undefined.c -o undefined
/usr/bin/ld: /tmp/ccBKfoJ9.o: in function `main':
undefined.c:(.text+0x13): undefined reference to `greet'
collect2: error: ld returned 1 exit status
$ gcc -c undefined.c && nm undefined.o
                 U greet
0000000000000000 T main
                 U puts
```

Компиляция прошла (прототип есть в `greet.h`), упал `ld`: `greet` помечен `U`, а файла с
её определением в команде нет. Лечение: передать `greet.c`.

```text
$ gcc undefined.c ../hello/greet.c -o undefined && ./undefined
Hello, world!
```

Мелочь: gcc заменил `printf("%s\n", ...)` на `puts`, поэтому в `nm` видно `puts`.

### duplicate-a.c + duplicate-b.c: multiple definition

```text
$ gcc duplicate-a.c duplicate-b.c -o dup
/usr/bin/ld: /tmp/ccKMLFll.o: in function `helper':
duplicate-b.c:(.text+0x0): multiple definition of `helper'; /tmp/ccpxI9Sh.o:duplicate-a.c:(.text+0x0): first defined here
/usr/bin/ld: /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o: in function `_start':
(.text+0x1b): undefined reference to `main'
collect2: error: ld returned 1 exit status
$ gcc -c duplicate-a.c duplicate-b.c && nm duplicate-a.o duplicate-b.o

duplicate-a.o:
0000000000000000 T helper
000000000000000f T print_a
                 U printf

duplicate-b.o:
0000000000000000 T helper
000000000000000f T print_b
                 U printf
```

Два глобальных `T helper`, линковщик не знает, какой брать. Функции разные (возвращают 1 и
2), так что «оставить одну» не подходит. Сделал `helper` в `duplicate-b.c` статической
([fixed/duplicate-b.c](fixed/duplicate-b.c)), и символ стал локальным `t`:

```text
$ gcc -c duplicate-a.c duplicate-b.c && nm duplicate-b.o
0000000000000000 t helper
000000000000000f T print_b
                 U printf
$ gcc duplicate-a.c duplicate-b.c -o dup
/usr/bin/ld: /usr/lib/gcc/x86_64-linux-gnu/13/../../../x86_64-linux-gnu/Scrt1.o: in function `_start':
(.text+0x1b): undefined reference to `main'
collect2: error: ld returned 1 exit status
```

`multiple definition` ушло, осталась вторая ошибка из первого вывода, которую я сначала
не заметил: в примере вообще нет `main`. Дописал свой [fixed/dup-main.c](fixed/dup-main.c):

```text
$ gcc -Wall -Wextra duplicate-a.c duplicate-b.c dup-main.c -o dup && ./dup
A: helper=1
B: helper=2
```

### missing-header.c: implicit declaration

```text
$ gcc missing-header.c -o missing && ./missing
missing-header.c: In function ‘main’:
missing-header.c:15:25: warning: implicit declaration of function ‘strlen’ [-Wimplicit-function-declaration]
   15 |     printf("len=%zu\n", strlen(text));
      |                         ^~~~~~
missing-header.c:11:1: note: include ‘<string.h>’ or provide a declaration of ‘strlen’
   10 | #include <stdio.h>
  +++ |+#include <string.h>
   11 | 
missing-header.c:15:25: warning: incompatible implicit declaration of built-in function ‘strlen’ [-Wbuiltin-declaration-mismatch]
   15 |     printf("len=%zu\n", strlen(text));
      |                         ^~~~~~
missing-header.c:15:25: note: include ‘<string.h>’ or provide a declaration of ‘strlen’
len=5
$ gcc -Werror missing-header.c -o missing; echo "код выхода: $?"
missing-header.c: In function ‘main’:
missing-header.c:15:25: error: implicit declaration of function ‘strlen’ [-Werror=implicit-function-declaration]
...
cc1: all warnings being treated as errors
код выхода: 1
```

Предупреждение gcc 13 выдаёт даже без `-Wall` и сам подсказывает, какой `#include` нужен.
Без `-Werror` файл собрался и посчитал верно, но только потому, что gcc узнал встроенную
`strlen`. С `-Werror` сборка встала. Исправление: добавить `#include <string.h>`
([fixed/missing-header.c](fixed/missing-header.c)).

```text
$ gcc -Wall -Wextra -Werror missing-header.c -o missing && ./missing
len=5
```

`Makefile-spaces` разобран в практике 2.

## Что понял

Ошибки сборки сами говорят этап: `error:` с номером строки от компилятора, `ld returned 1`
от линковщика. А ошибка с памятью ни на каком этапе сборки не видна. Её показал только
запуск с ASan, и то потому, что я специально пересобрал программу с ним.
