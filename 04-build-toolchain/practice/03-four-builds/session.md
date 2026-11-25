# Практика 3. Один проект, четыре сборки

Один и тот же `hello/` из Chapter 04 собирал через make, CMake и Meson, а `hello-autotools/`
через autotools. Свои файлы не понадобились, только команды. Контейнер с Ubuntu 24.04:
gcc 13.3.0, CMake 3.28.3, Meson 1.3.2, ninja 1.11.1.

## 1. Убрать следы make

```text
$ make clean
rm -f main.o greet.o hello
$ ls
CMakeLists.txt
Makefile
greet.c
greet.h
main.c
meson.build
```

## 2–4. CMake

```text
$ cmake -S . -B build
-- The C compiler identification is GNU 13.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Configuring done (0.8s)
-- Generating done (0.0s)
-- Build files have been written to: /home/stepan/build-lab/hello/build
$ cmake --build build
[ 25%] Building C object CMakeFiles/greet.dir/greet.c.o
[ 50%] Linking C static library libgreet.a
[ 50%] Built target greet
[ 75%] Building C object CMakeFiles/hello.dir/main.c.o
[100%] Linking C executable hello
[100%] Built target hello
$ ./build/hello "CMake"
Hello, CMake!
greet() вызван 1 раз
```

Компилятор CMake нашёл сам: `/usr/bin/cc`, это тот же gcc 13.3.0. В отличие от make, тут
появился промежуточный `libgreet.a`, потому что в `CMakeLists.txt` greet описан как
статическая библиотека. Все артефакты в `build/`, рядом с исходниками ничего:

```text
$ ls build
CMakeCache.txt
CMakeFiles
Makefile
cmake_install.cmake
hello
libgreet.a
$ find build -name "*.o"
build/CMakeFiles/hello.dir/main.c.o
build/CMakeFiles/greet.dir/greet.c.o
```

Команды целиком CMake показывает с `VERBOSE=1`:

```text
$ touch greet.h && cmake --build build -- VERBOSE=1 | grep -E "/usr/bin/cc"
/usr/bin/cc  -I/home/stepan/build-lab/hello -std=gnu11 -Wall -Wextra -MD -MT CMakeFiles/greet.dir/greet.c.o -MF CMakeFiles/greet.dir/greet.c.o.d -o CMakeFiles/greet.dir/greet.c.o -c /home/stepan/build-lab/hello/greet.c
/usr/bin/cc  -I/home/stepan/build-lab/hello -std=gnu11 -MD -MT CMakeFiles/hello.dir/main.c.o -MF CMakeFiles/hello.dir/main.c.o.d -o CMakeFiles/hello.dir/main.c.o -c /home/stepan/build-lab/hello/main.c
/usr/bin/cc CMakeFiles/hello.dir/main.c.o -o hello  libgreet.a 
```

Сравнил с make, и отличий нашлось три. Стандарт `-std=gnu11`, а не `c11`: `CMAKE_C_STANDARD 11`
по умолчанию включает расширения GNU. `-Wall -Wextra` стоят только у greet, потому что
`target_compile_options(greet PRIVATE ...)`. Нет `-O2 -g`: тип сборки не задан. Зато есть
`-I` на каталог с `greet.h` (из `PUBLIC` include у greet) и `-MD -MF`, через которые
CMake сам узнаёт про заголовки.

## 5. Правка greet.c

```text
$ echo "/* правка */" >> greet.c
$ cmake --build build
[ 25%] Building C object CMakeFiles/greet.dir/greet.c.o
[ 50%] Linking C static library libgreet.a
[ 50%] Built target greet
[ 75%] Linking C executable hello
[100%] Built target hello
```

Пересобрался только `greet.c.o`, библиотека и перелинковка `hello`. `main.c.o` не трогался.

Второй каталог сборки, релизный, рядом с первым:

```text
$ cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
...
-- Build files have been written to: /home/stepan/build-lab/hello/build-release
$ cmake --build build-release
...
[100%] Built target hello
$ ls build/Makefile build-release/Makefile
build-release/Makefile
build/Makefile
```

## 6. Meson

