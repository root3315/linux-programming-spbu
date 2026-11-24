/* duplicate-b.c — исправленная версия broken/duplicate-b.c из Chapter 04.
 *
 * helper() была определена и здесь, и в duplicate-a.c, обе без static,
 * и линковщик видел два одинаковых глобальных имени. Теперь helper здесь
 * static: имя видно только внутри этого файла и с duplicate-a.c не спорит.
 */
#include <stdio.h>

static int helper(void)
{
    return 2;
}

void print_b(void)
{
    printf("B: helper=%d\n", helper());
}
