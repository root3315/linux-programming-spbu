# Практика 3. Генератор или руками

Продолжаю с файлами из практики 2. Мои версии: [calc.l](calc.l), [calc.y](calc.y) и
[mini_calc.c](mini_calc.c) (исходник из архива, `handwritten/mini_calc.c`, с моей степенью).
Опыты с поломками делал на копиях `~/gen-lab/broken` и `~/gen-lab/swap`, в решении их нет.

## Строки и отношение (шаги 1 и 2)

```text
stepan@devbox:~/gen-lab/Chapter10$ wc -l calc/calc.l calc/calc.y handwritten/mini_calc.c
  30 calc/calc.l
  59 calc/calc.y
 170 handwritten/mini_calc.c
 259 total
stepan@devbox:~/gen-lab/Chapter10$ for f in calc/calc.l calc/calc.y handwritten/mini_calc.c; do echo -n "$f "; tar -xzOf ../chapter10.tar.gz Chapter10/$f | wc -l; done
calc/calc.l 28
calc/calc.y 57
handwritten/mini_calc.c 170
stepan@devbox:~/gen-lab/Chapter10$ cd calc && make clean >/dev/null && make >/dev/null && wc -l lex.yy.c calc.tab.c calc.tab.h; cd ..
 1817 lex.yy.c
 1355 calc.tab.c
   77 calc.tab.h
 3249 total
stepan@devbox:~/gen-lab/Chapter10$ tar -xzOf ../chapter10.tar.gz Chapter10/calc/calc.l Chapter10/calc/calc.y | wc -l
85
stepan@devbox:~/gen-lab/Chapter10$ echo "170 85" | awk '{ printf "%.2f\n", $1 / $2 }'
2.00
```

Цифры прямо из архива: `calc.l` 28, `calc.y` 57, `mini_calc.c` 170. На слайде 30, 60 и 200.
Ручной вариант длиннее правил в 170 / 85 = 2 раза, а не в три с лишним, как вышло бы по слайду
(200 / 60). Сгенерированный код при этом больше правил в десятки раз: `lex.yy.c` 1817 строк при
30 в `calc.l` (с моими правками), `calc.tab.c` 1355.

## Степень во flex и bison (шаг 3)

```text
stepan@devbox:~/gen-lab/Chapter10$ diff -u /tmp/calc_p2.y calc/calc.y; diff -u /tmp/calc_p2l.l calc/calc.l
--- /tmp/calc_p2.y	2026-11-25 14:36:00.627245764 +0500
+++ calc/calc.y	2026-11-25 14:36:00.641990980 +0500
@@ -19,7 +19,8 @@
 
 %left '+' '-'
 %left '*' '/' '%'
-%precedence NEG                 /* унарный минус — самый старший */
+%precedence NEG                 /* унарный минус, старше * и / */
+%right '^'                      /* степень: старше унарного минуса */
 
 %%
 
@@ -41,6 +42,7 @@
     | expr '*' expr             { $$ = $1 * $3; }
     | expr '/' expr             { if ($3 == 0) { printf("делить на ноль нельзя\n"); $$ = 0; } else $$ = $1 / $3; }
     | expr '%' expr             { if ($3 == 0) { printf("делить на ноль нельзя\n"); $$ = 0; } else $$ = fmod($1, $3); }
+    | expr '^' expr             { $$ = pow($1, $3); }
     | '-' expr %prec NEG        { $$ = -$2; }
     | '(' expr ')'              { $$ = $2; }
     ;
--- /tmp/calc_p2l.l	2026-11-25 14:36:00.629380812 +0500
+++ calc/calc.l	2026-11-25 14:36:00.650057706 +0500
@@ -21,6 +21,7 @@
 "*"                 { return '*'; }
 "/"                 { return '/'; }
 "%"                 { return '%'; }
+"^"                 { return '^'; }
 "("                 { return '('; }
 ")"                 { return ')'; }
 "\n"                { return '\n'; }
stepan@devbox:~/gen-lab/Chapter10$ cd calc && make clean >/dev/null && make && bison -d -v calc.y; grep -ci conflict calc.output; rm -f calc.output
bison -d calc.y
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
0
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '%s\n' '2 ^ 10' '2 ^ 3 ^ 2' '-2 ^ 2' '(-2) ^ 2' '2 ^ -1' '2 * 3 ^ 2' '-2 * 3' '2 ^ 0.5' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 1024
= 512
= -4
= 4
= 0.5
= 18
= -6
= 1.41421
stepan@devbox:~/gen-lab/Chapter10/calc$ cd ..
```

