# ДЗ 8. Функция shorten_name под пятью проверками

Проект из Chapter 08 после практики 4 (в `greeting.c` уже `snprintf`), в него добавил свою
функцию. Всё собирал и запускал в контейнере с Ubuntu 24.04: gcc 13.3.0, gcov 13.3.0,
cppcheck 2.13.0, clang-tidy 18.1.3, CMake 3.28.3.

## 1. Что делает функция

```c
int shorten_name(char *out, size_t cap, const char *name);
```

Кладёт в `out` имя, сокращённое до 10 символов вместе с многоточием. Имя до 10 букв
копируется как есть, длинное становится «первые 9 букв + `…`». Многоточие — один символ
U+2026, в UTF-8 это 3 байта.

Главное решение: «10 букв» считаю буквами, а не байтами. Иначе «Александрина» (12 букв,
24 байта) обрезалась бы на 10-м байте, то есть после 5 букв, а для эмодзи или `€` граница
байтов попала бы посреди буквы, и на экране получился бы мусор. Буква в UTF-8 занимает от 1 до 4 байт, у
продолжений старшие биты `10`, с них буква не начинается. Функция режет только по границе
буквы.

Битый UTF-8 функция не чинит. Байт, который не продолжает начатую букву, считается
отдельной буквой, поэтому любая «буква» не длиннее 4 байт (почему это важно, в разделе 4).

Коды возврата: `0` — скопировано без изменений, `1` — сокращено, `-1` — `out == NULL` или
`cap == 0`, `-2` — не влезло в `cap`, в `out` пустая строка. `NULL` вместо имени
работает как пустая строка. Буфера `SHORT_NAME_CAP` = 40 байт хватает всегда: 9 букв по
4 байта + 3 байта многоточия + `'\0'`.

```text
$ ./app Linux
Hello, Linux!
$ ./app Александрина
Hello, Александр…!
$ ./app "😀😀😀😀😀😀😀😀😀😀😀"
Hello, 😀😀😀😀😀😀😀😀😀…!
```

`app` сначала сокращает имя, потом передаёт его в `make_greeting`. Заметил странность:
`./app "!Константинопольский"` даёт `HELLO, Констант…!!!`, потому что `!` тоже считается
буквой. Для «громкого» режима остаётся 8 букв имени, а не 9. Оставил так: функция про
длину строки, а не про формат приветствия.

## 2. make test — вывод

```text
$ make test; echo "код: $?"
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -o tests/test_greeting tests/test_greeting.c src/greeting.c src/shorten.c
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -o tests/test_shorten tests/test_shorten.c src/greeting.c src/shorten.c
./tests/test_greeting
ok:   код возврата 0
ok:   обычное имя
ok:   NULL допустим
ok:   NULL заменяется на world
ok:   пустое имя допустимо
ok:   пустое имя заменяется
ok:   out == NULL отклоняется
ok:   cap == 0 отклоняется
ok:   кириллица
ok:   громкое приветствие
ok:   длинное имя возвращает -2

все проверки прошли (но это не значит, что код без ошибок)
./tests/test_shorten
ok:   обычное имя не меняется
ok:   пустое имя остаётся пустым
ok:   NULL как пустое имя
ok:   ровно 10 букв не режется
ok:   длинное имя: 9 букв и многоточие
ok:   кириллица, ровно 10 букв (20 байт)
ok:   кириллица режется по буквам, а не по байтам
ok:   11 эмодзи: результат ровно на SHORT_NAME_CAP байт
ok:   битый UTF-8: одинокий байт продолжения — отдельная буква
ok:   битый UTF-8: оборванная буква — отдельная буква
ok:   out == NULL отклоняется
ok:   cap == 0 отклоняется
ok:   буфер впритык: 5 букв + '\0' в 6 байт
ok:   буфер на байт меньше: -2 и пустая строка

все проверки shorten_name прошли
код: 0
```

Три обязательных случая из условия: «обычное имя не меняется», «пустое имя остаётся
пустым», «длинное имя: 9 букв и многоточие». Остальные — границы, на которых легко
ошибиться: ровно 10 букв, кириллица по 2 байта, эмодзи по 4 (11 эмодзи дают ровно 40 байт
с `'\0'`, худший случай для `SHORT_NAME_CAP`), буфер впритык и на байт меньше. Два теста на
битый UTF-8 появились после фаззера и покрытия (разделы 3 и 4).

Перед каждой проверкой `expect()` забивает буфер символами `'x'`, чтобы тест не прошёл
случайно на нулях, оставшихся от прошлого вызова.

