# Практика 4. Документация: doxygen

Продолжаю с файлами из практики 3. Мой [mini_calc.c](mini_calc.c): комментарий у `main` и функция
`safe_div` с `@param` и `@return`. `Doxyfile` в итоге не менялся (опыт с `EXTRACT_STATIC` ниже
откатил), поэтому в папку он не положен. Проверяю результат по html через `grep`, страницы
в браузере не открывал.

## Подготовка

```text
stepan@devbox:~/gen-lab$ cd ~/gen-lab/Chapter10
stepan@devbox:~/gen-lab/Chapter10$ (cd calc && make clean && make) 2>&1 | tail -3
bison -d calc.y
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
```

Сначала собираю `calc`: в `INPUT` указана папка `calc`, и doxygen возьмёт сгенерированные
`calc.tab.c`, `calc.tab.h` и `lex.yy.c`.

## doxygen как есть (шаг 1)

```text
stepan@devbox:~/gen-lab/Chapter10$ grep -v '^#' Doxyfile | grep -v '^$'
PROJECT_NAME           = "Калькулятор: flex, bison и вручную"
PROJECT_BRIEF          = "Учебный пример к лекции 10"
OUTPUT_DIRECTORY       = docs
INPUT                  = calc handwritten
FILE_PATTERNS          = *.c *.h
RECURSIVE              = NO
EXTRACT_ALL            = YES
EXTRACT_STATIC         = YES
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
HAVE_DOT               = NO
QUIET                  = YES
WARN_IF_UNDOCUMENTED   = NO
stepan@devbox:~/gen-lab/Chapter10$ doxygen Doxyfile; echo rc=$?
rc=0
stepan@devbox:~/gen-lab/Chapter10$ ls docs/html | grep -c html$; ls docs/html/*.html | head -30
41
docs/html/annotated.html
docs/html/calc_8tab_8c.html
docs/html/calc_8tab_8h.html
docs/html/calc_8tab_8h_source.html
docs/html/classes.html
docs/html/dir_8f52640648b42ec91e3ad888d5271144.html
docs/html/dir_b23ce9843a0cf83641620a63d26b700d.html
docs/html/files.html
docs/html/functions.html
docs/html/functions_vars.html
docs/html/globals.html
docs/html/globals_b.html
docs/html/globals_c.html
docs/html/globals_defs.html
docs/html/globals_e.html
docs/html/globals_enum.html
docs/html/globals_eval.html
docs/html/globals_f.html
docs/html/globals_func.html
docs/html/globals_i.html
docs/html/globals_m.html
docs/html/globals_n.html
docs/html/globals_p.html
docs/html/globals_r.html
docs/html/globals_s.html
docs/html/globals_t.html
docs/html/globals_type.html
docs/html/globals_u.html
docs/html/globals_vars.html
docs/html/globals_y.html
stepan@devbox:~/gen-lab/Chapter10$ ls docs/html | grep -E '^(calc|lex|mini)'
calc_8tab_8c.html
calc_8tab_8h.html
calc_8tab_8h_source.html
lex_8yy_8c.html
mini__calc_8c.html
```

Кроме строк, которые перечислены на слайде, в `Doxyfile` есть `EXTRACT_ALL = YES`. О нём на слайде
ничего нет, а из-за него в документацию попадает всё, даже без комментариев. `QUIET = YES`, поэтому
при успехе doxygen молчит. Получилось 41 html-файл. Страницы `calc_8tab_8c`, `calc_8tab_8h` и
`lex_8yy_8c` это сгенерированные файлы, `mini__calc_8c` это наш.

## Что на странице файла (шаг 2)

```text
stepan@devbox:~/gen-lab/Chapter10$ grep -o 'href="mini__calc_8c.html#[a-z0-9]*">[a-z_]*</a>' docs/html/mini__calc_8c.html | sed 's/.*>\(.*\)<.*/\1/' | sort -u
advance
current
main
next_token
p
parse_atom
parse_expr
parse_pow
parse_term
parse_unary
stepan@devbox:~/gen-lab/Chapter10$ grep -c 'Читает строки' docs/html/mini__calc_8c.html
0
stepan@devbox:~/gen-lab/Chapter10$ grep -o 'Степень\.[^<]*\|Умножение и деление[^<]*\|Сложение и вычитание[^<]*' docs/html/mini__calc_8c.html | sort -u
Сложение и вычитание — самый низкий приоритет. 
Сложение и вычитание — самый низкий приоритет.  
Степень. Правая ассоциативность: 2^3^2 это 2^(3^2). 
Степень. Правая ассоциативность: 2^3^2 это 2^(3^2).  
Умножение и деление — старше сложения. 
Умножение и деление — старше сложения.  
```