```text
$ meson setup builddir
The Meson build system
Version: 1.3.2
Source dir: /home/stepan/build-lab/hello
Build dir: /home/stepan/build-lab/hello/builddir
Build type: native build
Project name: hello
Project version: 1.0.0
C compiler for the host machine: cc (gcc 13.3.0 "cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0")
C linker for the host machine: cc ld.bfd 2.42
Host machine cpu family: x86_64
Host machine cpu: x86_64
Build targets in project: 2

Found ninja-1.11.1 at /usr/bin/ninja
$ meson compile -C builddir
ninja: Entering directory `/home/stepan/build-lab/hello/builddir'
[1/4] Compiling C object libgreet.a.p/greet.c.o
[2/4] Compiling C object hello.p/main.c.o
[3/4] Linking static target libgreet.a
[4/4] Linking target hello
INFO: autodetecting backend as ninja
INFO: calculating backend command to run: /usr/bin/ninja -C /home/stepan/build-lab/hello/builddir
$ ./builddir/hello "Meson"
Hello, Meson!
greet() вызван 1 раз
```

## 7. autotools

Тут споткнулся. По слайду достаточно `autoreconf --install && ./configure && make`:

```text
$ autoreconf --install
$ ./configure
checking for a BSD-compatible install... /usr/bin/install -c
checking whether build environment is sane... yes
/home/stepan/build-lab/hello-autotools/missing: line 3: $'\r': command not found
...
/home/stepan/build-lab/hello-autotools/missing: line 32: syntax error near unexpected token `$'in\r''
/home/stepan/build-lab/hello-autotools/missing: line 32: `case $1 in'
configure: WARNING: 'missing' script is too old or missing
...
config.status: creating Makefile
config.status: executing depfiles commands
$ make
...
./depcomp: line 28: syntax error near unexpected token `$'in\r''
./depcomp: line 28: `case $1 in'
make: *** [Makefile:378: main.o] Error 2
```

`$'\r'` подсказал причину: в архиве Chapter 04 все файлы с концами строк CRLF. `configure`
`autoreconf --install` пересоздал, а вспомогательные `missing` и `depcomp` уже лежали в
каталоге, и он их не тронул. Bash спотыкается о `\r` в конце строк. С `--force` скрипты
создаются заново:

```text
$ file missing depcomp configure
missing:   POSIX shell script, ASCII text executable, with CRLF line terminators
depcomp:   POSIX shell script, ASCII text executable, with CRLF line terminators
configure: POSIX shell script, ASCII text executable
$ autoreconf --install --force
$ file missing depcomp configure
missing:   POSIX shell script, ASCII text executable
depcomp:   POSIX shell script, ASCII text executable
configure: POSIX shell script, ASCII text executable
$ ./configure | tail -4
checking that generated files are newer than configure... done
configure: creating ./config.status
config.status: creating Makefile
config.status: executing depfiles commands
$ make
gcc -DPACKAGE_NAME=\"hello\" -DPACKAGE_TARNAME=\"hello\" -DPACKAGE_VERSION=\"1.0.0\" -DPACKAGE_STRING=\"hello\ 1.0.0\" -DPACKAGE_BUGREPORT=\"student@spbu.ru\" -DPACKAGE_URL=\"\" -DPACKAGE=\"hello\" -DVERSION=\"1.0.0\" -I.    -Wall -Wextra -O2 -g -O2 -MT main.o -MD -MP -MF .deps/main.Tpo -c -o main.o main.c
mv -f .deps/main.Tpo .deps/main.Po
gcc -Wall -Wextra -O2 -g -O2   -o hello main.o  
$ ./hello
Hello from autotools!
```

В команде gcc видно, что сделал `./configure`: все поля из `AC_INIT` пришли макросами `-D`,
а `-g -O2` — это его `CFLAGS` по умолчанию, они идут после `AM_CFLAGS` из `Makefile.am`.

## 8. Всё с нуля одной командой

```text
$ rm -rf build build-release builddir && make clean
rm -f main.o greet.o hello
$ make && cmake -S . -B build && cmake --build build && meson setup builddir && meson compile -C builddir
gcc -std=c11 -Wall -Wextra -O2 -g -c main.c -o main.o
gcc -std=c11 -Wall -Wextra -O2 -g -c greet.c -o greet.o
gcc -o hello main.o greet.o
-- The C compiler identification is GNU 13.3.0
...
[100%] Built target hello
The Meson build system
...
[4/4] Linking target hello
...
```

## Ответы на вопросы со слайда

1. Почему в CMake нет `hello.o`. Объектные файлы у CMake внутренняя деталь, они лежат в
   `build/CMakeFiles/<цель>.dir/` и называются `main.c.o`. Я описываю цели, а не файлы.
2. Откуда hello узнал про `greet.h`. Из `target_include_directories(greet PUBLIC ...)`:
   `PUBLIC` передаёт каталог всем, кто линкуется с greet. В команде это `-I/home/stepan/...`.
3. Что будет, если собрать в двух каталогах. Две независимые сборки (`build` и
   `build-release`), у каждой свой кэш и свой Makefile. Друг другу не мешают.
4. Где лежит сгенерированный Makefile. В каталоге сборки: `build/Makefile`.
5. Чем `meson setup` отличается от `cmake -S . -B build`. По сути то же самое: настроить
   каталог сборки. Только Meson всегда генерирует файлы для ninja и сразу его ищет, а CMake
   по умолчанию делает Makefile.
6. Почему в autotools сначала `./configure`. Makefile ещё нет, есть только шаблон
   `Makefile.in`. `./configure` проверяет систему (какой компилятор, что умеет) и по
   результатам пишет Makefile.

## Что понял

Все четыре способа дали одинаково работающий hello, а различаются тем, где лежат артефакты
и сколько служебных файлов появляется. Каталог сборки отдельно от исходников (CMake, Meson)
оказался удобнее: удалил `build/`, и всё чисто. Autotools ещё и напомнил, что скрипты с
CRLF в Linux не работают.
