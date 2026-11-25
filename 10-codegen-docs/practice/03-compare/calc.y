/* calc.y — «грамматика» калькулятора: из токенов собирает выражение.
 *
 * bison читает правила вида «выражение = число» и «выражение = выражение + выражение»
 * и строит разборщик. Слово слева от ':' — то, что получаем,
 * справа — из чего. В фигурных скобках — что посчитать ($$ — результат).
 */
%{
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int yylex(void);
void yyerror(const char *s);
%}

%define api.value.type {double}   /* у всех значений тип double */

%token NUM

%left '+' '-'
%left '*' '/' '%'
%precedence NEG                 /* унарный минус, старше * и / */
%right '^'                      /* степень: старше унарного минуса */

%%

input:
      %empty
    | input line
    ;

line:
      '\n'                      { /* пустая строка — ничего */ }
    | expr '\n'                 { printf("= %g\n", $1); }
    | error '\n'                { yyerrok; printf("не понял, попробуйте ещё\n"); }
    ;

expr:
      NUM                       { $$ = $1; }
    | expr '+' expr             { $$ = $1 + $3; }
    | expr '-' expr             { $$ = $1 - $3; }
    | expr '*' expr             { $$ = $1 * $3; }
    | expr '/' expr             { if ($3 == 0) { printf("делить на ноль нельзя\n"); $$ = 0; } else $$ = $1 / $3; }
    | expr '%' expr             { if ($3 == 0) { printf("делить на ноль нельзя\n"); $$ = 0; } else $$ = fmod($1, $3); }
    | expr '^' expr             { $$ = pow($1, $3); }
    | '-' expr %prec NEG        { $$ = -$2; }
    | '(' expr ')'              { $$ = $2; }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "ошибка: %s\n", s);
}

int main(void)
{
    printf("калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход\n");
    return yyparse();
}
