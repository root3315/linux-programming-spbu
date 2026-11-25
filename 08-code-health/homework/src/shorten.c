/* shorten.c — сокращение имени до SHORT_NAME_MAX букв (описание в shorten.h). */
#include "shorten.h"

#include <string.h>

static const char ELLIPSIS[] = "\xE2\x80\xA6";   /* «…», U+2026 в UTF-8 */

/* Байты-продолжения UTF-8 имеют вид 10xxxxxx, с них буква не начинается. */
static int is_continuation(unsigned char byte)
{
    return (byte & 0xC0) == 0x80;
}

/* Длина в байтах буквы, которая начинается с s[0]. Её подсказывают старшие
 * биты первого байта, но буква кончается раньше, если продолжения нет.
 * Так и битый UTF-8 делится на буквы не длиннее 4 байт, а одинокий байт
 * продолжения считается отдельной буквой. */
static size_t char_len(const char *s)
{
    unsigned char lead = (unsigned char)s[0];
    size_t len = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;

    for (size_t i = 1; i < len; i++)
        if (!is_continuation((unsigned char)s[i]))
            return i;
    return len;
}

/* Сколько байт занимают первые max_chars букв строки s. */
static size_t prefix_bytes(const char *s, size_t max_chars)
{
    size_t bytes = 0;

    for (size_t chars = 0; chars < max_chars && s[bytes] != '\0'; chars++)
        bytes += char_len(s + bytes);
    return bytes;
}

int shorten_name(char *out, size_t cap, const char *name)
{
    if (out == NULL || cap == 0)
        return -1;
    if (name == NULL)
        name = "";

    size_t keep = prefix_bytes(name, SHORT_NAME_MAX);
    int shortened = name[keep] != '\0';
    if (shortened)
        keep = prefix_bytes(name, SHORT_NAME_MAX - 1);

    size_t tail = shortened ? sizeof ELLIPSIS - 1 : 0;
    size_t need = keep + tail;            /* байт без завершающего '\0' */
    if (need >= cap) {
        out[0] = '\0';
        return -2;
    }

    memcpy(out, name, keep);
    memcpy(out + keep, ELLIPSIS, tail);
    out[need] = '\0';
    return shortened;
}
