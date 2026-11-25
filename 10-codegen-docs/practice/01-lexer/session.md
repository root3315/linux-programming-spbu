# Практика 1. Слова: flex

Ubuntu 24.04, пользователь `stepan`, flex 2.6.4, bison 3.8.2, gcc 13.3.0. Работаю в `~/gen-lab`.
Здесь правлю только `calc.l`, мой вариант лежит рядом: [calc.l](calc.l).

## Скачать, распаковать, собрать как есть

```text
stepan@devbox:~/gen-lab$ wget -q --tries=3 --timeout=30 https://www.smartmelio.ru/chapter10.tar.gz && ls -l chapter10.tar.gz | awk '{print $5, $9}'
5152 chapter10.tar.gz
stepan@devbox:~/gen-lab$ tar -xzvf chapter10.tar.gz
Chapter10/
Chapter10/README.md
Chapter10/calc/
Chapter10/calc/Makefile
Chapter10/calc/calc.l
Chapter10/calc/calc.y
Chapter10/handwritten/
Chapter10/handwritten/Makefile
Chapter10/handwritten/mini_calc.c
Chapter10/Doxyfile
Chapter10/.gitignore
stepan@devbox:~/gen-lab$ cd Chapter10
stepan@devbox:~/gen-lab/Chapter10$ file Doxyfile calc/* handwritten/*
Doxyfile:                Unicode text, UTF-8 text, with CRLF line terminators
calc/Makefile:           makefile script, Unicode text, UTF-8 text, with CRLF line terminators
calc/calc.l:             C source, Unicode text, UTF-8 text, with CRLF line terminators
calc/calc.y:             C source, Unicode text, UTF-8 text, with CRLF line terminators
handwritten/Makefile:    makefile script, ASCII text, with CRLF line terminators
handwritten/mini_calc.c: C source, Unicode text, UTF-8 text, with CRLF line terminators
stepan@devbox:~/gen-lab/Chapter10$ cd calc && make
bison -d calc.y
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '2+3*4\n' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 14
stepan@devbox:~/gen-lab/Chapter10/calc$ cd ..
```

Архив 5152 байта. Все файлы в нём с CRLF (`file` пишет `with CRLF line terminators`). Сборка
прошла и так, `printf '2+3*4\n' | ./calc` дал 14, так что для `make`, `flex` и `bison` CRLF не
мешает.

## Концы строк и размер сгенерированного кода

```text
stepan@devbox:~/gen-lab/Chapter10$ sed -i 's/\r$//' Doxyfile calc/calc.l calc/calc.y calc/Makefile handwritten/mini_calc.c handwritten/Makefile
stepan@devbox:~/gen-lab/Chapter10$ file calc/calc.l calc/calc.y
calc/calc.l: C source, Unicode text, UTF-8 text
calc/calc.y: C source, Unicode text, UTF-8 text
stepan@devbox:~/gen-lab/Chapter10$ cd calc
stepan@devbox:~/gen-lab/Chapter10/calc$ make clean && make
rm -f calc calc.tab.c calc.tab.h lex.yy.c
rm -rf docs/html
bison -d calc.y
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
stepan@devbox:~/gen-lab/Chapter10/calc$ wc -l calc.l calc.y lex.yy.c calc.tab.c
   28 calc.l
   57 calc.y
 1802 lex.yy.c
 1347 calc.tab.c
 3234 total
```

Свои файлы всё равно перевёл в LF, чтобы дифф и репозиторий не были забиты `^M`.

Строки не совпали со слайдом: `calc.l` 28 (на слайде 30), `calc.y` 57 (60), `lex.yy.c` 1802
(1794). Разница небольшая, но цифры со слайда я бы не цитировал. Главное верно: 28 строк правил
дают 1802 строки C, больше чем в 60 раз. Из 57 строк `calc.y` bison сделал 1347.

## Ошибки ввода до правок

```text
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '2 + + 3\n' | ./calc
ошибка: syntax error
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
не понял, попробуйте ещё
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '1 / 0\n' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
делить на ноль нельзя
= 0
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '2 $ 3\n' | ./calc
ошибка: syntax error
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
не понял, попробуйте ещё
```