В списке функций файла: `next_token`, `parse_atom`, `parse_term`, `parse_expr`, как на слайде, а ещё
`parse_pow` и `parse_unary` из практики 3, `advance`, `main` и переменные `current` и `p` (это
следствие `EXTRACT_ALL`). Описания из `@brief` в html есть. У `main` описания нет: слов «Читает
строки» на странице 0, потому что комментария к `main` пока не было.

## @brief у main и пересборка (шаги 3 и 4)

```text
stepan@devbox:~/gen-lab/Chapter10$ diff -u /tmp/mini_p3.c handwritten/mini_calc.c
--- /tmp/mini_p3.c	2026-11-25 14:36:53.467825857 +0500
+++ handwritten/mini_calc.c	2026-11-25 14:36:53.578595605 +0500
@@ -175,6 +175,10 @@
     }
 }
 
+/**
+ * @brief Читает строки со стандартного ввода и печатает значение каждой.
+ * @return 0 после конца ввода
+ */
 int main(void)
 {
     char line[256];
stepan@devbox:~/gen-lab/Chapter10$ doxygen Doxyfile; echo rc=$?
rc=0
stepan@devbox:~/gen-lab/Chapter10$ grep -c 'Читает строки' docs/html/mini__calc_8c.html
2
stepan@devbox:~/gen-lab/Chapter10$ grep -o 'Читает строки[^<]*' docs/html/mini__calc_8c.html | sort -u
Читает строки со стандартного ввода и печатает значение каждой. 
Читает строки со стандартного ввода и печатает значение каждой.  
```

После `@brief` описание у `main` появилось на странице: два вхождения, в списке функций и в
подробном описании.

## Убрать EXTRACT_STATIC (шаг 5)

```text
stepan@devbox:~/gen-lab/Chapter10$ cp Doxyfile /tmp/Doxyfile.orig && sed -i '/^EXTRACT_STATIC/d' Doxyfile && diff -u /tmp/Doxyfile.orig Doxyfile
--- /tmp/Doxyfile.orig	2026-11-25 14:36:54.063824779 +0500
+++ Doxyfile	2026-11-25 14:36:54.066383160 +0500
@@ -9,7 +9,6 @@
 FILE_PATTERNS          = *.c *.h
 RECURSIVE              = NO
 EXTRACT_ALL            = YES
-EXTRACT_STATIC         = YES
 GENERATE_HTML          = YES
 GENERATE_LATEX         = NO
 HAVE_DOT               = NO
stepan@devbox:~/gen-lab/Chapter10$ rm -rf docs && doxygen Doxyfile; echo rc=$?
rc=0
stepan@devbox:~/gen-lab/Chapter10$ grep -o 'href="mini__calc_8c.html#[a-z0-9]*">[a-z_]*</a>' docs/html/mini__calc_8c.html | sed 's/.*>\(.*\)<.*/\1/' | sort -u
main
stepan@devbox:~/gen-lab/Chapter10$ grep -c 'next_token\|parse_atom\|parse_term\|parse_expr' docs/html/mini__calc_8c.html
0
```

Без `EXTRACT_STATIC` в списке остался один `main`: все `static` функции пропали, grep по их именам
даёт 0. Страница не пустая, как обещает слайд, потому что `main` не `static`.

## Функция с @param и @return (шаг 6)

```text
stepan@devbox:~/gen-lab/Chapter10$ diff -u /tmp/mini_p4a.c handwritten/mini_calc.c
--- /tmp/mini_p4a.c	2026-11-25 14:36:54.276819355 +0500
+++ handwritten/mini_calc.c	2026-11-25 14:36:54.355426973 +0500
@@ -133,6 +133,21 @@
 }
 
 /**
+ * @brief Поделить a на b без падения на нуле.
+ * @param a делимое
+ * @param b делитель; если 0, печатается сообщение
+ * @return a / b или 0, если b равно 0
+ */
+double safe_div(double a, double b)
+{
+    if (b == 0) {
+        printf("делить на ноль нельзя\n");
+        return 0;
+    }
+    return a / b;
+}
+
+/**
  * @brief Умножение и деление — старше сложения.
  */
 static double parse_term(void)
@@ -144,12 +159,7 @@
             v *= parse_unary();
         } else if (current.kind == T_SLASH) {
             advance();
-            double d = parse_unary();
-            if (d == 0) {
-                printf("делить на ноль нельзя\n");
-                return 0;
-            }
-            v /= d;
+            v = safe_div(v, parse_unary());
         } else {
             return v;
         }
stepan@devbox:~/gen-lab/Chapter10$ (cd handwritten && make && printf '%s\n' '1 / 0' '10 / 4' '2 + 6 / 3' | ./mini_calc)
gcc -Wall -Wextra -O2 -o mini_calc mini_calc.c -lm
калькулятор без flex и bison: строка — Enter — результат
делить на ноль нельзя
= 0
= 2.5
= 4
stepan@devbox:~/gen-lab/Chapter10$ rm -rf docs && doxygen Doxyfile; echo rc=$?
rc=0
stepan@devbox:~/gen-lab/Chapter10$ grep -o 'href="mini__calc_8c.html#[a-z0-9]*">[a-z_]*</a>' docs/html/mini__calc_8c.html | sed 's/.*>\(.*\)<.*/\1/' | sort -u
main
safe_div
stepan@devbox:~/gen-lab/Chapter10$ grep -o 'Поделить a на b[^<]*\|делимое\|делитель[^<]*\|a / b или 0[^<]*' docs/html/mini__calc_8c.html | sort -u
a / b или 0, если b равно 0 
Поделить a на b без падения на нуле. 
Поделить a на b без падения на нуле.  
делимое
делитель; если 0, печатается сообщение 
```

