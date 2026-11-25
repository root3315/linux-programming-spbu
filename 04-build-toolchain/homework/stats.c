/* stats.c — реализация статистики по имени. */
#include "stats.h"

#include <ctype.h>
#include <stdio.h>

struct name_stats stats_collect(const char *name)
{
    struct name_stats st = {0, 0, 0, 0};

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
    return st;
}

void stats_print(const struct name_stats *st)
{
    printf("длина %zu: букв %zu, цифр %zu, прочих %zu\n",
           st->length, st->letters, st->digits, st->other);
}
