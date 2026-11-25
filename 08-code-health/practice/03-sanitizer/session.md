# Практика 3. Санитайзер

Продолжаю в `~/health-lab`, тесты из практик 1–2 на месте (в том числе красный тест на
длинное имя). Своих файлов в этой практике нет, только команды.

## 1. make sanitize

По слайду тут тесты проходят и отчёта нет. У меня вышло по-другому:

```text
$ make sanitize; echo "код: $?"
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer -o build/sanitize-test tests/test_greeting.c src/greeting.c
./build/sanitize-test
=================================================================
==477==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x72f5a5700070 at pc 0x72f5a83f6923 bp 0x7ffe80792cc0 sp 0x7ffe80792468
WRITE of size 65 at 0x72f5a5700070 thread T0
    #0 0x72f5a83f6922 in strcpy ../../../../src/libsanitizer/asan/asan_interceptors.cpp:563
    #1 0x616be7a2b89a in strcat /usr/include/x86_64-linux-gnu/bits/string_fortified.h:130
    #2 0x616be7a2b89a in make_greeting src/greeting.c:29
    #3 0x616be7a2b57c in main tests/test_greeting.c:54
...
Address 0x72f5a5700070 is located in stack of thread T0 at offset 112 in frame
    #0 0x616be7a2b28f in main tests/test_greeting.c:24

  This frame has 2 object(s):
    [48, 112) 'buffer' (line 25)
    [144, 209) 'long_name' (line 51) <== Memory access at offset 112 partially underflows this variable
...
==477==ABORTING
make: *** [Makefile:45: sanitize] Error 1
код: 2
```

Причина в моём тесте на 64 буквы из практики 1. Без санитайзера он просто давал FAIL, а
под ASan упал на первой же записи за край. Чтобы увидеть то, что на слайде, собрал тот же
санитайзер с исходными тестами из архива:

```text
$ gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer -o build/sanitize-orig ~/Chapter08/project/tests/test_greeting.c src/greeting.c && ./build/sanitize-orig; echo "код: $?"
ok:   код возврата 0
ok:   обычное имя
ok:   NULL допустим
ok:   NULL заменяется на world
ok:   пустое имя допустимо
ok:   пустое имя заменяется
ok:   out == NULL отклоняется
ok:   cap == 0 отклоняется

все проверки прошли (но это не значит, что код без ошибок)
код: 0
```

Молчит. Ошибка в коде та же, но короткие имена до неё не доводят.

## 2. Программа с длинным именем

Первая попытка запустить `./build/sanitize-app` дала код 127: файла нет. Он не собрался,
потому что в рецепте `sanitize` идёт после тестов, а на тестах `make` остановился.

```text
$ ls build/
cov
sanitize-orig
sanitize-test
$ make -i sanitize 2>&1 | tail -n 3
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer -o build/sanitize-app src/main.c src/greeting.c
echo "тесты под санитайзером прошли (но они всё ещё не проверяют длинные имена)"
тесты под санитайзером прошли (но они всё ещё не проверяют длинные имена)
```

Последняя строка тут врёт: с `-i` `make` напечатал её, хотя тесты упали.

```text
$ ./build/sanitize-app "$(python3 -c "print(\"A\"*100)")" 2>&1 | head -n 17; echo "код: ${PIPESTATUS[0]}"
=================================================================
==538==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7c8e15400060 at pc 0x7c8e18076923 bp 0x7ffe728c1bc0 sp 0x7ffe728c1368
WRITE of size 101 at 0x7c8e15400060 thread T0
    #0 0x7c8e18076922 in strcpy ../../../../src/libsanitizer/asan/asan_interceptors.cpp:563
    #1 0x5acd2f29d6aa in strcat /usr/include/x86_64-linux-gnu/bits/string_fortified.h:130
    #2 0x5acd2f29d6aa in make_greeting src/greeting.c:29
    #3 0x5acd2f29d376 in main src/main.c:13
    #4 0x7c8e177d01c9  (/lib/x86_64-linux-gnu/libc.so.6+0x2a1c9) (BuildId: 8cebda42d140fc2b09639f8b13a5d5f2e617ef09)
    #5 0x7c8e177d028a in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x2a28a) (BuildId: 8cebda42d140fc2b09639f8b13a5d5f2e617ef09)
    #6 0x5acd2f29d564 in _start (/home/stepan/health-lab/build/sanitize-app+0x2564) (BuildId: 5e4e26710a29fa678d7602e235422f108dd7fc7c)

Address 0x7c8e15400060 is located in stack of thread T0 at offset 96 in frame
    #0 0x5acd2f29d2af in main src/main.c:9

  This frame has 1 object(s):
    [32, 96) 'buffer' (line 10) <== Memory access at offset 96 overflows this variable
HINT: this may be a false positive if your program uses some custom stack unwind mechanism, swapcontext or vfork
код: 141
```

