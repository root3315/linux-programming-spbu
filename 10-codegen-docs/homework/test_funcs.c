/**
 * @file test_funcs.c
 * @brief Тесты встроенных функций (funcs.c). Выход 0, если всё прошло.
 */
#include <math.h>
#include <stdio.h>

#include "funcs.h"

static int failed;

/** Проверка: печатает строку и номер, если условие ложно. */
#define CHECK(cond)                                                   \
    do {                                                              \
        if (!(cond)) {                                                \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            failed++;                                                 \
        }                                                             \
    } while (0)

/** Вызывает функцию и сравнивает результат с ожидаемым с запасом 1e-12. */
static int gives(const char *name, double arg, double want)
{
    double out = 0;
    return func_call(name, arg, &out) == FUNC_OK && fabs(out - want) < 1e-12;
}

int main(void)
{
    double out = 123;

    CHECK(gives("sqrt", 9, 3));
    CHECK(gives("sqrt", 0, 0));
    CHECK(gives("abs", -2.5, 2.5));
    CHECK(gives("sin", 0, 0));
    CHECK(gives("cos", 0, 1));
    CHECK(gives("ln", 1, 0));

    CHECK(func_call("sqrt", -1, &out) == FUNC_DOMAIN);
    CHECK(func_call("ln", 0, &out) == FUNC_DOMAIN);
    CHECK(func_call("ln", -3, &out) == FUNC_DOMAIN);
    CHECK(func_call("sqrt", NAN, &out) == FUNC_DOMAIN);
    CHECK(func_call("foo", 1, &out) == FUNC_UNKNOWN);
    CHECK(func_call("", 1, &out) == FUNC_UNKNOWN);
    CHECK(func_call("SQRT", 9, &out) == FUNC_UNKNOWN);
    CHECK(out == 123);   /* при отказе результат не трогали */

    if (failed == 0)
        puts("test_funcs: все проверки прошли");
    return failed ? 1 : 0;
}
