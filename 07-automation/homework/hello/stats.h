/* stats.h — простая статистика по строке имени.
 * Третий модуль проекта: считает, из чего состоит имя.
 */
#ifndef STATS_H
#define STATS_H

#include <stddef.h>

struct name_stats {
    size_t length;   /* всего байт */
    size_t letters;  /* латинские буквы */
    size_t digits;   /* цифры */
    size_t other;    /* всё остальное: пробелы, знаки, кириллица (UTF-8) */
    size_t distinct; /* сколько разных байт */
};

/* Разбирает строку name. NULL считается пустой строкой. */
struct name_stats stats_collect(const char *name);

/* Печатает статистику одной строкой в stdout. */
void stats_print(const struct name_stats *st);

#endif /* STATS_H */
