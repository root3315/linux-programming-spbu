/* test_shorten.c — тесты shorten_name().
 *
 * Обычное имя, пустое, слишком длинное, кириллица (буквы по 2 байта),
 * эмодзи (по 4 байта) и маленький буфер.
 */
#include "shorten.h"

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

/* Вызывает shorten_name с буфером SHORT_NAME_CAP и сверяет код и строку. */
static void expect(const char *name, int rc, const char *result, const char *what)
{
    char out[SHORT_NAME_CAP];

    memset(out, 'x', sizeof out);
    check(shorten_name(out, sizeof out, name) == rc && strcmp(out, result) == 0, what);
}

int main(void)
{
    char small[8];

    expect("Linux", 0, "Linux", "обычное имя не меняется");
    expect("", 0, "", "пустое имя остаётся пустым");
    expect(NULL, 0, "", "NULL как пустое имя");
    expect("Abcdefghij", 0, "Abcdefghij", "ровно 10 букв не режется");
    expect("Konstantinopol", 1, "Konstanti…", "длинное имя: 9 букв и многоточие");

    expect("Константин", 0, "Константин", "кириллица, ровно 10 букв (20 байт)");
    expect("Александрина", 1, "Александр…", "кириллица режется по буквам, а не по байтам");
    expect("😀😀😀😀😀😀😀😀😀😀😀", 1, "😀😀😀😀😀😀😀😀😀…",
           "11 эмодзи: результат ровно на SHORT_NAME_CAP байт");
    /* Нашёл фаззер: хвост из байтов продолжения раньше склеивался с «a»
     * в одну «букву», и результат не влезал в SHORT_NAME_CAP. */
    expect("a\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80", 1,
           "a\x80\x80\x80\x80\x80\x80\x80\x80…",
           "битый UTF-8: одинокий байт продолжения — отдельная буква");
    /* Белое пятно из make coverage: первый байт «Я» (D0) без второго. */
    expect("abc\xD0" "defghijk", 1, "abc\xD0" "defgh…",
           "битый UTF-8: оборванная буква — отдельная буква");

    check(shorten_name(NULL, 10, "x") == -1, "out == NULL отклоняется");
    check(shorten_name(small, 0, "x") == -1, "cap == 0 отклоняется");

    check(shorten_name(small, 6, "Linux") == 0 && strcmp(small, "Linux") == 0,
          "буфер впритык: 5 букв + '\\0' в 6 байт");
    check(shorten_name(small, 5, "Linux") == -2 && small[0] == '\0',
          "буфер на байт меньше: -2 и пустая строка");

    if (failed) {
        printf("\n%d проверок не прошло\n", failed);
        return 1;
    }
    printf("\nвсе проверки shorten_name прошли\n");
    return 0;
}
