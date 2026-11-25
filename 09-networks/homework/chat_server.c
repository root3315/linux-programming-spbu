/* chat_server.c: «чат на двоих», сервер.
 *
 * Запуск: ./chat_server PORT
 * Клиент шлёт строку «имя: сообщение», сервер отвечает
 * «привет, имя! ты сказал: сообщение» и печатает счётчик сообщений.
 * Клиентов обслуживает по одному, соединение за соединением.
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "chat.h"

static unsigned long message_count;

/* Убирает пробелы по краям, возвращает указатель внутрь строки. */
static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t'))
        s[--len] = '\0';
    return s;
}

static int reply(int conn, const char *text)
{
    char out[MAX_LINE + 128];
    int len = snprintf(out, sizeof out, "%s\n", text);
    if (len < 0 || (size_t)len >= sizeof out)
        return -1;
    return send_all(conn, out, (size_t)len);
}

/* Отказ: пишем в консоль сервера и отвечаем клиенту. */
static int refuse(int conn, const char *why)
{
    char text[MAX_LINE + 128];
    printf("отказ: %s\n", why);
    snprintf(text, sizeof text, "ошибка: %s", why);
    return reply(conn, text);
}

/* Одна готовая строка от клиента. too_long: она не поместилась в MAX_LINE. */
static int handle_line(int conn, char *line, int too_long)
{
    if (too_long)
        return refuse(conn, "строка длиннее 512 байт");
    if (line[0] == '\0')
        return refuse(conn, "пустая строка, нужен формат «имя: сообщение»");

    char *colon = strchr(line, ':');
    if (colon == NULL)
        return refuse(conn, "нет двоеточия, нужен формат «имя: сообщение»");

    *colon = '\0';
    char *name = trim(line);
    char *text = trim(colon + 1);

    if (name[0] == '\0')
        return refuse(conn, "пустое имя");
    if (strlen(name) > MAX_NAME)
        return refuse(conn, "имя длиннее 32 байт");
    if (text[0] == '\0')
        return refuse(conn, "пустое сообщение");

    message_count++;
    printf("[%lu] %s: %s\n", message_count, name, text);

    char answer[MAX_LINE + 128];
    snprintf(answer, sizeof answer, "привет, %s! ты сказал: %s", name, text);
    return reply(conn, answer);
}

/* Читает соединение кусками и режет на строки. Очень длинную строку
 * дочитывает до конца и выбрасывает, соединение не рвёт. */
static void serve(int conn)
{
    char line[MAX_LINE + 1];
    size_t len = 0;
    int too_long = 0;
    char chunk[1024];
    ssize_t n;

    while ((n = recv(conn, chunk, sizeof chunk, 0)) != 0) {
        if (n < 0) {
            if (errno == EINTR)
                continue;
            perror("recv");
            return;
        }
        for (ssize_t i = 0; i < n; i++) {
            char c = chunk[i];
            if (c == '\n') {
                line[len] = '\0';
                if (len > 0 && line[len - 1] == '\r')
                    line[len - 1] = '\0';
                if (handle_line(conn, line, too_long) < 0)
                    return;
                len = 0;
                too_long = 0;
            } else if (len < MAX_LINE) {
                line[len++] = c;
            } else {
                too_long = 1;
            }
        }
    }
}

static int listen_on(int port)
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket");
        return -1;
    }

    int yes = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
        perror("setsockopt");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(server, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind");
        close(server);
        return -1;
    }
    if (listen(server, 5) < 0) {
        perror("listen");
        close(server);
        return -1;
    }
    return server;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "использование: %s PORT\n", argv[0]);
        return 2;
    }
    int port = parse_port(argv[1]);
    if (port < 0) {
        fprintf(stderr, "неверный порт: %s (нужно число от 1 до 65535)\n", argv[1]);
        return 2;
    }

    signal(SIGPIPE, SIG_IGN);

    int server = listen_on(port);
    if (server < 0)
        return 1;

    printf("слушаю 127.0.0.1:%d (Ctrl+C — выход)\n", port);
    fflush(stdout);

    for (;;) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof peer;
        int conn = accept(server, (struct sockaddr *)&peer, &peer_len);
        if (conn < 0) {
            perror("accept");
            continue;
        }

        char ip[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof ip);
        printf("клиент %s:%d подключился\n", ip, ntohs(peer.sin_port));
        fflush(stdout);

        serve(conn);
        close(conn);

        printf("клиент ушёл, всего сообщений: %lu\n", message_count);
        fflush(stdout);
    }
}
