# Практика 2. Закрыть белые пятна

Продолжаю в `~/health-lab` после практики 1. Мой файл: [test_greeting.c](test_greeting.c),
это `tests/test_greeting.c` из практики 1 плюс тест на «громкое» приветствие.

## 1. Первый запуск make coverage

```text
$ make coverage; echo "код: $?"
rm -rf build/cov
mkdir -p build/cov
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude --coverage -c tests/test_greeting.c -o build/cov/test_greeting.o
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude --coverage -c src/greeting.c -o build/cov/greeting.o
gcc --coverage build/cov/test_greeting.o build/cov/greeting.o -o build/cov/test
./build/cov/test
ok:   код возврата 0
...
ok:   кириллица
FAIL: длинное имя возвращает -2

1 проверок не прошло
make: *** [Makefile:37: coverage] Error 1
код: 2
```

До `gcov` не дошло: мой красный тест из практики 1 роняет рецепт на строке
`./build/cov/test`. Функцию чинить рано (это практика 4), поэтому запускаю с `-i`, чтобы
`make` шёл дальше после ошибки:

```text
$ make -i coverage 2>&1 | tail -n 14
build/cov/test-test_greeting.gcda:cannot open data file, assuming not executed
File 'src/greeting.c'
Lines executed:69.23% of 13
Branches executed:100.00% of 8
Taken at least once:87.50% of 8
No calls
Creating 'greeting.c.gcov'

File '/usr/include/x86_64-linux-gnu/bits/string_fortified.h'
--- непокрытые строки greeting.c ---
32:    #####:   22:        strcpy(out, "HELLO, ");
33:    #####:   23:        strcat(out, name + 1);
34:    #####:   24:        strcat(out, "!!!");
35:    #####:   25:        return 0;
```

До: 69.23% строк, 9 из 13. Мои два теста из практики 1 процент не сдвинули, они идут по
той же ветке, что и исходные.

## 2. Где белые пятна

```text
$ ls *.gcov
greeting.c.gcov
string_fortified.h.gcov
$ sed -n "20,40p" greeting.c.gcov
        -:   16:
        5:   17:    if (name == NULL || name[0] == '\0')
branch  0 taken 80% (fallthrough)
branch  1 taken 20%
branch  2 taken 25% (fallthrough)
branch  3 taken 75%
        2:   18:        name = "world";
        -:   19:
        5:   20:    if (name[0] == '!') {
branch  0 taken 0% (fallthrough)
branch  1 taken 100%
        -:   21:        /* Новая функция: «HELLO, имя!!!» — тестов на неё нет. */
    #####:   22:        strcpy(out, "HELLO, ");
    #####:   23:        strcat(out, name + 1);
    #####:   24:        strcat(out, "!!!");
    #####:   25:        return 0;
        -:   26:    }
        -:   27:
        5:   28:    strcpy(out, "Hello, ");
        5:   29:    strcat(out, name);
        5:   30:    strcat(out, "!");
```

Все четыре `#####` в ветке «громкого» приветствия: ни одно имя в тестах не начинается с
`!`. У строки 20 видно то же по веткам: «branch 0 taken 0%». Пять вызовов, а не семь,
потому что вызовы с `out == NULL` и `cap == 0` выходят раньше, на строке 15.

Откуда `cannot open notes file`: в Makefile `gcov` передаётся
`build/cov/test-test_greeting.gcno`, а файл называется по объектному:

```text
$ ls build/cov
greeting.gcda
greeting.gcno
greeting.o
test
test_greeting.gcda
test_greeting.gcno
test_greeting.o
```

Для `greeting.c` это не мешает. Если передать `gcov` сами исходники, он находит всё и
заодно считает покрытие самого теста:

```text
$ gcov -o build/cov src/greeting.c tests/test_greeting.c | grep -A1 "^File"
File 'src/greeting.c'
Lines executed:100.00% of 13
--
File '/usr/include/x86_64-linux-gnu/bits/string_fortified.h'
Lines executed:100.00% of 6
--
File 'tests/test_greeting.c'
Lines executed:94.12% of 34
--
File '/usr/include/x86_64-linux-gnu/bits/stdio2.h'
Lines executed:100.00% of 2
```

(Этот запуск уже после шага 3, поэтому у `greeting.c` 100%.) В самом тесте не выполнились
строки «все проверки прошли»: пока тест на длинное имя красный, до них не доходит.

## 3. Тест на «!Аня»

```c
memset(buffer, 0, sizeof buffer);
check(make_greeting(buffer, sizeof buffer, "!Аня") == 0
      && strcmp(buffer, "HELLO, Аня!!!") == 0, "громкое приветствие");
```

```text
$ make -i coverage
rm -rf build/cov
mkdir -p build/cov
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude --coverage -c tests/test_greeting.c -o build/cov/test_greeting.o
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude --coverage -c src/greeting.c -o build/cov/greeting.o
gcc --coverage build/cov/test_greeting.o build/cov/greeting.o -o build/cov/test
./build/cov/test
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
FAIL: длинное имя возвращает -2

1 проверок не прошло
make: [Makefile:37: coverage] Error 1 (ignored)
gcov -b -o build/cov build/cov/test-test_greeting.gcno build/cov/greeting.gcno | head -8
build/cov/test-test_greeting.gcno:cannot open notes file
build/cov/test-test_greeting.gcda:cannot open data file, assuming not executed
File 'src/greeting.c'
Lines executed:100.00% of 13
Branches executed:100.00% of 8
Taken at least once:100.00% of 8
No calls
Creating 'greeting.c.gcov'

File '/usr/include/x86_64-linux-gnu/bits/string_fortified.h'
--- непокрытые строки greeting.c ---
$ grep -c "#####" greeting.c.gcov
0
```

После: 100.00% строк и 100% веток. Список непокрытых строк пустой.

## 4. Почему «!Аня» проходит, а длинное имя нет

«HELLO, Аня!!!» — это 16 байт (`Аня` в UTF-8 занимает 6), в буфер на 64 влезает.
Длинное имя идёт через строку 29 `strcat(out, name)`. По покрытию она выполнена 5 раз,
и один из них как раз с длинным именем. Покрытие видит, что строку запускали, но не видит,
с какими данными. Ошибка с длинным именем осталась на месте при 100%.

## Что понял

Покрытие — это карта того, куда тесты не заходили. Оно честно показало ветку без тестов, но
100% строк не значит «ошибок нет»: опасная строка была покрыта с самого начала.