Во flex одна строка. В bison две: `%right '^'` и правило с `pow`. `%right` значит, что `2^3^2` это
`2^(3^2)` = 512. `-2^2` решил считать как -4, как в математике: степень старше унарного минуса,
поэтому `%right '^'` стоит ниже `%precedence NEG` (в bison строки ниже старше). Комментарий у NEG
пришлось поправить, он больше не «самый старший». `2 ^ -1` тоже работает (0.5): `-1` это `expr`.
Конфликтов 0.

## Степень в ручном разборе (шаг 3)

```text
stepan@devbox:~/gen-lab/Chapter10$ diff -u /tmp/mini_p2.c handwritten/mini_calc.c
--- /tmp/mini_p2.c	2026-11-25 14:36:01.792783997 +0500
+++ handwritten/mini_calc.c	2026-11-25 14:36:01.850972755 +0500
@@ -15,6 +15,7 @@
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
+#include <math.h>
 
 /** Что за слово нам встретилось. */
 typedef enum {
@@ -23,6 +24,7 @@
     T_MINUS, /**< - */
     T_STAR,  /**< * */
     T_SLASH, /**< / */
+    T_POW,   /**< ^ */
     T_LP,    /**< ( */
     T_RP,    /**< ) */
     T_END,   /**< конец строки */
@@ -69,6 +71,7 @@
     case '-': t.kind = T_MINUS; break;
     case '*': t.kind = T_STAR;  break;
     case '/': t.kind = T_SLASH; break;
+    case '^': t.kind = T_POW;   break;
     case '(': t.kind = T_LP;    break;
     case ')': t.kind = T_RP;    break;
     default:  t.kind = T_BAD;   break;
@@ -81,9 +84,10 @@
 static void advance(void) { current = next_token(); }
 
 static double parse_expr(void);
+static double parse_unary(void);
 
 /**
- * @brief Разобрать «кирпичик»: число, скобку или унарный минус.
+ * @brief Разобрать «кирпичик»: число или выражение в скобках.
  */
 static double parse_atom(void)
 {
@@ -92,10 +96,6 @@
         advance();
         return v;
     }
-    if (current.kind == T_MINUS) {
-        advance();
-        return -parse_atom();
-    }
     if (current.kind == T_LP) {
         advance();
         double v = parse_expr();
@@ -107,18 +107,44 @@
 }
 
 /**
+ * @brief Степень. Правая ассоциативность: 2^3^2 это 2^(3^2).
+ */
+static double parse_pow(void)
+{
+    double base = parse_atom();
+
+    if (current.kind == T_POW) {
+        advance();
+        return pow(base, parse_unary());
+    }
+    return base;
+}
+
+/**
+ * @brief Унарный минус. Младше степени: -2^2 это -(2^2).
+ */
+static double parse_unary(void)
+{
+    if (current.kind == T_MINUS) {
+        advance();
+        return -parse_unary();
+    }
+    return parse_pow();
+}
+
+/**
  * @brief Умножение и деление — старше сложения.
  */
 static double parse_term(void)
 {
-    double v = parse_atom();
+    double v = parse_unary();
     for (;;) {
         if (current.kind == T_STAR) {
             advance();
-            v *= parse_atom();
+            v *= parse_unary();
         } else if (current.kind == T_SLASH) {
             advance();
-            double d = parse_atom();
+            double d = parse_unary();
             if (d == 0) {
                 printf("делить на ноль нельзя\n");
                 return 0;
stepan@devbox:~/gen-lab/Chapter10$ cd handwritten && make
gcc -Wall -Wextra -O2 -o mini_calc mini_calc.c -lm
stepan@devbox:~/gen-lab/Chapter10/handwritten$ printf '%s\n' '2 ^ 10' '2 ^ 3 ^ 2' '-2 ^ 2' '(-2) ^ 2' '2 ^ -1' '2 * 3 ^ 2' '-2 * 3' '2 ^ 0.5' | ./mini_calc
калькулятор без flex и bison: строка — Enter — результат
= 1024
= 512
= -4
= 4
= 0.5
= 18
= -6
= 1.41421
stepan@devbox:~/gen-lab/Chapter10/handwritten$ cd ..
stepan@devbox:~/gen-lab/Chapter10$ wc -l calc/calc.l calc/calc.y handwritten/mini_calc.c
  31 calc/calc.l
  61 calc/calc.y
 196 handwritten/mini_calc.c
 288 total
```

