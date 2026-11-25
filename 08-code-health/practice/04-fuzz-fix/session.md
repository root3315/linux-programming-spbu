# Практика 4. Фаззер, исправление, линтеры

Продолжаю в `~/health-lab`. Мой файл: [greeting.c](greeting.c), это `src/greeting.c` из
`Chapter08/project`, где `strcpy`/`strcat` заменены на `snprintf`. Тест на длинное имя с
ожиданием `-2` уже есть с практики 1.

## 1. make fuzz до исправления

```text
$ make fuzz 2> fuzz.txt; echo "код: $?"
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer -o fuzz_greeting fuzz/fuzz_greeting.c src/greeting.c
./fuzz_greeting 2000 1
код: 2
$ grep -E "^(фаззинг|==.*ERROR|WRITE)|greeting.c|fuzz_greeting.c|\x27out\x27" fuzz.txt
==610==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7f36e9900040 at pc 0x7f36ec597923 bp 0x7ffdcd73d1a0 sp 0x7ffdcd73c948
WRITE of size 30 at 0x7f36e9900040 thread T0
    #2 0x6345c90a5c5a in make_greeting src/greeting.c:29
    #3 0x6345c90a5756 in main fuzz/fuzz_greeting.c:40
    #0 0x6345c90a535f in main fuzz/fuzz_greeting.c:19
```

Та же строка 29, только буфер теперь фаззера: `char out[32]`. Запись 30 байт, значит
случайное имя было длиной 29.

Строки `фаззинг: 2000 попыток, зерно 1` нет нигде, ни на экране, ни в файле. `printf`
пишет в буфер, когда stdout не терминал, а ASan убивает программу через `abort`, и буфер
пропадает.

На какой попытке упало, фаззер не пишет. Нашёл перебором числа попыток при том же зерне:

```text
$ for n in $(seq 1 30); do ./fuzz_greeting $n 1 > /dev/null 2>&1 || { echo "первое падение при $n попытках"; break; }; done
первое падение при 1 попытках
```

На первой. Уже после исправления собрал фаззер со старым `greeting.c` из архива и
проверил другие зёрна:

```text
$ gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer -o /tmp/fuzz-old fuzz/fuzz_greeting.c ~/Chapter08/project/src/greeting.c
$ for seed in 1 2 3 42; do for n in $(seq 1 50); do /tmp/fuzz-old $n $seed > /dev/null 2>&1 || { echo "зерно $seed: упало на попытке $n"; break; }; done; done
зерно 1: упало на попытке 1
зерно 2: упало на попытке 1
зерно 3: упало на попытке 1
зерно 42: упало на попытке 1
```

Везде первая попытка. На слайде в одном месте «на девятой», в другом «1-я попытка»; у меня
так. Объяснение простое: в `out[32]` после `"Hello, "` и `"!"` помещается имя до 23
символов, а фаззер берёт длину от 0 до 60. Больше половины случайных имён не влезают.

## 2–3. snprintf и честный отказ

Обе ветки (обычная и «громкая») переписал на `snprintf` с размером буфера:

```c
int n;
if (name[0] == '!')
    n = snprintf(out, cap, "HELLO, %s!!!", name + 1);
else
    n = snprintf(out, cap, "Hello, %s!", name);

if (n < 0)
    return -1;
if ((size_t)n >= cap)
    return -2;   /* не влезло: в out обрезанная строка, но без выхода за край */
return 0;
```

`snprintf` пишет не больше `cap` байт вместе с `'\0'`, а возвращает, сколько символов
хотел записать. Если это `>= cap`, строка обрезана. `#include <string.h>` заменил на
`<stdio.h>`.

## 4–5. Тест на длинное имя, make test, make fuzz

Тест на 64 буквы с ожиданием `-2` написан в практике 1, теперь он зелёный:

```text
$ make test; echo "код: $?"
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -o tests/test_greeting tests/test_greeting.c src/greeting.c
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
код: 0
$ make fuzz; echo "код: $?"
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer -o fuzz_greeting fuzz/fuzz_greeting.c src/greeting.c
./fuzz_greeting 2000 1
фаззинг: 2000 попыток, зерно 1
проблем не найдено — но это не доказательство, что их нет
код: 0
```

Заодно остальное:

