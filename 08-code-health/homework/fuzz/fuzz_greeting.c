/* fuzz_greeting.c — простой фаззер: «обезьяна», которая придумывает
 * тысячи случайных имён и скармливает их make_greeting() и shorten_name().
 *
 * Сборка и запуск:
 *   gcc -fsanitize=address,undefined -g -O1 -Iinclude \
 *       fuzz/fuzz_greeting.c src/greeting.c src/shorten.c -o fuzz_greeting
 *   ./fuzz_greeting 2000 1
 *
 * Первый аргумент — сколько попыток сделать (по умолчанию 1000),
 * второй — зерно: с тем же зерном получается тот же набор входов.
 */
#include "greeting.h"
#include "shorten.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Кусочки, из которых собирается имя: ASCII (1 байт), кириллица
 * (2 байта), «€» (3 байта), эмодзи (4 байта) и «!», чтобы заходить
 * в ветку «громкого» приветствия. Последние два кусочка — битый UTF-8:
 * одинокий байт продолжения и восемь таких байт подряд. */
static const char *const pieces[] = {
    "a", "b", "Z", "0", " ", "'", "!", "Я", "ж", "ё", "€", "😀",
    "\x80", "\x80\x80\x80\x80\x80\x80\x80\x80",
};
#define PIECES (sizeof pieces / sizeof pieces[0])
#define MAX_PIECES 60
#define MAX_PIECE_BYTES 8

/* Собирает в name (cap байт) случайное имя из 0…MAX_PIECES кусочков. */
static void random_name(char *name, size_t cap)
{
    int count = rand() % (MAX_PIECES + 1);
    size_t len = 0;

    for (int k = 0; k < count; k++) {
        const char *piece = pieces[rand() % PIECES];
        size_t n = strlen(piece);
        if (len + n >= cap)
            break;
        memcpy(name + len, piece, n);
        len += n;
    }
    name[len] = '\0';
}

static int is_continuation(unsigned char byte)
{
    return (byte & 0xC0) == 0x80;
}

/* Правильный ли это UTF-8: за первым байтом идёт столько продолжений,
 * сколько он обещает, и нет одиноких байтов продолжения. */
static int utf8_valid(const char *s)
{
    while (*s != '\0') {
        unsigned char lead = (unsigned char)*s++;
        int more = lead < 0x80 ? 0 : lead >= 0xF0 ? 3 : lead >= 0xE0 ? 2 : lead >= 0xC0 ? 1 : -1;
        if (more < 0)
            return 0;
        for (; more > 0; more--)
            if (!is_continuation((unsigned char)*s++))
                return 0;
    }
    return 1;
}

/* Число букв в правильном UTF-8: байты, которые не продолжения. */
static size_t count_chars(const char *s)
{
    size_t chars = 0;
    for (; *s != '\0'; s++)
        if (!is_continuation((unsigned char)*s))
            chars++;
    return chars;
}

static int fail(int attempt, const char *what, const char *name)
{
    printf("ОШИБКА на попытке %d: %s\nимя: \"%s\"\n", attempt, what, name);
    return 1;
}

/* Одна попытка для make_greeting: буфер нарочно маленький, 32 байта. */
static int fuzz_greeting(int attempt, const char *name)
{
    char out[32];

    int rc = make_greeting(out, sizeof out, name);
    if (rc == -2)
        return 0;          /* «не влезло» — это допустимый ответ */
    if (rc != 0)
        return fail(attempt, "make_greeting: неожиданный код", name);
    if (strlen(out) >= sizeof out)
        return fail(attempt, "make_greeting: строка длиннее буфера", name);
    return 0;
}

/* Проверяет ответ shorten_name. Для любого имени: код из списка, -2 только
 * при буфере меньше SHORT_NAME_CAP, результат — начало имени (с «…», если
 * сокращено). Для правильного UTF-8 ещё число букв и что буква не разрезана. */
static int shorten_ok(const char *name, int rc, const char *out, size_t cap)
{
    static const char ellipsis[] = "…";
    const size_t tail = sizeof ellipsis - 1;
    size_t len = strlen(out);
    int valid = utf8_valid(name);

    if (rc == -2)
        return out[0] == '\0' && cap < SHORT_NAME_CAP;
    if (rc == 0)
        return strcmp(out, name) == 0 && (!valid || count_chars(name) <= SHORT_NAME_MAX);
    if (rc != 1 || len < tail || strcmp(out + len - tail, ellipsis) != 0)
        return 0;

    size_t keep = len - tail;
    if (strncmp(out, name, keep) != 0 || name[keep] == '\0')
        return 0;
    return !valid || (count_chars(out) == SHORT_NAME_MAX
                      && !is_continuation((unsigned char)name[keep]));
}

/* Одна попытка для shorten_name. Буфер случайного размера 1…48 байт
 * в куче: так ASan замечает запись даже на один байт за край. */
static int fuzz_shorten(int attempt, const char *name)
{
    size_t cap = 1 + (size_t)(rand() % 48);
    char *out = malloc(cap);
    if (out == NULL)
        return fail(attempt, "нет памяти", name);

    int rc = shorten_name(out, cap, name);
    int ok = shorten_ok(name, rc, out, cap);
    if (!ok)
        printf("shorten_name: код %d, cap %zu, имя %zu байт, out \"%s\"\n",
               rc, cap, strlen(name), out);
    free(out);
    return ok ? 0 : fail(attempt, "shorten_name: неверный результат", name);
}

int main(int argc, char **argv)
{
    int iterations = (argc > 1) ? atoi(argv[1]) : 1000;
    unsigned seed = (argc > 2) ? (unsigned)strtoul(argv[2], NULL, 10) : 42u;
    srand(seed);

    /* Без буфера: если ASan уронит программу, строки выше всё равно видны. */
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("фаззинг: %d попыток, зерно %u\n", iterations, seed);

    for (int i = 1; i <= iterations; i++) {
        char name[MAX_PIECES * MAX_PIECE_BYTES + 1];
        random_name(name, sizeof name);

        if (fuzz_greeting(i, name) || fuzz_shorten(i, name))
            return 1;
    }

    printf("проблем не найдено — но это не доказательство, что их нет\n");
    return 0;
}
