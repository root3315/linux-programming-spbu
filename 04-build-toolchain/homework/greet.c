/* greet.c — реализация модуля приветствия.
 * Компилируется отдельно: gcc -c greet.c -o greet.o
 */
#include "greet.h"

#include <stdio.h>
#include <string.h>

static int calls = 0;

const char *greet(const char *name)
{
    /* Буфер статический: результат живёт до следующего вызова.
     * Для учебного примера это осознанное упрощение. */
    static char buffer[64];

    if (name == NULL || name[0] == '\0')
        name = "world";

    calls++;
    snprintf(buffer, sizeof buffer, "Hello, %s!", name);
    return buffer;
}

int greet_calls(void)
{
    return calls;
}
