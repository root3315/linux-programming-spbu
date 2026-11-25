# Практика 2. Операция %

Продолжаю с файлами из практики 1. Правлю оба: [calc.l](calc.l) и [calc.y](calc.y).

## Первая версия: только calc.y, без math.h

```text
stepan@devbox:~/gen-lab/Chapter10/calc$ diff -u /tmp/calc_p1.y calc.y
--- /tmp/calc_p1.y	2026-11-25 14:35:19.434616355 +0500
+++ calc.y	2026-11-25 14:35:19.626030199 +0500
@@ -17,7 +17,7 @@
 %token NUM
 
 %left '+' '-'
-%left '*' '/'
+%left '*' '/' '%'
 %precedence NEG                 /* унарный минус — самый старший */
 
 %%
@@ -39,6 +39,7 @@
     | expr '-' expr             { $$ = $1 - $3; }
     | expr '*' expr             { $$ = $1 * $3; }
     | expr '/' expr             { if ($3 == 0) { printf("делить на ноль нельзя\n"); $$ = 0; } else $$ = $1 / $3; }
+    | expr '%' expr             { $$ = fmod($1, $3); }
     | '-' expr %prec NEG        { $$ = -$2; }
     | '(' expr ')'              { $$ = $2; }
     ;
stepan@devbox:~/gen-lab/Chapter10/calc$ make clean && make
rm -f calc calc.tab.c calc.tab.h lex.yy.c
rm -rf docs/html
bison -d calc.y
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
calc.y: In function ‘yyparse’:
calc.y:42:43: warning: implicit declaration of function ‘fmod’ [-Wimplicit-function-declaration]
   42 |     | expr '%' expr             { $$ = fmod($1, $3); }
      |                                           ^~~~
calc.y:10:1: note: include ‘<math.h>’ or provide a declaration of ‘fmod’
    9 | #include <stdlib.h>
  +++ |+#include <math.h>
   10 | 
calc.y:42:43: warning: incompatible implicit declaration of built-in function ‘fmod’ [-Wbuiltin-declaration-mismatch]
   42 |     | expr '%' expr             { $$ = fmod($1, $3); }
      |                                           ^~~~
calc.y:42:43: note: include ‘<math.h>’ or provide a declaration of ‘fmod’
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '%s\n' '10 % 3' '10 / 3' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 1
= 3.33333
```

Собралось, но gcc 13 ругается на `implicit declaration of function 'fmod'`. Бинарник работает:
`10 % 3` даёт 1. Насколько знаю, в gcc 14 неявное объявление уже ошибка, так что `#include <math.h>`
нужен в любом случае. `-lm` в Makefile уже есть.

`10 / 3` печатает 3.33333, а не 3.33, как на слайде: `%g` показывает шесть значащих цифр.

## Подключить math.h

```text
stepan@devbox:~/gen-lab/Chapter10/calc$ diff -u /tmp/calc_a1.y calc.y
--- /tmp/calc_a1.y	2026-11-25 14:35:22.220213447 +0500
+++ calc.y	2026-11-25 14:35:22.235109290 +0500
@@ -7,6 +7,7 @@
 %{
 #include <stdio.h>
 #include <stdlib.h>
+#include <math.h>
 
 int yylex(void);
 void yyerror(const char *s);
stepan@devbox:~/gen-lab/Chapter10/calc$ make clean && make
rm -f calc calc.tab.c calc.tab.h lex.yy.c
rm -rf docs/html
bison -d calc.y
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '%s\n' '10 % 3' '10 / 3' '7 % 2 * 3' '2 + 10 % 4' '-7 % 3' '5.5 % 2' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 1
= 3.33333
= 3
= 4
= -1
= 1.5
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '%s\n' '10 % 0' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= -nan
```

Предупреждений нет. Проверки: `7 % 2 * 3` равно 3 (`%` стоит на уровне `*` и `/`, считается слева
направо), `2 + 10 % 4` равно 4, `-7 % 3` равно -1 (`fmod` берёт знак делимого, как `%` в C),
`5.5 % 2` равно 1.5. `10 % 0` даёт `= -nan`: `fmod(10, 0)` возвращает NaN.

