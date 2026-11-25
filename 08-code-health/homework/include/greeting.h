/* greeting.h — одна маленькая функция, на которой видно всю проверку кода.
 *
 * make_greeting() кладёт в буфер out строку "Hello, <name>!".
 * cap — размер буфера в байтах.
 *
 * Возвращает:
 *    0  — всё хорошо
 *   -1  — неправильные аргументы (out == NULL или cap == 0)
 *   -2  — строка не влезла в буфер
 */
#ifndef GREETING_H
#define GREETING_H

#include <stddef.h>

int make_greeting(char *out, size_t cap, const char *name);

#endif /* GREETING_H */
