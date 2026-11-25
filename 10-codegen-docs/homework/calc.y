/* calc.y — грамматика: из токенов собирает выражение и сразу считает его.
 *
 * Ошибки считает не грамматика, а calc.c: calc_fail помнит, что в строке уже
 * была ошибка, и результат такой строки не печатается.
 */
%{
#include <stdlib.h>
#include <math.h>

#include "calc.h"
%}

%union {
    double num;     /* число и значение выражения */
    char  *str;     /* имя; выделено в лексере, освобождается здесь */
}

%token <num> NUM
%token <str> NAME
%type  <num> expr

/* Если разбор бросает токен на ошибке, имя надо освободить. */
%destructor { free($$); } <str>

%left '+' '-'
%left '*' '/' '%'
%precedence NEG                 /* унарный минус выше умножения */
%right '^'                      /* степень выше унарного минуса: -2 ^ 2 = -4 */

%%

input:
      %empty
    | input line
    ;

line:
      '\n'                      { calc_end_line(); }
    | expr '\n'                 { if (!calc_failed()) calc_print($1); calc_end_line(); }
    | NAME '=' expr '\n'        { calc_assign($1, $3); free($1); calc_end_line(); }
    | error '\n'                { yyerrok; calc_end_line(); }
    ;

expr:
      NUM                       { $$ = $1; }
    | NAME                      { $$ = calc_lookup($1); free($1); }
    | NAME '(' expr ')'         { $$ = calc_call($1, $3); free($1); }
    | expr '+' expr             { $$ = $1 + $3; }
    | expr '-' expr             { $$ = $1 - $3; }
    | expr '*' expr             { $$ = $1 * $3; }
    | expr '/' expr             {
                                    if ($3 == 0) { calc_fail("деление на ноль"); $$ = 0; }
                                    else $$ = $1 / $3;
                                }
    | expr '%' expr             {
                                    if ($3 == 0) { calc_fail("деление на ноль"); $$ = 0; }
                                    else $$ = fmod($1, $3);
                                }
    | expr '^' expr             { $$ = pow($1, $3); }
    | '-' expr %prec NEG        { $$ = -$2; }
    | '(' expr ')'              { $$ = $2; }
    ;

%%
