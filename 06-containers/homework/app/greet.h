/* greet.h — интерфейс модуля приветствия.
 * Единственное место, где объявлено, что умеет greet.c.
 * Подключается и в main.c, и в greet.c: так компилятор видит
 * одинаковую сигнатуру с обеих сторон.
 */
#ifndef GREET_H
#define GREET_H

/* Возвращает "<GREETING>, <name>!"; GREETING берётся из окружения.
 * Строка живёт в статическом буфере — освобождать не нужно. */
const char *greet(const char *name);

/* Сколько раз вызывали greet(). */
int greet_calls(void);

#endif /* GREET_H */
