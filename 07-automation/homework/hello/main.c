/* main.c — точка входа hello.
 *
 *   hello [имя]        приветствие и статистика по имени
 *   hello --version    версия из Makefile
 *   hello --bench N    статистика по строке из N байт, для замеров
 */
#include "greet.h"
#include "stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Версию передаёт Makefile: -DVERSION=\"1.1.0\". Без него — "unknown". */
#ifndef VERSION
#define VERSION "unknown"
#endif

static void usage(FILE *out)
{
    fputs("Использование: hello [--version] [--bench N] [ИМЯ]\n", out);
}

/* Строка из n байт: образец повторяется, пока не наберётся длина. */
static char *bench_text(size_t n)
{
    static const char sample[] = "Stepan 2026, Привет! ";
    char *text = malloc(n + 1);

    if (text == NULL)
        return NULL;
    for (size_t i = 0; i < n; i++)
        text[i] = sample[i % (sizeof sample - 1)];
    text[n] = '\0';
    return text;
}

static int run_bench(const char *arg)
{
    char *end;
    long n = strtol(arg, &end, 10);

    if (*arg == '\0' || *end != '\0' || n <= 0) {
        usage(stderr);
        return 2;
    }

    char *text = bench_text((size_t)n);
    if (text == NULL) {
        perror("malloc");
        return 1;
    }
    struct name_stats st = stats_collect(text);
    stats_print(&st);
    free(text);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        usage(stdout);
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("hello %s\n", VERSION);
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--bench") == 0) {
        if (argc != 3) {
            usage(stderr);
            return 2;
        }
        return run_bench(argv[2]);
    }

    const char *name = (argc > 1) ? argv[1] : "Linux";
    struct name_stats st = stats_collect(name);

    printf("%s\n", greet(name));
    stats_print(&st);
    printf("greet() вызван %d раз\n", greet_calls());
    return 0;
}
