/* memory.c — исправленная версия broken/memory.c из Chapter 04.
 *
 * В исходнике цикл шёл до i <= 4 и писал data[4], а блок из malloc
 * всего на 4 int (индексы 0..3). Проверка:
 *   gcc -fsanitize=address,undefined -g -O1 memory.c -o memory && ./memory
 * ASan молчит, программа печатает last=9.
 */
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int *data = malloc(4 * sizeof *data);
    if (!data)
        return 1;

    for (int i = 0; i < 4; i++)
        data[i] = i * i;

    printf("last=%d\n", data[3]);
    free(data);
    return 0;
}