Здесь больше работы. Нужны токен `T_POW`, ветка в `next_token`, две новые функции `parse_pow` и
`parse_unary`, унарный минус пришлось вынести из `parse_atom` в `parse_unary` (иначе `-2^2` было бы
4, а не -4) и заменить три вызова `parse_atom` в `parse_term` на `parse_unary`. Файл вырос со 170 до
196 строк, `calc.y` на 2 строки, `calc.l` на 1. На тех же восьми выражениях ответы совпали. Степень
проще в генераторе.

## Убрать %left (шаг 4)

```text
stepan@devbox:~/gen-lab/Chapter10$ rm -rf ~/gen-lab/broken && cp -r calc ~/gen-lab/broken && cd ~/gen-lab/broken
stepan@devbox:~/gen-lab/broken$ sed -i '/^%left/d' calc.y && grep -n '^%' calc.y
7:%{
14:%}
16:%define api.value.type {double}   /* у всех значений тип double */
18:%token NUM
20:%precedence NEG                 /* унарный минус, старше * и / */
21:%right '^'                      /* степень: старше унарного минуса */
23:%%
48:%%
stepan@devbox:~/gen-lab/broken$ make clean >/dev/null; make
bison -d calc.y
calc.y: warning: 40 shift/reduce conflicts [-Wconflicts-sr]
calc.y: note: rerun with option '-Wcounterexamples' to generate conflict counterexamples
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
stepan@devbox:~/gen-lab/broken$ printf '%s\n' '2 + 3 * 4' '2 * 3 + 4' '10 - 2 - 3' '8 / 4 / 2' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 14
= 14
= 11
= 4
stepan@devbox:~/gen-lab/broken$ bison -d -Wcounterexamples calc.y 2>&1 | head -30
calc.y: warning: 40 shift/reduce conflicts [-Wconflicts-sr]
calc.y: warning: shift/reduce conflict on token '+' [-Wcounterexamples]
  Example: '-' expr • '+' expr
  Shift derivation
    expr
    ↳ 13: '-' expr
              ↳ 7: expr • '+' expr
  Reduce derivation
    expr
    ↳ 7: expr             '+' expr
         ↳ 13: '-' expr •
calc.y: warning: shift/reduce conflict on token '-' [-Wcounterexamples]
  Example: '-' expr • '-' expr
  Shift derivation
    expr
    ↳ 13: '-' expr
              ↳ 8: expr • '-' expr
  Reduce derivation
    expr
    ↳ 8: expr             '-' expr
         ↳ 13: '-' expr •
calc.y: warning: shift/reduce conflict on token '*' [-Wcounterexamples]
  Example: '-' expr • '*' expr
  Shift derivation
    expr
    ↳ 13: '-' expr
              ↳ 9: expr • '*' expr
  Reduce derivation
    expr
    ↳ 9: expr             '*' expr
stepan@devbox:~/gen-lab/broken$ cd ~/gen-lab/Chapter10
```

Удалил обе строки `%left`, а `%precedence NEG` и `%right '^'` оставил. bison пишет
`40 shift/reduce conflicts`, но сборка проходит: это предупреждение, не ошибка. С
`-Wcounterexamples` он показывает примеры вроде `'-' expr • '+' expr`: после унарного минуса
неясно, сворачивать выражение или читать `+` дальше. Конфликт bison решает сдвигом, и все
операции становятся правоассоциативными и без приоритета: `2 * 3 + 4` даёт 14 вместо 10,
`10 - 2 - 3` даёт 11 вместо 5, `8 / 4 / 2` даёт 4 вместо 1. `2 + 3 * 4` = 14 вышло правильно
случайно.

## Поменять вызов в parse_expr (шаг 5)

