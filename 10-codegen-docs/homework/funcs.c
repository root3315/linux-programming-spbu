/**
 * @file funcs.c
 * @brief Таблица встроенных функций.
 */
#include "funcs.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

/** Описание одной функции. */
typedef struct {
    const char *name;            /**< имя в выражении */
    double (*apply)(double);     /**< что посчитать */
    int (*in_domain)(double);    /**< годится ли аргумент; NULL, если годится любой */
} func_def;

/**
 * @brief Область определения sqrt.
 * @param x аргумент
 * @return не 0, если x >= 0
 */
static int non_negative(double x)
{
    return x >= 0;
}

/**
 * @brief Область определения ln.
 * @param x аргумент
 * @return не 0, если x > 0
 */
static int positive(double x)
{
    return x > 0;
}

/** Все встроенные функции. */
static const func_def table[] = {
    { "sqrt", sqrt, non_negative },
    { "abs",  fabs, NULL },
    { "sin",  sin,  NULL },
    { "cos",  cos,  NULL },
    { "ln",   log,  positive },
};

func_status func_call(const char *name, double arg, double *out)
{
    for (size_t i = 0; i < sizeof table / sizeof table[0]; i++) {
        if (strcmp(table[i].name, name) != 0)
            continue;
        if (table[i].in_domain != NULL && !table[i].in_domain(arg))
            return FUNC_DOMAIN;
        *out = table[i].apply(arg);
        return FUNC_OK;
    }
    return FUNC_UNKNOWN;
}