## 3. make coverage — процент и белые пятна

Цель `coverage` из архива я переделал. Теперь она собирает оба теста с `--coverage`,
запускает их и вызывает `gcov` по исходникам `greeting.c` и `shorten.c`. Объектные файлы
названы как исходники, поэтому `gcov` находит `.gcno` сам, без старой ошибки
`cannot open notes file`. Если в отчётах остался хоть один `#####`, цель падает.

Первый прогон, когда второго теста на битый UTF-8 ещё не было:

```text
$ make coverage; echo "код: $?"
...
gcov -o build/cov src/greeting.c src/shorten.c | grep -A1 "^File 'src/"
File 'src/greeting.c'
Lines executed:100.00% of 11
--
File 'src/shorten.c'
Lines executed:96.43% of 28
--- непокрытые строки (#####) ---
shorten.c.gcov:26:    #####:   25:            return i;
make: *** [Makefile:51: coverage] Error 1
код: 2
```

Белое пятно одно, строка 25 в `char_len`:

```c
for (size_t i = 1; i < len; i++)
    if (!is_continuation((unsigned char)s[i]))
        return i;
```

Это случай, когда первый байт обещает многобайтовую букву, а продолжения нет: буква
оборвана. Мой тест на битый UTF-8 проверял только лишние байты продолжения. Добавил
второй: `"abc\xD0" "defghijk"`, где `D0` — первый байт «Я» без второго. Жду, что `D0`
станет отдельной буквой и результат будет `"abc\xD0" "defgh…"`.

```text
$ make coverage; echo "код: $?"
...
gcov -o build/cov src/greeting.c src/shorten.c | grep -A1 "^File 'src/"
File 'src/greeting.c'
Lines executed:100.00% of 11
--
File 'src/shorten.c'
Lines executed:100.00% of 28
--- непокрытые строки (#####) ---
нет
код: 0
```

100% строк у `shorten.c` и `greeting.c`.

## 4. Отчёт санитайзера до исправления

### Исходная ошибка проекта

Это ошибка из лекции, её я исправил в практике 4. Фаззер из архива на старом `greeting.c`:

```text
==610==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7f36e9900040 at pc 0x7f36ec597923 bp 0x7ffdcd73d1a0 sp 0x7ffdcd73c948
WRITE of size 30 at 0x7f36e9900040 thread T0
    #2 0x6345c90a5c5a in make_greeting src/greeting.c:29
    #3 0x6345c90a5756 in main fuzz/fuzz_greeting.c:40
```

После `snprintf` фаззер, тесты и `make sanitize` зелёные (вывод в практике 4).

### Ошибка, которую нашёл фаззер: битый UTF-8

Первая версия `shorten.c` считала буквой «байт, который не продолжение, и все продолжения
после него». Тесты (тогда их было 12) прошли. Фаззер с кусочками ASCII, кириллицы, `€`,
эмодзи и одного байта `\x80` тоже: 2000 попыток, потом по 200 000 с зёрнами 1, 2, 3.

Пока писал проверки в фаззере, засомневался в обещании из заголовка «40 байт хватает
всегда». Хвост из десятка байтов `\x80` после `Z` по старой логике — одна буква, и 9 таких
«букв» могут занять сколько угодно байт. Сам фаззер такое не собирал: нужно много `\x80`
подряд. Добавил в его алфавит кусочек из восьми `\x80`:

```text
$ make fuzz; echo "код: $?"
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer -o fuzz_greeting fuzz/fuzz_greeting.c src/greeting.c src/shorten.c
./fuzz_greeting 2000 1
фаззинг: 2000 попыток, зерно 1
ОШИБКА на попытке 115: shorten_name: неверный результат
...
make: *** [Makefile:69: fuzz] Error 1
код: 2
```

Санитайзер тут ни при чём: память не портится, функция честно вернула `-2`. Нарушено
обещание интерфейса, и поймала это проверка в фаззере (`-2` допустим только при
`cap < SHORT_NAME_CAP`). Чтобы видеть подробности, добавил в фаззер печать кода, `cap` и
длины имени. Тот же прогон со старым `shorten.c` (сохранил его как `~/shorten.c.v1`) и
финальным фаззером:

```text
$ gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer -o /tmp/fuzz-v1 fuzz/fuzz_greeting.c src/greeting.c -x c ~/shorten.c.v1 && /tmp/fuzz-v1 2000 1; echo "код: $?"
фаззинг: 2000 попыток, зерно 1
shorten_name: код -2, cap 40, имя 114 байт, out ""
ОШИБКА на попытке 115: shorten_name: неверный результат
...
код: 1
```

