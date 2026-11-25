# Отчёт по ДЗ 10: калькулятор с переменными и функциями

Проверял в Ubuntu 24.04: flex 2.6.4, bison 3.8.2, doxygen 1.9.8, gcc 13.3. Исходная версия
для сравнения — калькулятор из примера к лекции (`calc.l`, `calc.y`, `Makefile`), собранный так
же, как в примере. Новая версия собирается с `-std=c11 -Wall -Wextra -Werror`.

## 1. Что добавили в грамматику

- Токен `NAME` — имя: буква или `_`, дальше буквы, цифры, `_`. Значение токена теперь не
  `double`, а `%union` из числа (`NUM`) и строки (`NAME`).
- Присваивание: `NAME '=' expr '\n'`. Печатает `x = 5`.
- Переменная в выражении: `expr: NAME`.
- Вызов функции: `expr: NAME '(' expr ')'`. Имя функции ищется в `funcs.c` (`sqrt`, `abs`,
  `sin`, `cos`, `ln`).
- Остаток `%` (на одном уровне с `*` и `/`) и степень `^` (правая ассоциативность, выше
  унарного минуса, поэтому `-2 ^ 2` = -4). Оба оператора сверх условия.
- Деление и остаток на ноль дают ошибку вместо `= 0` из исходной версии.
- Ошибки: первая ошибка в строке печатается в stderr, результат строки не печатается, правило
  `error '\n'` пропускает остаток строки. Имена из лексера освобождает `%destructor`.
- Таблица переменных вынесена в `vars.c` и `vars.h`, функции в `funcs.c` и `funcs.h`,
  общие ошибки и `main` в `calc.c`. Грамматика только вызывает их.
- Лексер в конце ввода без перевода строки сам отдаёт `\n`, поэтому `2 + 3` без Enter считается.
- Bison: `bison -Wall -Wcounterexamples -Werror`, конфликтов нет, в `calc.output` слово
  `conflict` встречается 0 раз.

## 2. Какие строки правили (calc.l / calc.y)

Новых и изменённых строк (`diff исходный новый | grep -c '^>'`):

```text
calc.l: 48 (было 28 строк, стало 59)
calc.y: 33 (было 57 строк, стало 64)
```

`calc.l`:

- строки 16-31: `need_newline`, макрос `TOKEN` и `copy_name`;
- строка 34: `%option noyywrap noinput nounput nodefault` вместо `-Wno-unused-function` в Makefile;
- строки 38-47: число через `strtod` в `yylval.num`, правило `NAME`; имя длиннее 31 символа
  режется и даёт ошибку;
- строки 48-50: пробелы с `\r`, `\n` и «любой другой символ» через `TOKEN`;
- строки 51-57: правило `<<EOF>>`, которое отдаёт недостающий `\n`.

`calc.y`:

- строки 13-23: `%union`, типы `NUM`, `NAME`, `expr`, `%destructor`;
- строки 25-28: приоритеты, добавлены `%` и `%right '^'`;
- строки 38-41: четыре правила `line`, среди них присваивание;
- строки 46-47: `NAME` и `NAME '(' expr ')'`;
- строки 51-58: `/` и `%` с проверкой нуля, 59: `^`.

Сборка до и после. Было (флаг `-Wno-unused-function` прячет предупреждения):

```text
$ make
bison -d calc.y
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
```

Стало (`-Werror`, без исключений для сгенерированного кода):

```text
$ make clean && make
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -O2 -g -c -o calc.o calc.c
bison -Wall -Wcounterexamples -Werror -d calc.y
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -O2 -g -c -o calc.tab.o calc.tab.c
flex calc.l
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -O2 -g -c -o lex.yy.o lex.yy.c
...
gcc  -o calc calc.o calc.tab.o lex.yy.o vars.o funcs.o -lm
```

## 3. Примеры работы

До (исходная версия), каждая строка отдельным запуском. `tail -n +2` убирает приглашение:

```text
$ echo '2 + 3 * 4' | ./calc | tail -n +2
= 14
$ echo 'x = 5' | ./calc | tail -n +2
ошибка: syntax error
не понял, попробуйте ещё
$ echo 'sqrt(9)' | ./calc | tail -n +2
ошибка: syntax error
не понял, попробуйте ещё
$ echo '1 / 0' | ./calc | tail -n +2
делить на ноль нельзя
= 0
$ echo '7 % 3' | ./calc | tail -n +2
ошибка: syntax error
не понял, попробуйте ещё
```

После, переменные и функции:

```text
$ printf '%s\n' 'x = 5' '2 * x' 'y = x * 2 + 1' 'y' 'sqrt(9)' 'sqrt(x * 5)' | ./calc
x = 5
= 10
y = 11
= 11
= 3
= 5
```

После, приоритеты и ассоциативность на месте:

```text
$ printf '%s\n' '2 + 3 * 4' '(2 + 3) * 4' '10 - 4 - 3' '-2 ^ 2' '2 ^ 3 ^ 2' '7 % 3' | ./calc
= 14
= 20
= 3
= -4
= 512
= 1
```

После, ошибки ввода. Программа идёт дальше, код выхода 0, после ошибок последняя строка считается:

