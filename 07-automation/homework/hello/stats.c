/* stats.c — реализация статистики по имени. */
#include "stats.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>

/* Сколько разных байт в строке. Значений байта всего 256, поэтому
 * хватает таблицы «уже видел»: один проход вместо сравнения каждого
 * байта со всеми предыдущими (было O(n^2), см. REPORT.md, раздел 4). */
static size_t count_distinct(const unsigned char *s, size_t len)
{
    unsigned char seen[UCHAR_MAX + 1] = {0};
    size_t distinct = 0;

    for (size_t i = 0; i < len; i++) {
        if (!seen[s[i]]) {
            seen[s[i]] = 1;
            distinct++;
        }
    }
    return distinct;
}

struct name_stats stats_collect(const char *name)
{
    struct name_stats st = {0, 0, 0, 0, 0};

    if (name == NULL)
        return st;

    for (const char *p = name; *p != '\0'; p++) {
        /* isalpha/isdigit принимают только unsigned char или EOF.
         * Байты кириллицы в UTF-8 отрицательные как char,
         * без приведения это неопределённое поведение. */
        unsigned char c = (unsigned char)*p;

        st.length++;
        if (isdigit(c))
            st.digits++;
        else if (c < 128 && isalpha(c))
            st.letters++;
        else
            st.other++;
    }
    st.distinct = count_distinct((const unsigned char *)name, st.length);
    return st;
}

void stats_print(const struct name_stats *st)
{
    printf("длина %zu: букв %zu, цифр %zu, прочих %zu, разных байт %zu\n",
           st->length, st->letters, st->digits, st->other, st->distinct);
}
