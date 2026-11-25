/* stats.c — реализация статистики по имени. */
#include "stats.h"

#include <ctype.h>
#include <stdio.h>

/* Сколько разных байт в строке: для каждого байта ищу, не встречался ли
 * он раньше. */
static size_t count_distinct(const unsigned char *s, size_t len)
{
    size_t distinct = 0;

    for (size_t i = 0; i < len; i++) {
        size_t seen = 0;

        for (size_t j = 0; j < i; j++)
            if (s[j] == s[i])
                seen++;
        if (seen == 0)
            distinct++;
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