```text
$ make coverage 2>&1 | sed -n "/^File .src/,/^Taken/p"
File 'src/greeting.c'
Lines executed:100.00% of 11
Branches executed:100.00% of 12
Taken at least once:91.67% of 12
$ make sanitize 2>&1 | tail -n 2; echo "код: ${PIPESTATUS[0]}"
echo "тесты под санитайзером прошли (но они всё ещё не проверяют длинные имена)"
тесты под санитайзером прошли (но они всё ещё не проверяют длинные имена)
код: 0
$ ./build/sanitize-app "$(python3 -c "print(\"A\"*100)")"; echo "код: $?"
не удалось собрать приветствие (код -2)
код: 1
```

По веткам 91.67%: ветку `n < 0` не выбрал ни один тест. Чтобы `snprintf` вернул
отрицательное число, нужна ошибка кодирования, на обычных строках я её не вызову. Эту
дыру оставляю и знаю, почему.

## 6. make tidy

```text
$ make tidy; echo "код: $?"
cppcheck --enable=warning,style --quiet lint/issues.c
lint/issues.c:17:5: warning: Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead. [getsCalled]
    gets(line);               /* опасная функция: нет контроля длины */
    ^
lint/issues.c:21:40: error: Division by zero. [zerodiv]
    printf("среднее: %d\n", sum / parts);   /* деление на ноль */
                                       ^
lint/issues.c:20:17: note: Assignment 'parts=0', assigned value is 0
    int parts = 0;
                ^
lint/issues.c:21:40: note: Division by zero
    printf("среднее: %d\n", sum / parts);   /* деление на ноль */
                                       ^
lint/issues.c:18:5: error: Uninitialized variable: sum [legacyUninitvar]
    sum += (int)line[0];
    ^
нет build/compile_commands.json — сначала: cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
код: 0
```

В линтере осталось то же, что было: `make tidy` смотрит только на учебный
`lint/issues.c`, мой `greeting.c` он не проверяет. И код 0, хотя `cppcheck` нашёл две
ошибки: без `--error-exitcode` он всегда выходит с 0. Делаю `compile_commands.json` и
повторяю:

```text
$ cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null && make tidy 2>&1 | tail -n 6
Error while processing /home/stepan/health-lab/lint/issues.c.
/home/stepan/health-lab/lint/issues.c:17:5: error: call to undeclared function 'gets'; ISO C99 and later do not support implicit function declarations [clang-diagnostic-implicit-function-declaration]
   17 |     gets(line);               /* опасная функция: нет контроля длины */
      |     ^
Found compiler error(s).
нет build/compile_commands.json — сначала: cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

`clang-tidy` отработал, но в конце всё равно «нет build/compile_commands.json». В Makefile
написано `test -f ... && clang-tidy ... || echo "нет ..."`: `clang-tidy` нашёл ошибку и
вернул не 0, и сработала ветка после `||`. Сообщение врёт.

Свой код проверил руками:

```text
$ cppcheck --enable=warning,style --quiet src/ tests/ fuzz/; echo "код: $?"
код: 0
$ clang-tidy -p build src/greeting.c src/main.c 2>&1 | grep -E "warning:|generated|Suppressed"
2 warnings generated.
3 warnings generated.
/home/stepan/health-lab/src/greeting.c:20:13: warning: Call to function 'snprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'snprintf_s' in case of C11 [clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling]
/home/stepan/health-lab/src/greeting.c:22:13: warning: Call to function 'snprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'snprintf_s' in case of C11 [clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling]
/home/stepan/health-lab/src/main.c:15:9: warning: Call to function 'fprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'fprintf_s' in case of C11 [clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling]
```

`cppcheck` молчит. `clang-tidy` ругается на `snprintf` и даже на `fprintf` в `main.c` и
предлагает `snprintf_s` из необязательного приложения K стандарта C11. В glibc этих
функций нет, так что предупреждение для моего кода ложное. На старый `greeting.c` он
ругался на `strcpy` и `strcat` (`insecureAPI.strcpy`), и это было по делу.
В ДЗ это правило выключено в `.clang-tidy`, а `make tidy` проверяет весь проект с ненулевым
кодом при замечаниях.

## Что понял

Фаззер нашёл ошибку за одну попытку там, где восемь тестов молчали. После `snprintf` он
2000 раз не упал, но это хороший знак, а не доказательство. И цели в чужом Makefile
стоит читать: `make tidy` из архива зелёный всегда и проверяет не тот файл.