`-x c` нужен, потому что без расширения `.c` gcc принял файл за скрипт линковщика
(`file format not recognized; treating as linker script`).

Исправление: длину буквы теперь задаёт первый байт (`0xxxxxxx` — 1, `110xxxxx` — 2,
`1110xxxx` — 3, `11110xxx` — 4), и буква кончается раньше, если продолжения нет. Лишний
байт продолжения становится отдельной буквой.

```text
$ diff -u ~/shorten.c.v1 src/shorten.c | tail -n +3
@@ -11,21 +11,28 @@
...
+static size_t char_len(const char *s)
+{
+    unsigned char lead = (unsigned char)s[0];
+    size_t len = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
+
+    for (size_t i = 1; i < len; i++)
+        if (!is_continuation((unsigned char)s[i]))
+            return i;
+    return len;
+}
+
+/* Сколько байт занимают первые max_chars букв строки s. */
 static size_t prefix_bytes(const char *s, size_t max_chars)
 {
     size_t bytes = 0;
-    size_t chars = 0;

-    while (s[bytes] != '\0') {
-        if (!is_continuation((unsigned char)s[bytes])) {
-            if (chars == max_chars)
-                break;
-            chars++;
-        }
-        bytes++;
-    }
+    for (size_t chars = 0; chars < max_chars && s[bytes] != '\0'; chars++)
+        bytes += char_len(s + bytes);
     return bytes;
 }
```

Плюс тест «битый UTF-8: одинокий байт продолжения — отдельная буква», чтобы это не
вернулось. Число букв фаззер теперь сверяет только для правильного UTF-8: у битого оно
зависит от того, как считать, и проверка просто повторяла бы саму функцию.

### Ошибка, внесённая специально (продвинутый критерий)

Первая попытка: `need > cap` вместо `need >= cap`, классическая ошибка на единицу. Её
сразу поймал мой тест «буфер на байт меньше», так что для фаззера она не годилась.
Вторая: забыл, что многоточие тоже занимает место, и сравниваю с буфером только длину
букв:

```text
$ diff -u ~/shorten.c.ok src/shorten.c | tail -n +3
@@ -50,7 +50,7 @@

     size_t tail = shortened ? sizeof ELLIPSIS - 1 : 0;
     size_t need = keep + tail;            /* байт без завершающего '\0' */
-    if (need >= cap) {
+    if (keep >= cap) {
         out[0] = '\0';
         return -2;
     }
```

Тесты, покрытие и санитайзер на тестах её не видят:

```text
$ make test 2>&1 | grep -E "FAIL|прошли"; echo "make test: код ${PIPESTATUS[0]}"
все проверки прошли (но это не значит, что код без ошибок)
все проверки shorten_name прошли
make test: код 0
$ make sanitize 2>&1 | tail -n 1
тесты под ASan и UBSan прошли
$ make coverage 2>&1 | tail -n 4
File 'src/shorten.c'
Lines executed:100.00% of 28
--- непокрытые строки (#####) ---
нет
```

Все мои тесты на длинные имена идут с буфером 40 байт, а там места хватает. `make tidy`
тоже молчит. Фаззер:

