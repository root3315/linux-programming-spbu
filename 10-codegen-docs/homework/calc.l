/* calc.l — лексер: разбивает ввод на числа, имена и знаки.
 *
 * Самое длинное совпадение выигрывает, при равенстве — верхнее правило.
 * Опции noinput и nounput убирают две неиспользуемые функции из lex.yy.c,
 * nodefault запрещает правило по умолчанию (flex сам проверит, что все символы покрыты).
 */
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calc.h"
#include "calc.tab.h"
#include "vars.h"

/* Последний отданный токен не '\n'? Тогда в конце ввода отдадим '\n' сами. */
static int need_newline;
#define TOKEN(t) (need_newline = ((t) != '\n'), (t))

/* Копия первых n символов s; без памяти выходим: продолжать нечем. */
static char *copy_name(const char *s, size_t n)
{
    char *p = malloc(n + 1);
    if (p == NULL) {
        fputs("ошибка: не хватило памяти\n", stderr);
        exit(EXIT_FAILURE);
    }
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}
%}

%option noyywrap noinput nounput nodefault

%%

[0-9]+(\.[0-9]+)?        { yylval.num = strtod(yytext, NULL); return TOKEN(NUM); }
[A-Za-z_][A-Za-z0-9_]*   {
                             size_t n = (size_t)yyleng;
                             if (n > VARS_NAME_MAX) {
                                 calc_fail("имя слишком длинное (максимум %d)", VARS_NAME_MAX);
                                 n = VARS_NAME_MAX;
                             }
                             yylval.str = copy_name(yytext, n);
                             return TOKEN(NAME);
                         }
[ \t\r]+                 { /* пробелы пропускаем */ }
\n                       { return TOKEN('\n'); }
.                        { return TOKEN(yytext[0]); }
<<EOF>>                  {
                             if (need_newline) {
                                 need_newline = 0;
                                 return '\n';
                             }
                             return 0;
                         }

%%
