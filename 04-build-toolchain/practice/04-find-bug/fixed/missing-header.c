/* missing-header.c — исправленная версия broken/missing-header.c из Chapter 04.
 *
 * Не хватало #include <string.h>, поэтому компилятор не знал прототип
 * strlen. Теперь собирается без предупреждений даже с -Werror:
 *   gcc -Wall -Wextra -Werror missing-header.c -o missing
 */
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *text = "Linux";
    printf("len=%zu\n", strlen(text));
    return 0;
}