```text
$ make fuzz > fuzz-bug.txt 2>&1; echo "make fuzz: код $?"
make fuzz: код 2
$ head -n 25 fuzz-bug.txt
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer -o fuzz_greeting fuzz/fuzz_greeting.c src/greeting.c src/shorten.c
./fuzz_greeting 2000 1
фаззинг: 2000 попыток, зерно 1
=================================================================
==1557==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x503000000081 at pc 0x7f4e6be8a303 bp 0x7ffde9608b30 sp 0x7ffde96082d8
WRITE of size 3 at 0x503000000081 thread T0
    #0 0x7f4e6be8a302 in memcpy ../../../../src/libsanitizer/sanitizer_common/sanitizer_common_interceptors_memintrinsics.inc:115
    #1 0x5beacf139fc9 in memcpy /usr/include/x86_64-linux-gnu/bits/string_fortified.h:29
    #2 0x5beacf139fc9 in shorten_name src/shorten.c:59
    #3 0x5beacf138b13 in fuzz_shorten fuzz/fuzz_greeting.c:133
    #4 0x5beacf138b13 in main fuzz/fuzz_greeting.c:156
    #5 0x7f4e6b5901c9  (/lib/x86_64-linux-gnu/libc.so.6+0x2a1c9) (BuildId: 8cebda42d140fc2b09639f8b13a5d5f2e617ef09)
    #6 0x7f4e6b59028a in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x2a28a) (BuildId: 8cebda42d140fc2b09639f8b13a5d5f2e617ef09)
    #7 0x5beacf139ae4 in _start (/home/stepan/health-hw/fuzz_greeting+0x5ae4) (BuildId: a9e2cd9d87a90b6184622250ec134d200356f2dd)

0x503000000081 is located 0 bytes after 17-byte region [0x503000000070,0x503000000081)
allocated by thread T0 here:
    #0 0x7f4e6be8c9c7 in malloc ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:69
    #1 0x5beacf138af9 in fuzz_shorten fuzz/fuzz_greeting.c:129
    #2 0x5beacf138af9 in main fuzz/fuzz_greeting.c:156
    #3 0x7f4e6b5901c9  (/lib/x86_64-linux-gnu/libc.so.6+0x2a1c9) (BuildId: 8cebda42d140fc2b09639f8b13a5d5f2e617ef09)
    #4 0x7f4e6b59028a in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x2a28a) (BuildId: 8cebda42d140fc2b09639f8b13a5d5f2e617ef09)
    #5 0x5beacf139ae4 in _start (/home/stepan/health-hw/fuzz_greeting+0x5ae4) (BuildId: a9e2cd9d87a90b6184622250ec134d200356f2dd)

SUMMARY: AddressSanitizer: heap-buffer-overflow ../../../../src/libsanitizer/sanitizer_common/sanitizer_common_interceptors_memintrinsics.inc:115 in memcpy
$ for n in $(seq 1 100); do ./fuzz_greeting $n 1 > /dev/null 2>&1 || { echo "упало на попытке $n"; break; }; done
упало на попытке 4
```

Читаю: переполнение в куче, запись 3 байт (это многоточие), строка `shorten.c:59`
`memcpy(out + keep, ELLIPSIS, tail)`. Буфер на 17 байт выделил фаззер (`malloc(cap)` в
строке 129), и запись началась ровно за его концом. Значит, буквы заняли весь буфер, а
многоточию места не осталось. Упало на 4-й попытке из 2000.

Сработало, потому что в фаззере буфер случайного размера от 1 до 48 байт и выделяется
через `malloc` ровно на `cap`. ASan ставит «забор» сразу за концом такого блока. В тестах
буферы фиксированного размера, а такие размеры, как 17, я там просто не пробовал.

После исправления (вернул `need >= cap`) всё зелёное:

```text
$ make fuzz; echo "код: $?"
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer -o fuzz_greeting fuzz/fuzz_greeting.c src/greeting.c src/shorten.c
./fuzz_greeting 2000 1
фаззинг: 2000 попыток, зерно 1
проблем не найдено — но это не доказательство, что их нет
код: 0
$ make sanitize; echo "код: $?"
...
тесты под ASan и UBSan прошли
код: 0
```

### Что нашли линтеры

Свою цель `tidy` сделал строгой: `cppcheck --error-exitcode=1` и
`clang-tidy --warnings-as-errors='*'` по `src`, `tests` и `fuzz`. Первый прогон:

```text
$ make tidy; echo "код: $?"
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null
cppcheck --enable=warning,style --error-exitcode=1 --quiet -Iinclude src tests fuzz
clang-tidy -p build --quiet --warnings-as-errors='*' \
	src/*.c tests/*.c fuzz/*.c
[1/6] Processing file /home/stepan/health-hw/src/greeting.c.
547 warnings generated.
[2/6] Processing file /home/stepan/health-hw/src/greeting.c.
1094 warnings generated.
[3/6] Processing file /home/stepan/health-hw/src/greeting.c.
...
[12/6] Processing file /home/stepan/health-hw/fuzz/fuzz_greeting.c.
6498 warnings generated.
/home/stepan/health-hw/fuzz/fuzz_greeting.c:38:9: error: Call to function 'strcat' is insecure as it does not provide bounding of the memory buffer. Replace unbounded copy functions with analogous functions that support length arguments such as 'strlcat'. CWE-119 [clang-analyzer-security.insecureAPI.strcpy,-warnings-as-errors]
   38 |         strcat(name, pieces[rand() % PIECES]);
      |         ^~~~~~
...
make: *** [Makefile:78: tidy] Error 1
код: 2
```

`cppcheck` молчал. `clang-tidy` нашёл две вещи:

1. `strcat` в моём же фаззере, в сборке случайного имени. Переполнения там не было (буфер
   имени на 481 байт, а 60 кусков по 8 байт дают максимум 480), но придирка правильная:
   размер нигде не проверялся. Переписал `random_name` на `memcpy` с явной длиной и
   проверкой `cap`.
