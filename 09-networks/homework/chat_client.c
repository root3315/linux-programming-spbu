/* chat_client.c: «чат на двоих», клиент.
 *
 * Запуск: ./chat_client HOST PORT ИМЯ [сообщение ...]
 * Сообщения из аргументов уходят подряд по одному соединению.
 * Без сообщений читает строки со стандартного ввода.
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "chat.h"

/* Читает ответ до перевода строки. Возвращает 1 при успехе, 0 если сервер
 * закрыл соединение, -1 при ошибке. */
static int read_reply(int sock, char *buf, size_t size)
{
    size_t len = 0;
    while (len + 1 < size) {
        char c;
        ssize_t n = recv(sock, &c, 1, 0);
        if (n < 0)
            return -1;
        if (n == 0)
            return 0;
        if (c == '\n')
            break;
        buf[len++] = c;
    }
    buf[len] = '\0';
    return 1;
}

static int say(int sock, const char *name, const char *text)
{
    char out[4096];
    int len = snprintf(out, sizeof out, "%s: %s\n", name, text);
    /* перевод строки внутри сообщения разрезал бы его на две строки протокола */
    for (int i = 0; i < len - 1 && (size_t)i < sizeof out; i++)
        if (out[i] == '\n' || out[i] == '\r')
            out[i] = ' ';
    if (len < 0 || (size_t)len >= sizeof out) {
        fprintf(stderr, "сообщение слишком длинное для клиента\n");
        return 0;
    }
    if (send_all(sock, out, (size_t)len) < 0) {
        perror("send");
        return -1;
    }

    char reply[MAX_LINE + 256];
    int got = read_reply(sock, reply, sizeof reply);
    if (got < 0) {
        perror("recv");
        return -1;
    }
    if (got == 0) {
        fprintf(stderr, "сервер закрыл соединение\n");
        return -1;
    }
    printf("%s\n", reply);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "использование: %s HOST PORT ИМЯ [сообщение ...]\n", argv[0]);
        return 2;
    }
    int port = parse_port(argv[2]);
    if (port < 0) {
        fprintf(stderr, "неверный порт: %s (нужно число от 1 до 65535)\n", argv[2]);
        return 2;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1) {
        fprintf(stderr, "плохой адрес: %s\n", argv[1]);
        return 2;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    if (connect(sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    int status = 0;
    if (argc > 4) {
        for (int i = 4; i < argc && status == 0; i++)
            status = say(sock, argv[3], argv[i]);
    } else {
        char *line = NULL;
        size_t cap = 0;
        ssize_t got;
        while (status == 0 && (got = getline(&line, &cap, stdin)) > 0) {
            if (line[got - 1] == '\n')
                line[got - 1] = '\0';
            status = say(sock, argv[3], line);
        }
        free(line);
    }

    close(sock);
    return status == 0 ? 0 : 1;
}
