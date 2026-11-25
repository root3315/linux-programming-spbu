# ДЗ 4. Системы сборки (Make / CMake), проект hello

На Windows у меня нет компилятора, поэтому первый раз собирал в Docker-контейнере gcc:14.
Потом пересобрал всё на Ubuntu 24.04, как в курсе, и весь вывод ниже уже оттуда:
gcc 13.3.0, GNU Make 4.3, CMake 3.28.3, Meson 1.3.2.

Что сделано: к проекту hello (`main.c`, `greet.c/.h`) добавил третий модуль `stats.c/.h`
(считает длину имени, буквы, цифры и остальное), обновил `Makefile` и `CMakeLists.txt`,
включил `-Werror`, прогнал санитайзеры. Заодно положил `meson.build`. В задание он не
входит, но на Ubuntu 24.04 я его проверил: собирается и работает так же.

## 1. Команды сборки

Make:

```text
make                 обычная сборка (-O2 -g), получается ./hello
make release         чистая пересборка с -O2 -DNDEBUG
make check           сборка с ASan+UBSan в build-san/ и прогон программы
make clean
```

CMake:

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build
cmake -S . -B build-asan -DHELLO_SANITIZE=ON    (то же с санитайзерами)
cmake --build build-asan && ctest --test-dir build-asan
```

Meson:

```text
meson setup builddir
meson compile -C builddir
```

Флаги везде одни: `-std=c11 -Wall -Wextra -Wpedantic -Werror`. В Meson то же самое задано
через `warning_level=3` и `werror=true`. Санитайзеры:
`-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all`.

## 2. Вывод make и cmake

```text
$ make
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -c main.c -o main.o
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -c greet.c -o greet.o
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -c stats.c -o stats.o
gcc  -o hello main.o greet.o stats.o
$ ./hello; echo "код выхода: $?"
Hello, Linux!
длина 5: букв 5, цифр 0, прочих 0
greet() вызван 1 раз
код выхода: 0
$ ./hello ""
Hello, world!
длина 0: букв 0, цифр 0, прочих 0
greet() вызван 1 раз
$ ./hello 'Привет 2024'
Hello, Привет 2024!
длина 17: букв 0, цифр 4, прочих 13
greet() вызван 1 раз
```

`./hello Linux` даёт то же, что `./hello` без аргумента: по умолчанию имя "Linux". С пустым
аргументом `greet` говорит "Hello, world!". Длина у "Привет 2024" 17, потому что считаются
байты: каждая русская буква в UTF-8 занимает два.

```text
$ make release
rm -f main.o greet.o stats.o hello
rm -rf build-san
make CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -DNDEBUG" hello
make[1]: Entering directory '/home/stepan/build-homework'
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -DNDEBUG -c main.c -o main.o
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -DNDEBUG -c greet.c -o greet.o
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -DNDEBUG -c stats.c -o stats.o
gcc  -o hello main.o greet.o stats.o
make[1]: Leaving directory '/home/stepan/build-homework'
```

```text
$ cmake -S . -B build
-- The C compiler identification is GNU 13.3.0
...
-- Configuring done (0.2s)
-- Generating done (0.0s)
-- Build files have been written to: /home/stepan/build-homework/build
$ cmake --build build
[ 16%] Building C object CMakeFiles/greet.dir/greet.c.o
[ 33%] Linking C static library libgreet.a
[ 33%] Built target greet
[ 50%] Building C object CMakeFiles/stats.dir/stats.c.o
[ 66%] Linking C static library libstats.a
[ 66%] Built target stats
[ 83%] Building C object CMakeFiles/hello.dir/main.c.o
[100%] Linking C executable hello
[100%] Built target hello
$ ctest --test-dir build
...
1/4 Test #1: no_args ..........................   Passed    0.00 sec
2/4 Test #2: with_arg .........................   Passed    0.00 sec
3/4 Test #3: empty_arg ........................   Passed    0.00 sec
4/4 Test #4: cyrillic .........................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 4
```

Meson для сравнения:

```text
$ meson compile -C builddir
ninja: Entering directory `/home/stepan/build-homework/builddir'
[1/6] Compiling C object libgreet.a.p/greet.c.o
[2/6] Compiling C object hello.p/main.c.o
[3/6] Compiling C object libstats.a.p/stats.c.o
[4/6] Linking static target libstats.a
[5/6] Linking static target libgreet.a
[6/6] Linking target hello
...
$ ./builddir/hello "Meson"
Hello, Meson!
длина 5: букв 5, цифр 0, прочих 0
greet() вызван 1 раз
```

Предупреждений нет нигде. Проверял так: собрал всё с нуля одной цепочкой в лог и поискал
в нём слово warning.