Сделал `safe_div` и подставил её в `parse_term` вместо проверки делителя на месте. Нарочно без
`static`, чтобы увидеть её и без `EXTRACT_STATIC`. Поведение то же: `1 / 0` печатает сообщение
и `= 0`, `10 / 4` даёт 2.5. Есть небольшая разница: раньше при нулевом делителе `parse_term`
сразу выходил, теперь разбор продолжается. На странице: краткое описание, строки «делимое» и
«делитель; если 0, печатается сообщение» из `@param` и текст из `@return`. Функция видна, хотя
`EXTRACT_STATIC` выключен.

## Вернуть EXTRACT_STATIC

```text
stepan@devbox:~/gen-lab/Chapter10$ cp /tmp/Doxyfile.orig Doxyfile && rm -rf docs && doxygen Doxyfile; echo rc=$?
rc=0
stepan@devbox:~/gen-lab/Chapter10$ grep -o 'href="mini__calc_8c.html#[a-z0-9]*">[a-z_]*</a>' docs/html/mini__calc_8c.html | sed 's/.*>\(.*\)<.*/\1/' | sort -u
advance
current
main
next_token
p
parse_atom
parse_expr
parse_pow
parse_term
parse_unary
safe_div
```

Все функции на месте, и `safe_div` тоже.

## Убрать комментарий у main (критерий)

```text
stepan@devbox:~/gen-lab/Chapter10$ cp -r ~/gen-lab/Chapter10 ~/gen-lab/nodoc && cd ~/gen-lab/nodoc
stepan@devbox:~/gen-lab/nodoc$ diff -u ~/gen-lab/Chapter10/handwritten/mini_calc.c handwritten/mini_calc.c
--- /home/stepan/gen-lab/Chapter10/handwritten/mini_calc.c	2026-11-25 14:36:54.355426973 +0500
+++ handwritten/mini_calc.c	2026-11-25 14:36:55.337018985 +0500
@@ -185,10 +185,6 @@
     }
 }
 
-/**
- * @brief Читает строки со стандартного ввода и печатает значение каждой.
- * @return 0 после конца ввода
- */
 int main(void)
 {
     char line[256];
stepan@devbox:~/gen-lab/nodoc$ rm -rf docs && doxygen Doxyfile; echo rc=$?
rc=0
stepan@devbox:~/gen-lab/nodoc$ grep -c 'Читает строки' docs/html/mini__calc_8c.html
0
stepan@devbox:~/gen-lab/nodoc$ grep -o 'href="mini__calc_8c.html#[a-z0-9]*">main</a>' docs/html/mini__calc_8c.html | sort -u
href="mini__calc_8c.html#a840291bc02cba5474a4cb46a9b9566fe">main</a>
stepan@devbox:~/gen-lab/nodoc$ cd ~/gen-lab/Chapter10
```

Убрал блок `/** ... */` перед `main` в копии. Описание исчезло (0 вхождений), но сама `main` в
списке осталась: из-за `EXTRACT_ALL = YES` doxygen показывает функцию и без комментария. То есть
описание живёт только в комментарии, и если комментарий устарел, сайт врёт вместе с ним.

## Калькулятор после правок

```text
stepan@devbox:~/gen-lab/Chapter10$ printf '%s\n' '2 + + 3' '1 / 0' '-2 ^ 2' | handwritten/mini_calc
калькулятор без flex и bison: строка — Enter — результат
= 5
делить на ноль нельзя
= 0
= -4
```

Бинарник после правок для doxygen работает как в практике 3, включая молчаливое `2 + + 3 = 5`.

## Вывод

doxygen берёт описания только из комментариев `/** ... */` и сам ничего не придумывает: убрал
комментарий, исчезло описание. Какие функции вообще попадут на сайт, решают `EXTRACT_ALL` и
`EXTRACT_STATIC`, и это надо знать заранее, иначе пустая или лишняя страница выглядит как поломка.
