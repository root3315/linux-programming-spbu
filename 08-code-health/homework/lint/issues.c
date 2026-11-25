/* issues.c — специально «грязный» файл для линтеров.
 *
 *   cppcheck --enable=warning,style --quiet lint/issues.c
 *   clang-tidy -p build lint/issues.c      # нужен compile_commands.json
 *
 * Здесь три типичные проблемы: неинициализированная переменная,
 * опасная функция и деление на ноль без проверки.
 */
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int sum;                  /* не инициализирована */
    char line[16];

    gets(line);               /* опасная функция: нет контроля длины */
    sum += (int)line[0];

    int parts = 0;
    printf("среднее: %d\n", sum / parts);   /* деление на ноль */

    return 0;
}
