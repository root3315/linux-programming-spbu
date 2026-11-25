/* chat.h: общие вещи сервера и клиента «чата на двоих». */
#ifndef CHAT_H
#define CHAT_H

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAX_LINE 512 /* самая длинная принимаемая строка, без перевода строки */
#define MAX_NAME 32  /* самое длинное имя в байтах */

/* Порт из строки: 1..65535. При ошибке возвращает -1. */
static inline int parse_port(const char *text)
{
    char *end = NULL;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value < 1 || value > 65535)
        return -1;
    return (int)value;
}

/* Пишет весь буфер: send может записать меньше, чем просили. */
static inline int send_all(int fd, const char *data, size_t len)
{
    while (len > 0) {
        ssize_t n = send(fd, data, len, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        data += n;
        len -= (size_t)n;
    }
    return 0;
}

#endif
