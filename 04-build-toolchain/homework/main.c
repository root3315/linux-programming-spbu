/* main.c — точка входа учебного проекта.
 *
 * Проект разбит на три файла .c: main, greet и stats.
 */
#include "greet.h"
#include "stats.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    const char *name = (argc > 1) ? argv[1] : "Linux";
    struct name_stats st = stats_collect(name);

    printf("%s\n", greet(name));
    stats_print(&st);
    printf("greet() вызван %d раз\n", greet_calls());

    return 0;
}