## Деление на ноль

```text
stepan@devbox:~/gen-lab/Chapter10/calc$ diff -u /tmp/calc_a2.y calc.y
--- /tmp/calc_a2.y	2026-11-25 14:35:24.400235839 +0500
+++ calc.y	2026-11-25 14:35:24.414816187 +0500
@@ -40,7 +40,7 @@
     | expr '-' expr             { $$ = $1 - $3; }
     | expr '*' expr             { $$ = $1 * $3; }
     | expr '/' expr             { if ($3 == 0) { printf("делить на ноль нельзя\n"); $$ = 0; } else $$ = $1 / $3; }
-    | expr '%' expr             { $$ = fmod($1, $3); }
+    | expr '%' expr             { if ($3 == 0) { printf("делить на ноль нельзя\n"); $$ = 0; } else $$ = fmod($1, $3); }
     | '-' expr %prec NEG        { $$ = -$2; }
     | '(' expr ')'              { $$ = $2; }
     ;
stepan@devbox:~/gen-lab/Chapter10/calc$ make
bison -d calc.y
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '%s\n' '10 % 0' '1 / 0' '10 % 3' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
делить на ноль нельзя
= 0
делить на ноль нельзя
= 0
= 1
```

Проверка такая же, как у `/`: сообщение и `= 0`.

## Конфликты bison

```text
stepan@devbox:~/gen-lab/Chapter10/calc$ bison -d -v calc.y; echo rc=$?; grep -ci conflict calc.output; rm -f calc.output
rc=0
0
```

bison молчит, `rc=0`. В отчёте `calc.output` слово `conflict` не встречается ни разу, grep
вернул 0.

## Правило в calc.l

```text
stepan@devbox:~/gen-lab/Chapter10/calc$ grep -n '"%"' calc.l
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '%s\n' '10 % 3' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 1
```

Без правила в `calc.l` `%` уже работает: его возвращает правило `.`. Слайд просит добавить
правило в лексер, строго оно не нужно. Добавил: явная запись читается лучше, и `.` остаётся для
настоящих ошибок.

## Пересборка с нуля

```text
stepan@devbox:~/gen-lab/Chapter10/calc$ diff -u /tmp/calc_a3.l calc.l
--- /tmp/calc_a3.l	2026-11-25 14:35:26.297120604 +0500
+++ calc.l	2026-11-25 14:35:26.317755866 +0500
@@ -20,6 +20,7 @@
 "-"                 { return '-'; }
 "*"                 { return '*'; }
 "/"                 { return '/'; }
+"%"                 { return '%'; }
 "("                 { return '('; }
 ")"                 { return ')'; }
 "\n"                { return '\n'; }
stepan@devbox:~/gen-lab/Chapter10/calc$ make clean && make
rm -f calc calc.tab.c calc.tab.h lex.yy.c
rm -rf docs/html
bison -d calc.y
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '%s\n' '10 % 3' '10 / 3' '10 % 0' '2 + + 3' '% 3' | ./calc
ошибка: syntax error
ошибка: syntax error
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 1
= 3.33333
делить на ноль нельзя
= 0
не понял, попробуйте ещё
не понял, попробуйте ещё
stepan@devbox:~/gen-lab/Chapter10/calc$ wc -l calc.l calc.y
  30 calc.l
  59 calc.y
  89 total
```

`make clean && make` без единого предупреждения. `% 3` и `2 + + 3` дают «не понял», программа не
падает. Теперь `calc.l` 30 строк, `calc.y` 59.

## Вывод

`%` это два места: токен в `calc.y` (строка приоритета и правило с `fmod`) и, по желанию, правило
в `calc.l`. Без `#include <math.h>` gcc 13 ещё собирает программу, но с предупреждением, и оно
легко теряется среди вывода сборки.
