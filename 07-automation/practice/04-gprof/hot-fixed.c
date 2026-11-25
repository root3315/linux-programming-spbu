/* hot-fixed.c — hot.c из Chapter 07 после профиля.
 * Изменён только main: sum_squares(200) не зависит от k, поэтому считаю
 * её один раз и умножаю на число повторов. Результат тот же.
 *
 *   gcc -O2 -o hot-fixed hot-fixed.c && ./hot-fixed 34
 */
#include <stdio.h>
#include <stdlib.h>

/* Наивная рекурсия: экспоненциальная сложность — главный «горячий» участок. */
static unsigned long fib(int n)
{
    if (n < 2)
        return (unsigned long)n;
    return fib(n - 1) + fib(n - 2);
}

/* Линейный проход: вызывается часто, но стоит дёшево. */
static unsigned long sum_squares(int n)
{
    unsigned long sum = 0;
    for (int i = 1; i <= n; i++)
        sum += (unsigned long)i * (unsigned long)i;
    return sum;
}

int main(int argc, char **argv)
{
    int n = (argc > 1) ? atoi(argv[1]) : 34;
    unsigned long f = fib(n);
    unsigned long big = 200000UL * sum_squares(200);

    printf("fib(%d) = %lu\n", n, f);
    printf("sum     = %lu\n", big);
    return 0;
}