```text
$ (make && make check && cmake -S . -B build && cmake --build build && meson setup builddir && meson compile -C builddir) > build.log 2>&1; echo "код выхода: $?"
код выхода: 0
$ grep -ci warning build.log
0
```

## 3. Что пересобралось и почему

Make, после `touch greet.h`:

```text
$ touch greet.h
$ make
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -c main.c -o main.o
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -c greet.c -o greet.o
gcc  -o hello main.o greet.o stats.o
```

Пересобрались `main.o` и `greet.o`: оба включают `greet.h`, и в Makefile это записано явно
(`main.o: greet.h stats.h`, `greet.o: greet.h`). `stats.o` не тронут, он про `greet.h` не
знает. Потом перелинковался `hello`, потому что изменились два его объектных файла.

Для сравнения, `touch stats.h`:

```text
$ touch stats.h
$ make
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -c main.c -o main.o
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -c stats.c -o stats.o
gcc  -o hello main.o greet.o stats.o
$ make
make: Nothing to be done for 'all'.
```

Тут наоборот: `main.o` и `stats.o` (они подключают `stats.h`), а `greet.o` остался.

Зависимости от заголовков в Makefile я прописал руками, поэтому там легко ошибиться:
добавил `#include` и забыл строку. Для этого обычно используют `gcc -MMD`, но по заданию
хватило ручных строк.

CMake, после `touch greet.h`:

```text
$ touch greet.h
$ cmake --build build
[ 16%] Building C object CMakeFiles/greet.dir/greet.c.o
[ 33%] Linking C static library libgreet.a
[ 33%] Built target greet
[ 66%] Built target stats
[ 83%] Building C object CMakeFiles/hello.dir/main.c.o
[100%] Linking C executable hello
[100%] Built target hello
```

Пересобрались `greet.c.o` и `main.c.o`, цель `stats` не тронута (`Built target` без
`Building`), потом `hello` перелинковался. Результат тот же, что у make, но руками я ничего
не перечислял. CMake передаёт gcc флаги `-MD -MT ... -MF ....o.d`, gcc сам записывает, какие
заголовки реально включил каждый `.c`, и при следующей сборке CMake читает эти файлы:

```text
$ touch greet.h && cmake --build build --verbose | grep -o "\-MD -MT [^ ]* -MF [^ ]*"
-MD -MT CMakeFiles/greet.dir/greet.c.o -MF CMakeFiles/greet.dir/greet.c.o.d
-MD -MT CMakeFiles/hello.dir/main.c.o -MF CMakeFiles/hello.dir/main.c.o.d
$ cat build/CMakeFiles/hello.dir/main.c.o.d
CMakeFiles/hello.dir/main.c.o: /home/stepan/build-homework/main.c \
 /usr/include/stdc-predef.h /home/stepan/build-homework/greet.h \
 /home/stepan/build-homework/stats.h \
...
```

В первый раз `touch stats.h` в CMake я не запускал, сейчас проверил. Как и ожидал,
пересобрались `stats.c` и `main.c`, без единой строки в `CMakeLists.txt`:

```text
$ touch stats.h
$ cmake --build build
[ 33%] Built target greet
[ 50%] Building C object CMakeFiles/stats.dir/stats.c.o
[ 66%] Linking C static library libstats.a
[ 66%] Built target stats
[ 83%] Building C object CMakeFiles/hello.dir/main.c.o
[100%] Linking C executable hello
[100%] Built target hello
```

## 4. Отчёт санитайзера

На чистом коде санитайзеры молчат. `make check` собирает всё с
`-fsanitize=address,undefined` и запускает hello четыре раза (без аргумента, "Linux", ""
и "Привет 2024"):

```text
$ make check; echo "код выхода: $?"
mkdir -p build-san
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all -c main.c -o build-san/main.o
...
./build-san/hello
Hello, Linux!
длина 5: букв 5, цифр 0, прочих 0
greet() вызван 1 раз
...
./build-san/hello "Привет 2024"
Hello, Привет 2024!
длина 17: букв 0, цифр 4, прочих 13
greet() вызван 1 раз
check: ошибок нет
код выхода: 0
```

То же с CMake (`HELLO_SANITIZE=ON`): все 4 теста `Passed`.

Чтобы убедиться, что `check` вообще что-то ловит, я временно сломал `stats.c` в копии
проекта (`~/hw-bug`, исходники не трогал): после `st.length++` добавил запись за границу
выделенных 4 байт на куче.

