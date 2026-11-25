/* greeting.c — исправленная версия.
 *
 * strcpy/strcat заменены на snprintf: он знает размер буфера и не пишет
 * за край. Если строка не влезла, функция честно возвращает -2.
 */
#include "greeting.h"

#include <stdio.h>

int make_greeting(char *out, size_t cap, const char *name)
{
    if (out == NULL || cap == 0)
        return -1;

    if (name == NULL || name[0] == '\0')
        name = "world";

    int n;
    if (name[0] == '!')
        n = snprintf(out, cap, "HELLO, %s!!!", name + 1);
    else
        n = snprintf(out, cap, "Hello, %s!", name);

    if (n < 0)
        return -1;
    if ((size_t)n >= cap)
        return -2;   /* не влезло: в out обрезанная строка, но без выхода за край */
    return 0;
}