```text
stepan@devbox:~/gen-lab/Chapter10$ rm -rf ~/gen-lab/swap && cp -r handwritten ~/gen-lab/swap && cd ~/gen-lab/swap
stepan@devbox:~/gen-lab/swap$ grep -n 'parse_term()' mini_calc.c
164:    double v = parse_term();
168:            v += parse_term();
171:            v -= parse_term();
stepan@devbox:~/gen-lab/swap$ sed -i '/^static double parse_expr(void)$/,/^}/s/parse_term()/parse_atom()/' mini_calc.c && diff -u ~/gen-lab/Chapter10/handwritten/mini_calc.c mini_calc.c
--- /home/stepan/gen-lab/Chapter10/handwritten/mini_calc.c	2026-11-25 14:36:01.850972755 +0500
+++ mini_calc.c	2026-11-25 14:36:09.703752630 +0500
@@ -161,14 +161,14 @@
  */
 static double parse_expr(void)
 {
-    double v = parse_term();
+    double v = parse_atom();
     for (;;) {
         if (current.kind == T_PLUS) {
             advance();
-            v += parse_term();
+            v += parse_atom();
         } else if (current.kind == T_MINUS) {
             advance();
-            v -= parse_term();
+            v -= parse_atom();
         } else {
             return v;
         }
stepan@devbox:~/gen-lab/swap$ make && printf '%s\n' '2 + 3 * 4' '(2 + 3) * 4' '10 - 2 - 3' | ./mini_calc
gcc -Wall -Wextra -O2 -o mini_calc mini_calc.c -lm
mini_calc.c:138:15: warning: ‘parse_term’ defined but not used [-Wunused-function]
  138 | static double parse_term(void)
      |               ^~~~~~~~~~
калькулятор без flex и bison: строка — Enter — результат
= 5
= 5
= 5
stepan@devbox:~/gen-lab/swap$ cd ~/gen-lab/Chapter10
```

Заменил вызов в `parse_expr`: теперь он зовёт `parse_atom` вместо `parse_term`. `2 + 3 * 4` даёт 5,
`(2 + 3) * 4` тоже 5: разбор перестал видеть `*`, остаток строки молча выброшен. Ошибки нет. Только
gcc заметил, что `parse_term` нигде не вызывается.

## Поменять вызов в main (шаг 5)

```text
stepan@devbox:~/gen-lab/Chapter10$ rm -rf ~/gen-lab/swap && cp -r handwritten ~/gen-lab/swap && cd ~/gen-lab/swap
stepan@devbox:~/gen-lab/swap$ sed -i '/^int main/,$s/double v = parse_expr();/double v = parse_term();/' mini_calc.c && diff -u ~/gen-lab/Chapter10/handwritten/mini_calc.c mini_calc.c
--- /home/stepan/gen-lab/Chapter10/handwritten/mini_calc.c	2026-11-25 14:36:01.850972755 +0500
+++ mini_calc.c	2026-11-25 14:36:11.123241287 +0500
@@ -188,7 +188,7 @@
             printf("= пусто\n");
             continue;
         }
-        double v = parse_expr();
+        double v = parse_term();
         printf("= %g\n", v);
     }
 
stepan@devbox:~/gen-lab/swap$ make && printf '%s\n' '2 + 3 * 4' '2 * 3 + 4' | ./mini_calc
gcc -Wall -Wextra -O2 -o mini_calc mini_calc.c -lm
калькулятор без flex и bison: строка — Enter — результат
= 2
= 6
stepan@devbox:~/gen-lab/swap$ cd ~/gen-lab/Chapter10
```

Другая замена: `main` зовёт `parse_term` вместо `parse_expr`. `2 + 3 * 4` даёт 2, `2 * 3 + 4` даёт 6.
Компилятор молчит.

## Плохой ввод в обоих вариантах

```text
stepan@devbox:~/gen-lab/Chapter10$ printf '%s\n' '2 + + 3' '1 / 0' '2 +' '(2 + 3' '2 $ 3' 'abc' | calc/calc
ошибка: syntax error
ошибка: syntax error
ошибка: syntax error
ошибка: syntax error
ошибка: syntax error
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
не понял, попробуйте ещё
делить на ноль нельзя
= 0
не понял, попробуйте ещё
не понял, попробуйте ещё
не понял, попробуйте ещё
не понял, попробуйте ещё
stepan@devbox:~/gen-lab/Chapter10$ printf '%s\n' '2 + + 3' '1 / 0' '2 +' '(2 + 3' '2 $ 3' 'abc' | handwritten/mini_calc
калькулятор без flex и bison: строка — Enter — результат
= 5
делить на ноль нельзя
= 0
= 2
= 5
= 2
= 0
```

Версия на bison на каждую плохую строку отвечает «не понял». Ручная считает молча: `2 + + 3` даёт 5,
`2 +` даёт 2, `(2 + 3` даёт 5 (закрывающей скобки нет, не заметила), `2 $ 3` даёт 2, `abc` даёт 0.
Деление на ноль переживают обе. Это строка «Ошибки в разборе» из таблицы на слайде.

## Вывод

На калькуляторе ручной разбор короче, чем обещал слайд (170 строк, не 200), и читается легко. Но
новая операция в нём это новая функция, а ошибки ввода он глотает молча. Для растущей грамматики
выбрал бы flex и bison: приоритеты там задаются строками, а поломка видна сразу, предупреждением.
