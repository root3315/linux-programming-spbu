/* test_hello.c — тест модулей greet и stats без фреймворка.
 *
 * Сборка и запуск: make test
 * Код возврата 1, если хоть одна проверка не прошла: на этом
 * останавливаются make, ci.sh и GitHub Actions.
 */
#include "greet.h"
#include "stats.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check(int ok, const char *what)
{
    printf("%s %s\n", ok ? "ok:  " : "FAIL:", what);
    if (!ok)
        failures++;
}

int main(void)
{
    check(strcmp(greet("Linux"), "Hello, Linux!") == 0, "greet: обычное имя");
    check(strcmp(greet(""), "Hello, world!") == 0, "greet: пустая строка");
    check(greet_calls() == 2, "greet: счётчик вызовов");

    struct name_stats st = stats_collect("Linux 2026");
    check(st.length == 10 && st.letters == 5 && st.digits == 4 && st.other == 1,
          "stats: буквы, цифры, прочие");
    check(st.distinct == 9, "stats: разные байты в \"Linux 2026\"");

    /* «Привет» — 6 букв, 12 байт UTF-8, из них 8 разных. */
    st = stats_collect("Привет");
    check(st.length == 12 && st.other == 12 && st.distinct == 8, "stats: кириллица");

    st = stats_collect(NULL);
    check(st.length == 0 && st.distinct == 0, "stats: NULL");

    if (failures) {
        printf("\n%d проверок не прошло\n", failures);
        return 1;
    }
    printf("\nвсе проверки прошли\n");
    return 0;
}
