/* greet.c — реализация модуля приветствия.
 * Компилируется отдельно: gcc -c greet.c -o greet.o
 * Слово приветствия берётся из переменной окружения GREETING
 * (по умолчанию "Hello"), чтобы один образ вёл себя по-разному в compose.
 */
#include "greet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int calls = 0;

const char *greet(const char *name)
{
    /* Буфер статический: результат живёт до следующего вызова.
     * Для учебного примера это осознанное упрощение. */
    static char buffer[128];
    const char *word = getenv("GREETING");

    if (word == NULL || word[0] == '\0')
        word = "Hello";
    if (name == NULL || name[0] == '\0')
        name = "world";

    calls++;
    snprintf(buffer, sizeof buffer, "%s, %s!", word, name);
    return buffer;
}

int greet_calls(void)
{
    return calls;
}