Код 141 — это 128 + 13 (SIGPIPE): `head` закрыл pipe раньше, чем программа дописала
отчёт. Настоящий код смотрел отдельно, с отчётом в файл:

```text
$ ./build/sanitize-app "$(python3 -c "print(\"A\"*100)")" 2> asan.txt; echo "код: $?"
код: 1
$ grep -c . asan.txt
49
```

ASan по умолчанию завершает программу с кодом 1. В отчёте 49 непустых строк, выше первые 17.

## 3. Как читаю отчёт

- `stack-buffer-overflow`: запись за край массива на стеке.
- `WRITE of size 101`: `strcat` копировал 100 букв и `'\0'`.
- `#2 make_greeting src/greeting.c:29`: первая строка с моим кодом, это `strcat(out, name)`.
  Кадры `#0` и `#1` внутри библиотеки, `#3` показывает, откуда вызвали.
- `[32, 96) 'buffer' (line 10)`: пострадал `char buffer[64]` из `main`, он занимает байты
  32…95 кадра, а запись пришла в 96, сразу за его концом.

Чем опасна строка 29: `strcat` пишет, пока не встретит `'\0'` в имени, и не знает
размер `out`. Размер буфера в `make_greeting` передаётся (`cap`), но не используется.

## 4. Без санитайзера

```text
$ make app
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/main.c -o src/main.o
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/greeting.c -o src/greeting.o
gcc -o app src/main.o src/greeting.o
$ ./app "$(python3 -c "print(\"A\"*100)")"; echo "код: $?"
*** stack smashing detected ***: terminated
bash: line 1:   578 Aborted                 (core dumped) ./app "$(python3 -c "print(\"A\"*100)")"
код: 134
```

При 100 буквах программа всё-таки упала, но не на записи, а при выходе из `main`: gcc в
Ubuntu по умолчанию ставит защиту стека, и она заметила испорченную «канарейку». Где была
ошибка, отсюда не узнать. Решил проверить переполнение поменьше:

```text
$ ./app "$(python3 -c "print(\"A\"*60)")"; echo "код: $?"
Hello, AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA!
код: 0
$ ./build/sanitize-app "$(python3 -c "print(\"A\"*60)")" 2>&1 | grep -E "ERROR|WRITE|greeting.c"
==593==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x77af30400060 at pc 0x77af331e3923 bp 0x7ffc2d97d520 sp 0x7ffc2d97ccc8
WRITE of size 61 at 0x77af30400060 thread T0
    #2 0x5686791456aa in make_greeting src/greeting.c:29
```

60 букв: `"Hello, "` + 60 + `"!"` + `'\0'` = 69 байт в буфер на 64. Обычная сборка
напечатала приветствие и вернула 0. Это самый плохой вариант со слайда: ошибка есть,
сигнала нет. Санитайзер на тех же данных сразу показал строку 29.

## 5. Почему без санитайзера ошибку не видно

Запись за край массива в C ничем не проверяется: байты просто ложатся в соседнюю память
стека. Если там ничего важного (или канарейку не задели), программа работает дальше.
ASan ставит вокруг каждого буфера «забор» из отравленных байт и проверяет каждую запись,
поэтому останавливает программу на первом же касании.

## Что понял

Санитайзер ловит порчу памяти точно, со строкой и размером, но только если программа
прошла через опасное место с плохими данными. С исходными тестами он молчит. Мой тест на
длинное имя дал ему такие данные, поэтому `make sanitize` у меня красный уже до фаззера.
