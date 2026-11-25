/* main.c — точка входа учебного проекта.
 *
 * Проект из ДЗ 4 (main, greet, stats), для контейнера добавлен режим сервиса:
 *   hello [имя]            напечатать приветствие и статистику, выйти
 *   hello --serve [имя]    то же, потом ждать SIGTERM/SIGINT и выйти с кодом 0
 * В режиме сервиса, если задана LOG_FILE, в этот файл дописываются строки
 * о старте и остановке (в compose он лежит в томе).
 */
#define _POSIX_C_SOURCE 200809L

#include "greet.h"
#include "stats.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static volatile sig_atomic_t got_signal = 0;

static void on_signal(int sig)
{
    got_signal = sig;
}

static void log_line(const char *who, const char *event)
{
    const char *path = getenv("LOG_FILE");
    if (path == NULL || path[0] == '\0')
        return;

    FILE *f = fopen(path, "a");
    if (f == NULL) {
        perror(path);
        return;
    }
    char stamp[32];
    time_t now = time(NULL);
    strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", gmtime(&now));
    fprintf(f, "%s UTC %s %s\n", stamp, who, event);
    fclose(f);
}

/* Ждём сигнал без активного ожидания. SIGTERM и SIGINT заблокированы
 * до sigsuspend: сигнал, пришедший между проверкой флага и засыпанием,
 * не потеряется — sigsuspend атомарно снимает блокировку и спит. */
static int serve(const char *name)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    sigset_t block, old;
    sigemptyset(&block);
    sigaddset(&block, SIGTERM);
    sigaddset(&block, SIGINT);
    sigprocmask(SIG_BLOCK, &block, &old);

    time_t start = time(NULL);
    log_line(name, "start");
    printf("жду SIGTERM\n");

    while (!got_signal)
        sigsuspend(&old);

    printf("получен %s, работал %ld с, выхожу\n",
           got_signal == SIGTERM ? "SIGTERM" : "SIGINT",
           (long)(time(NULL) - start));
    log_line(name, "stop");
    return 0;
}

int main(int argc, char **argv)
{
    /* В контейнере stdout — не терминал, а pipe к docker logs, и вывод
     * буферизуется блоками: строки появились бы только при выходе. */
    setvbuf(stdout, NULL, _IOLBF, 0);

    int serve_mode = (argc > 1 && strcmp(argv[1], "--serve") == 0);
    if (serve_mode) {
        argv++;
        argc--;
    }

    const char *name = (argc > 1) ? argv[1] : "Linux";
    struct name_stats st = stats_collect(name);

    printf("%s\n", greet(name));
    stats_print(&st);
    printf("greet() вызван %d раз\n", greet_calls());

    return serve_mode ? serve(name) : 0;
}
