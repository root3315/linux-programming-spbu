/**
 * @file calc.c
 * @brief Ошибки, вывод, доступ к таблицам и точка входа калькулятора.
 */
#include "calc.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "funcs.h"
#include "vars.h"

/** в текущей строке уже была ошибка */
static bool line_failed;
/** переменные; создаёт main */
static vars *table;

void calc_fail(const char *fmt, ...)
{
    if (line_failed)
        return;
    line_failed = true;

    fflush(stdout);   /* чтобы результаты и ошибки шли в том порядке, как печатались */
    fputs("ошибка: ", stderr);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

bool calc_failed(void)
{
    return line_failed;
}

void calc_end_line(void)
{
    line_failed = false;
}

void calc_print(double value)
{
    printf("= %.10g\n", value);
}

void calc_assign(const char *name, double value)
{
    if (line_failed)
        return;
    switch (vars_set(table, name, value)) {
    case VARS_OK:
        printf("%s = %.10g\n", name, value);
        break;
    case VARS_FULL:
        calc_fail("таблица переменных заполнена (%d)", VARS_LIMIT);
        break;
    case VARS_NOMEM:
        calc_fail("не хватило памяти");
        break;
    case VARS_BAD_NAME:
        calc_fail("плохое имя переменной");
        break;
    }
}

double calc_lookup(const char *name)
{
    double value = 0;
    if (!vars_get(table, name, &value))
        calc_fail("переменная не определена: %s", name);
    return value;
}

double calc_call(const char *name, double arg)
{
    double result = 0;
    switch (func_call(name, arg, &result)) {
    case FUNC_OK:
        break;
    case FUNC_UNKNOWN:
        calc_fail("неизвестная функция: %s", name);
        result = 0;
        break;
    case FUNC_DOMAIN:
        calc_fail("%s: аргумент %.10g вне области определения", name, arg);
        result = 0;
        break;
    }
    return result;
}

void yyerror(const char *msg)
{
    if (strcmp(msg, "syntax error") == 0)
        calc_fail("синтаксическая ошибка");
    else
        calc_fail("%s", msg);
}

/**
 * @brief Точка входа: создаёт таблицу переменных, разбирает ввод, всё освобождает.
 * @return 0, если ввод разобран до конца; не 0, если разборщик остановился
 *         (например, слишком глубокие скобки)
 */
int main(void)
{
    table = vars_new();
    if (table == NULL) {
        fputs("ошибка: не хватило памяти\n", stderr);
        return 1;
    }
    if (isatty(STDIN_FILENO))
        puts("калькулятор: пишите выражения, Enter — посчитать, Ctrl+D — выход");

    int status = yyparse();

    yylex_destroy();
    vars_free(table);
    return status;
}
