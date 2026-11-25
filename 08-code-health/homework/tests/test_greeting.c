/* test_greeting.c — тесты «по контрольным вопросам».
 *
 * Сначала тут были только обычное и пустое имя. Кириллицу, «!Аня» и
 * длинное имя (на нём и спрятана ошибка) дописал я в практиках 1–2.
 */
#include "greeting.h"

#include <stdio.h>
#include <string.h>

static int failed = 0;

static void check(int ok, const char *what)
{
    if (ok) {
        printf("ok:   %s\n", what);
    } else {
        printf("FAIL: %s\n", what);
        failed++;
    }
}

int main(void)
{
    char buffer[64];

    memset(buffer, 0, sizeof buffer);
    check(make_greeting(buffer, sizeof buffer, "Linux") == 0, "код возврата 0");
    check(strcmp(buffer, "Hello, Linux!") == 0, "обычное имя");

    memset(buffer, 0, sizeof buffer);
    check(make_greeting(buffer, sizeof buffer, NULL) == 0, "NULL допустим");
    check(strcmp(buffer, "Hello, world!") == 0, "NULL заменяется на world");

    memset(buffer, 0, sizeof buffer);
    check(make_greeting(buffer, sizeof buffer, "") == 0, "пустое имя допустимо");
    check(strcmp(buffer, "Hello, world!") == 0, "пустое имя заменяется");

    check(make_greeting(NULL, 10, "x") == -1, "out == NULL отклоняется");
    check(make_greeting(buffer, 0, "x") == -1, "cap == 0 отклоняется");

    memset(buffer, 0, sizeof buffer);
    check(make_greeting(buffer, sizeof buffer, "СПбГУ") == 0
          && strcmp(buffer, "Hello, СПбГУ!") == 0, "кириллица");

    memset(buffer, 0, sizeof buffer);
    check(make_greeting(buffer, sizeof buffer, "!Аня") == 0
          && strcmp(buffer, "HELLO, Аня!!!") == 0, "громкое приветствие");

    /* 64 буквы: вместе с "Hello, " и "!" в буфер на 64 байта не влезет. */
    char long_name[65];
    memset(long_name, 'A', 64);
    long_name[64] = '\0';
    check(make_greeting(buffer, sizeof buffer, long_name) == -2,
          "длинное имя возвращает -2");

    if (failed) {
        printf("\n%d проверок не прошло\n", failed);
        return 1;
    }
    printf("\nвсе проверки прошли (но это не значит, что код без ошибок)\n");
    return 0;
}