`2 + + 3` и `2 $ 3` программу не роняют: bison пишет `ошибка: syntax error` в stderr, правило
`error '\n'` печатает «не понял, попробуйте ещё», разбор идёт дальше. Порядок строк в выводе
перепутан: stderr без буфера, а stdout в пайпе буферизуется и выходит позже. `1 / 0` печатает
сообщение и всё равно `= 0`: правило ставит `$$ = 0`, и строка печатается как обычно.

## Число 12 в трассе flex

```text
stepan@devbox:~/gen-lab/Chapter10/calc$ flex -d -o lex_dbg.c calc.l && gcc -w -o calc_dbg calc.tab.c lex_dbg.c -lm 2>&1 | head -3; printf '12\n' | ./calc_dbg 2>&1 | grep -v '^--(end' ; rm -f calc_dbg lex_dbg.c
--accepting rule at line 17 ("12")
--accepting rule at line 25 ("
")
--EOF (start condition 0)
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 12
```

`flex -d` собирает лексер с отладочной печатью. Правило со строки 17 (число) приняло `"12"`
целиком, правило со строки 25 приняло перевод строки. Это «самое длинное совпадение»: `[0-9]+` не
останавливается на первой цифре.

## Комментарий до правки

```text
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '2+2 # моё\n' | ./calc
ошибка: syntax error
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
```

Без правила `#` уходит в правило `.`, парсер получает неизвестный токен и пишет `syntax error`.

## Правило для комментария

```text
stepan@devbox:~/gen-lab/Chapter10/calc$ sed -i 's|^\.   |"#".*               { /* комментарий до конца строки */ }\n.   |' calc.l
stepan@devbox:~/gen-lab/Chapter10/calc$ diff -u /tmp/calc_orig.l calc.l
--- /tmp/calc_orig.l	2026-11-25 14:35:06.877136698 +0500
+++ calc.l	2026-11-25 14:35:14.264585583 +0500
@@ -23,6 +23,7 @@
 "("                 { return '('; }
 ")"                 { return ')'; }
 "\n"                { return '\n'; }
+"#".*               { /* комментарий до конца строки */ }
 .                   { return yytext[0]; }   /* всё остальное — как есть */
 
 %%
stepan@devbox:~/gen-lab/Chapter10/calc$ make
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '2+2 # моё\n' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 4
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '# целая строка\n7*6\n' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 42
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '# только комментарий\n' | ./calc
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
stepan@devbox:~/gen-lab/Chapter10/calc$ printf '12\n12 + 0\n1 2\n' | ./calc
ошибка: syntax error
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 12
= 12
не понял, попробуйте ещё
stepan@devbox:~/gen-lab/Chapter10/calc$ flex -d -o lex_dbg.c calc.l && gcc -w -o calc_dbg calc.tab.c lex_dbg.c -lm; printf '12\n' | ./calc_dbg 2>&1 | grep -v '^--(end'; rm -f calc_dbg lex_dbg.c
--accepting rule at line 17 ("12")
--accepting rule at line 25 ("
")
--EOF (start condition 0)
калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход
= 12
stepan@devbox:~/gen-lab/Chapter10/calc$ make clean && make
rm -f calc calc.tab.c calc.tab.h lex.yy.c
rm -rf docs/html
bison -d calc.y
flex calc.l
gcc -Wall -Wextra -O2 -Wno-unused-function -o calc calc.tab.c lex.yy.c -lm
stepan@devbox:~/gen-lab/Chapter10/calc$ wc -l calc.l lex.yy.c calc.tab.c
   29 calc.l
 1811 lex.yy.c
 1347 calc.tab.c
 3187 total
```

Правило `"#".*` забирает `#` и всё до конца строки, но не `\n`. Перевод строки остаётся
токеном, поэтому `2+2 # моё` считается как `2+2`. На слайде написано «строки, начинающиеся с #»,
а проверка там `2+2 # моё`, то есть комментарий в хвосте строки. Правило без привязки к началу
строки закрывает оба случая. Порядок относительно `.` не важен: `"#".*` длиннее, значит
выигрывает.

Проверки: `2+2 # моё` даёт 4, строка только из комментария ничего не печатает, `12 + 0` даёт 12,
а `1 2` даёт syntax error: два числа подряд. Трасса снова показывает `"12"` одним куском. После
пересборки с нуля `calc.l` 29 строк, `lex.yy.c` 1811.

## Вывод

Лексер правится одним правилом в `calc.l`. `lex.yy.c` пересоздаётся при каждой сборке
(`make clean` его удаляет), поэтому править его бессмысленно.