```text
$ diff -u ../build-homework/stats.c stats.c | tail -5
-        st.length++;
+        st.length++; { char *volatile tmp = malloc(4); tmp[4] = 0; free(tmp); }
         if (isdigit(c))
             st.digits++;
         else if (c < 128 && isalpha(c))
$ make check 2>&1 | grep -v "^gcc\|^mkdir"; echo "код выхода: ${PIPESTATUS[0]}"
./build-san/hello
=================================================================
==45==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000014 at pc 0x55eb5ba8a873 bp 0x7ffe677262a0 sp 0x7ffe67726290
WRITE of size 1 at 0x502000000014 thread T0
    #0 0x55eb5ba8a872 in stats_collect /home/stepan/hw-bug/stats.c:21
    #1 0x55eb5ba8a4cf in main /home/stepan/hw-bug/main.c:13
...
0x502000000014 is located 0 bytes after 4-byte region [0x502000000010,0x502000000014)
allocated by thread T0 here:
    #0 0x74f4c14609c7 in malloc ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:69
    #1 0x55eb5ba8a927 in stats_collect /home/stepan/hw-bug/stats.c:21
...
SUMMARY: AddressSanitizer: heap-buffer-overflow /home/stepan/hw-bug/stats.c:21 in stats_collect
...
==45==ABORTING
make: *** [Makefile:65: check] Error 1
код выхода: 2
```

Наверху ещё добавил `#include <stdlib.h>` для `malloc`, поэтому строка сдвинулась на 21.
Код выхода make ненулевой (2), то есть `check` годится как проверка в CI. Зачем там
`volatile`, написал ниже в п. 3.

Отдельно пробовал ошибку для UBSan (индекс 1 в массиве `int[1]`):

```text
$ diff -u ../build-homework/stats.c stats.c
...
-        st.length++;
+        st.length++; { int one[1] = {0}; volatile int k = 1; st.other += one[k]; }
...
$ make check; echo "код выхода: $?"
...
./build-san/hello
stats.c:20:77: runtime error: index 1 out of bounds for type 'int [1]'
make: *** [Makefile:65: check] Error 1
код выхода: 2
```

Тоже падает с ненулевым кодом, но только благодаря `-fno-sanitize-recover=all` (см. п. 1
ниже). После этих опытов копии удалил, а `make check` на настоящем проекте снова даёт 0.

## 5. Что было непонятно

1. Почему UBSan не роняет программу. Я ждал, что при неопределённом поведении программа
   остановится, но по умолчанию UBSan только печатает `runtime error: ...` и работает
   дальше, код выхода остаётся 0, и `make check` был бы зелёным при ошибке. Проверил на той
   же сломанной копии, собрав просто с `-fsanitize=undefined`:

   ```text
   $ gcc -std=c11 -g -fsanitize=undefined main.c greet.c stats.c -o ub-default && ./ub-default; echo "код выхода: $?"
   stats.c:20:77: runtime error: index 1 out of bounds for type 'int [1]'
   ...
   Hello, Linux!
   длина 5: букв 5, цифр 0, прочих 18446744069737276416
   greet() вызван 1 раз
   код выхода: 0
   ```

   Поэтому добавил `-fno-sanitize-recover=all`: тогда UBSan завершает программу с ошибкой.
   ASan останавливает программу сам.

2. Зачем `release` начинается с `clean`. Make сравнивает только время файлов и про смену
   флагов не знает: если просто вызвать сборку с другим `CFLAGS`, старые `main.o` останутся
   «свежими» и ничего не пересоберётся. Поэтому `release` сначала чистит и потом зовёт
   `$(MAKE)` с переопределённым `CFLAGS`. Минус: после `make release` обычная сборка тоже
   идёт заново.

3. Почему на gcc 13 мою ошибку для ASan поймал не ASan. Вариант без `volatile`
   (`char *tmp = malloc(4); tmp[4] = 0;`) при пересборке на Ubuntu 24.04 упал, но с
   сообщением UBSan:
   `runtime error: store to address 0x502000000014 with insufficient space for an object of
   type 'char'`. Оказалось, в `-fsanitize=undefined` есть проверка размера объекта, и
   компилятор знал, что блок из `malloc(4)`. С `char *volatile tmp` он этого уже не видит, и
   срабатывает ASan с нормальным отчётом про heap-buffer-overflow. Для `check` разницы нет,
   код выхода ненулевой в обоих случаях, но я сначала не понял, откуда другой текст.

4. Не уверен насчёт static-библиотек в CMake: я сделал `greet` и `stats` отдельными
   библиотеками, хотя можно было просто перечислить все три `.c` в `add_executable`. Так
   видно зависимости между целями (в выводе сборки), но для такого маленького проекта это,
   наверное, лишнее.