2. `[12/6]`: `greeting.c` и `shorten.c` проверялись по 4 раза. В `CMakeLists.txt` каждая
   из четырёх программ компилировала их заново, и в `compile_commands.json` попало по
   4 записи. Вынес их в библиотеку `health`, теперь 6 файлов и 6 проверок.

Числа вида «547 warnings generated» — это замечания в системных заголовках, `clang-tidy`
их сам скрывает. Без `--quiet` он пишет об этом прямо: `Suppressed 942 warnings (942 in
non-user code)` (так было на `make lint-demo`).

Одно правило выключил в `.clang-tidy`:
`clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling`. Оно ругается на
любой `memcpy`, `snprintf`, `fprintf` и советует `memcpy_s`, `snprintf_s` из приложения K
стандарта C11. В glibc их нет: `grep -rl memcpy_s /usr/include/` ничего не нашёл. Если
правило включить:

```text
$ clang-tidy -p build --quiet --checks=clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling src/shorten.c 2>&1 | grep "warning:"
/home/stepan/health-hw/src/shorten.c:58:5: warning: Call to function 'memcpy' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'memcpy_s' in case of C11 [clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling]
/home/stepan/health-hw/src/shorten.c:59:5: warning: Call to function 'memcpy' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'memcpy_s' in case of C11 [clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling]
```

Оба `memcpy` пишут вместе не больше `need` байт, а `need < cap` проверено строкой выше,
так что предупреждение ложное. Итог:

```text
$ make tidy 2>&1 | grep -v "warnings generated\|Processing file"; echo "код: ${PIPESTATUS[0]}"
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null
cppcheck --enable=warning,style --error-exitcode=1 --quiet -Iinclude src tests fuzz
clang-tidy -p build --quiet --warnings-as-errors='*' \
	src/*.c tests/*.c fuzz/*.c
cppcheck и clang-tidy замечаний не нашли
код: 0
$ make tidy 2>&1 | grep -c "warning:"
0
```

## 5. Выводы

Что какой инструмент нашёл в моей функции:

| Инструмент | Нашёл | Что сделал |
|---|---|---|
| тесты | ошибку на единицу `need > cap` (специальную) | вернул `>=` |
| покрытие | непроверенную ветку «оборванная буква» | тест `"abc\xD0" "defghijk"` |
| фаззер и его проверки | `-2` при буфере 40 байт на битом UTF-8 | `char_len`, буква не длиннее 4 байт |
| фаззер и ASan | многоточие не учтено в размере (специальную) | вернул `need >= cap` |
| clang-tidy | `strcat` в фаззере, двойные проверки файлов | `memcpy` с длиной, библиотека в CMake |
| cppcheck | ничего | — |

Специальную ошибку с многоточием пропустили 14 тестов, 100% покрытия, ASan на тестах и оба
линтера. Фаззер нашёл её на 4-й попытке, потому что пробовал размеры буфера, о которых я не
подумал. А первую, на единицу, поймал обычный тест: «буфер впритык» я проверял специально.
Тесты хороши ровно на тех границах, которые придумал человек.

С битым UTF-8 фаззер сам ничего не нашёл, пока я не дал ему подходящие кусочки. Какие
данные генерировать, всё равно решает человек.

Покрытие показало пустую ветку и заставило написать на неё тест. Но 100% ничего не
доказали: ошибка с многоточием жила при 100% строк.

### Что было непонятно

1. Почему `make tidy` из архива всегда зелёный. Оказалось, `cppcheck` без
   `--error-exitcode` выходит с 0 даже при `error:`, а `clang-tidy` по умолчанию
   предупреждения ошибками не считает. Плюс цель проверяла только `lint/issues.c`.
2. Откуда 12 проверок на 6 файлов в `clang-tidy` (`[12/6]`). Это повторы одного файла в
   `compile_commands.json`, по одному на каждую программу, которая его компилирует.
3. Почему в отчёте ASan есть кадр с `memcpy` из `string_fortified.h`. В Ubuntu gcc при
   `-O2` включает `_FORTIFY_SOURCE`, и `memcpy` идёт через обёртку glibc. Моя строка всё
   равно чуть ниже: `#2 shorten_name src/shorten.c:59`.
4. Почему пропала первая строка вывода фаззера, когда его ронял ASan. `stdout` в файл или
   pipe буферизуется, а `abort` буфер не сбрасывает. Поставил
   `setvbuf(stdout, NULL, _IONBF, 0)`, теперь строка «фаззинг: …» видна всегда.