```text
$ printf '%s\n' 'y' 'sqrt(-1)' 'foo(1)' '2 + + 3' '1 / 0' '5 % 0' '2 + 3 * 4' | ./calc; echo "rc=$?"
ошибка: переменная не определена: y
ошибка: sqrt: аргумент -1 вне области определения
ошибка: неизвестная функция: foo
ошибка: синтаксическая ошибка
ошибка: деление на ноль
ошибка: деление на ноль
= 14
rc=0
```

Конец ввода без перевода строки, имя из 100000 букв и 200000 открывающих скобок:

```text
$ printf '2 + 3' | ./calc; echo "rc=$?"
= 5
rc=0
$ head -c 100000 /dev/zero | tr '\0' a | ./calc; echo "rc=$?"
ошибка: имя слишком длинное (максимум 31)
rc=0
$ head -c 200000 /dev/zero | tr '\0' '(' | ./calc; echo "rc=$?"
ошибка: memory exhausted
rc=2
```

На скобках bison упирается в свой предел стека (10000), печатает ошибку и выходит с кодом 2, без
падения.

Тесты и санитайзеры. `make test` запускает `test_vars`, `test_funcs` и `tests.sh` (49 случаев,
среди них 100000-буквенное имя, 5000 цифр в числе, 1100 переменных при пределе 1024, случайные
байты):

```text
$ make test
./test_vars
test_vars: все проверки прошли
./test_funcs
test_funcs: все проверки прошли
bash tests.sh ./calc
tests.sh: все 49 проверок прошли
```

То же под ASan и UBSan (`make clean && make test SANITIZE=1`), утечек и ошибок нет:

```text
$ make test SANITIZE=1
...
./test_vars
test_vars: все проверки прошли
./test_funcs
test_funcs: все проверки прошли
bash tests.sh ./calc
tests.sh: все 49 проверок прошли
```

## 4. Скриншот или список страниц сайта

Вместо скриншота список страниц. `make docs` (`doxygen Doxyfile`) отработал без
предупреждений, в Doxyfile стоит `WARN_AS_ERROR = YES`, то есть любая недокументированная
функция остановила бы сборку. В `docs/html/` получилось 28 страниц. Главные:

```text
index.html           главная страница (описание из calc.h, приоритеты)
files.html           список файлов
annotated.html       структуры
globals_func.html    все функции
calc_8h.html         calc.h
calc_8c.html         calc.c
vars_8h.html         vars.h
vars_8c.html         vars.c
funcs_8h.html        funcs.h
funcs_8c.html        funcs.c
...
```

Какие функции попали на страницы (`grep -l '>имя<' docs/html/*.html`, без `*_source.html`):

```text
calc_fail      calc_8c.html calc_8h.html
calc_failed    calc_8c.html calc_8h.html
calc_end_line  calc_8c.html calc_8h.html
calc_print     calc_8c.html calc_8h.html
calc_assign    calc_8c.html calc_8h.html
calc_lookup    calc_8c.html calc_8h.html index.html
calc_call      calc_8c.html calc_8h.html index.html
yyerror        calc_8c.html calc_8h.html
yyparse        calc_8h.html
yylex          calc_8h.html
yylex_destroy  calc_8h.html
main           calc_8c.html index.html
vars_new       vars_8c.html vars_8h.html
vars_free      vars_8c.html vars_8h.html
vars_set       vars_8c.html vars_8h.html
vars_get       vars_8c.html vars_8h.html
vars_count     vars_8c.html vars_8h.html
func_call      funcs_8c.html funcs_8h.html
```

`yyparse`, `yylex`, `yylex_destroy` описаны в `calc.h` как объявления: тела их генерируют
bison и flex, в сайт сгенерированные файлы не входят. Внутренние `static` функции
(`name_ok`, `find`, `non_negative`, `positive`) тоже описаны и есть на страницах `vars_8c.html`
и `funcs_8c.html`.

## 5. Что было непонятно

- Первая сборка упала на линковке: `multiple definition of 'yyparse'`. make по встроенному
  правилу сам сделал `calc.o` из `calc.y` (через `y.tab.c`), хотя рядом лежит `calc.c`.
  Помогла строка `.SUFFIXES:` в начале Makefile.
- doxygen не читает `.l` и `.y`. Описание грамматики лежит в комментарии `calc.h`
  (`@mainpage`), а сами функции действий вынес в `calc.c`, чтобы они попали на сайт.
- С `WARN_AS_ERROR = YES` doxygen сначала остановился на `struct vars` без описания, потом на
  `static`-переменных. Описал их. Ещё `__attribute__((format(...)))` у `calc_fail` в `calc.h`
  сбил его: функция в `calc.c` считалась неописанной. Лечится `PREDEFINED = "__attribute__(x)="`.
- `OUTPUT_LANGUAGE = Russian` в doxygen 1.9.8 пишет предупреждение, что перевод не обновлялся с
  1.8.15. Поставил `English`: подписи разделов на английском, мои описания на русском.
- Исключение для сгенерированного лексера не понадобилось: с `%option noinput nounput nodefault`
  `lex.yy.c` собирается с `-Wall -Wextra -Werror` без единого предупреждения.
