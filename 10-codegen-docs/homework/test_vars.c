/**
 * @file test_vars.c
 * @brief Тесты таблицы переменных (vars.c). Выход 0, если всё прошло.
 */
#include <stdio.h>
#include <string.h>

#include "vars.h"

static int failed;

/** Проверка: печатает строку и номер, если условие ложно. */
#define CHECK(cond)                                                   \
    do {                                                              \
        if (!(cond)) {                                                \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            failed++;                                                 \
        }                                                             \
    } while (0)

/** Новая переменная читается обратно, чужая не находится. */
static void test_set_get(void)
{
    vars *t = vars_new();
    double v = -1;
    CHECK(t != NULL);
    CHECK(vars_count(t) == 0);
    CHECK(!vars_get(t, "x", &v));
    CHECK(vars_set(t, "x", 5) == VARS_OK);
    CHECK(vars_get(t, "x", &v) && v == 5);
    CHECK(vars_get(t, "x", NULL));
    CHECK(!vars_get(t, "y", &v));
    CHECK(vars_count(t) == 1);
    vars_free(t);
}

/** Повторная запись меняет значение и не плодит записи. */
static void test_overwrite(void)
{
    vars *t = vars_new();
    double v = 0;
    CHECK(vars_set(t, "x", 1) == VARS_OK);
    CHECK(vars_set(t, "x", 2) == VARS_OK);
    CHECK(vars_get(t, "x", &v) && v == 2);
    CHECK(vars_count(t) == 1);
    vars_free(t);
}

/** Имена: пустое, с цифры, с точкой, на границе длины и длиннее. */
static void test_names(void)
{
    char name[VARS_NAME_MAX + 2];
    vars *t = vars_new();

    CHECK(vars_set(t, "", 1) == VARS_BAD_NAME);
    CHECK(vars_set(t, "1x", 1) == VARS_BAD_NAME);
    CHECK(vars_set(t, "a.b", 1) == VARS_BAD_NAME);
    CHECK(vars_set(t, "_a1", 1) == VARS_OK);

    memset(name, 'a', VARS_NAME_MAX);
    name[VARS_NAME_MAX] = '\0';
    CHECK(vars_set(t, name, 1) == VARS_OK);
    name[VARS_NAME_MAX] = 'a';
    name[VARS_NAME_MAX + 1] = '\0';
    CHECK(vars_set(t, name, 1) == VARS_BAD_NAME);
    CHECK(vars_count(t) == 2);
    vars_free(t);
}

/** Таблица растёт до ::VARS_LIMIT, дальше отказ, старые значения целы. */
static void test_limit(void)
{
    vars *t = vars_new();
    char name[16];
    double v = 0;

    for (int i = 0; i < VARS_LIMIT; i++) {
        snprintf(name, sizeof name, "v%d", i);
        CHECK(vars_set(t, name, i) == VARS_OK);
    }
    CHECK(vars_count(t) == VARS_LIMIT);
    CHECK(vars_set(t, "extra", 1) == VARS_FULL);
    CHECK(vars_set(t, "v7", 70) == VARS_OK);
    CHECK(vars_get(t, "v7", &v) && v == 70);
    CHECK(vars_get(t, "v1023", &v) && v == 1023);
    vars_free(t);
}

/** Освобождение NULL допустимо. */
static void test_free_null(void)
{
    vars_free(NULL);
}

int main(void)
{
    test_set_get();
    test_overwrite();
    test_names();
    test_limit();
    test_free_null();
    if (failed == 0)
        puts("test_vars: все проверки прошли");
    return failed ? 1 : 0;
}
