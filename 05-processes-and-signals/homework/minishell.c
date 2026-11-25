/* minishell.c — маленькая оболочка: конвейер через |, перенаправление > файл,
 * встроенные cd и exit, Ctrl+C не убивает оболочку.
 *
 * Сборка: make
 * Запуск: ./minishell
 *
 * Чего нет: кавычек, переменных, ~, <, >>, 2>, фоновых задач (&).
 * На неподдерживаемое перенаправление оболочка ругается, а не гадает.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CMDS 8
#define MAX_ARGS 64

struct command {
    char *argv[MAX_ARGS + 1]; /* NULL в конце, как ждёт execvp */
    char *outfile;            /* куда перенаправить stdout, или NULL */
};

static volatile sig_atomic_t got_sigint = 0;

static void on_sigint(int signo)
{
    (void)signo;
    got_sigint = 1;
}

/* SIGINT ловим без SA_RESTART: тогда read() внутри getline() и waitpid()
 * прерываются с EINTR, и основной цикл сам решает, что делать дальше. */
static int install_sigint_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    return sigaction(SIGINT, &sa, NULL);
}

static void print_prompt(void)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof cwd) == NULL)
        strcpy(cwd, "?");
    printf("minishell:%s$ ", cwd);
    fflush(stdout);
}

/* 1, если в слове есть символ перенаправления. */
static int has_redirect(const char *word)
{
    return strpbrk(word, "<>") != NULL;
}

/* Делит один кусок между | на слова. Строку не копирует: argv указывает
 * внутрь line, пробелы заменяются на '\0'. Возвращает 0 или -1 при ошибке. */
static int parse_command(char *text, struct command *cmd)
{
    int argc = 0;
    char *save = NULL;

    cmd->outfile = NULL;
    for (char *tok = strtok_r(text, " \t\n", &save); tok != NULL;
         tok = strtok_r(NULL, " \t\n", &save)) {
        if (strncmp(tok, ">>", 2) == 0) {
            fprintf(stderr, "minishell: >> не поддерживается, только > файл\n");
            return -1;
        }
        if (tok[0] == '>') {
            /* и "> file", и ">file" */
            char *name = (tok[1] != '\0') ? tok + 1 : strtok_r(NULL, " \t\n", &save);
            if (name == NULL || has_redirect(name)) {
                fprintf(stderr, "minishell: после > нужно имя файла\n");
                return -1;
            }
            if (cmd->outfile != NULL) {
                fprintf(stderr, "minishell: в команде может быть только одно >\n");
                return -1;
            }
            cmd->outfile = name;
            continue;
        }
        if (has_redirect(tok)) {
            /* ls>out, 2>err, <in: молча передать их аргументом хуже, чем отказать */
            fprintf(stderr, "minishell: %s: поддерживается только \"> файл\"\n", tok);
            return -1;
        }
        if (argc == MAX_ARGS) {
            fprintf(stderr, "minishell: больше %d слов в команде\n", MAX_ARGS);
            return -1;
        }
        cmd->argv[argc++] = tok;
    }
    cmd->argv[argc] = NULL;

    if (argc == 0) {
        fprintf(stderr, "minishell: пустая команда\n");
        return -1;
    }
    return 0;
}

/* Делит строку по | и разбирает каждый кусок. Возвращает число команд,
 * 0 для пустой строки, -1 при ошибке. */
static int parse_line(char *line, struct command *cmds)
{
    if (strspn(line, " \t\n") == strlen(line))
        return 0;

    int n = 0;
    char *start = line;
    for (;;) {
        char *bar = strchr(start, '|');
        if (bar != NULL)
            *bar = '\0';
        if (n == MAX_CMDS) {
            fprintf(stderr, "minishell: больше %d команд в конвейере\n", MAX_CMDS);
            return -1;
        }
        if (parse_command(start, &cmds[n]) < 0)
            return -1;
        n++;
        if (bar == NULL)
            return n;
        start = bar + 1;
    }
}

static int is_builtin(const char *name)
{
    return strcmp(name, "cd") == 0 || strcmp(name, "exit") == 0;
}

/* exit [N]: N должно быть числом, иначе код 2, как у bash. */
static void builtin_exit(const struct command *cmd, int *status, int *should_exit)
{
    const char *arg = cmd->argv[1];

    if (arg != NULL && cmd->argv[2] != NULL) {
        fprintf(stderr, "minishell: exit: слишком много аргументов\n");
        *status = 1;
        return; /* как bash: не выходим */
    }
    if (arg != NULL) {
        char *end = NULL;
        errno = 0;
        long code = strtol(arg, &end, 10);
        if (end == arg || *end != '\0' || errno != 0) {
            fprintf(stderr, "minishell: exit: %s: нужно число\n", arg);
            *status = 2;
        } else {
            *status = (int)(code & 0xff);
        }
    }
    *should_exit = 1;
}

/* Встроенные команды выполняются в самой оболочке, без fork: chdir в
 * ребёнке поменял бы каталог только ребёнку. Возвращает 1, если команда
 * встроенная. */
static int run_builtin(const struct command *cmd, int *status, int *should_exit)
{
    const char *name = cmd->argv[0];

    if (!is_builtin(name))
        return 0;
    if (cmd->outfile != NULL) {
        fprintf(stderr, "minishell: %s: > у встроенных команд не поддерживается\n", name);
        *status = 2;
        return 1;
    }
    if (strcmp(name, "cd") == 0) {
        const char *dir = cmd->argv[1] ? cmd->argv[1] : getenv("HOME");
        if (dir == NULL)
            dir = "/";
        if (chdir(dir) < 0) {
            fprintf(stderr, "cd: %s: %s\n", dir, strerror(errno));
            *status = 1;
        } else {
            *status = 0;
        }
        return 1;
    }
    builtin_exit(cmd, status, should_exit);
    return 1;
}

/* dup2 и закрыть старый номер. Ошибку не глотаю: иначе команда тихо
 * запустится не с теми дескрипторами. */
static void move_fd(int from, int to)
{
    if (dup2(from, to) < 0) {
        perror("dup2");
        _exit(126);
    }
    close(from);
}

/* Код ребёнка между fork и exec. Сюда приходят: конец канала от
 * предыдущей команды (in_fd), конвейер к следующей (pipe_fd) и маска
 * сигналов, которая была до fork (на время fork SIGINT заблокирован). */
static void exec_child(const struct command *cmd, int in_fd, const int pipe_fd[2],
                       const sigset_t *old_mask)
{
    /* Пока SIGINT заблокирован, обработчик оболочки в ребёнке не сработает.
     * Сначала возвращаю действие по умолчанию, потом снимаю блокировку:
     * пришедший за это время Ctrl+C убьёт ребёнка, как и должен. SIGQUIT
     * оболочка игнорирует, а SIG_IGN пережил бы exec, поэтому и его. */
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    sigprocmask(SIG_SETMASK, old_mask, NULL);

    if (in_fd != -1)
        move_fd(in_fd, STDIN_FILENO);
    if (pipe_fd[1] != -1) {
        move_fd(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]); /* читать будет следующая команда, не я */
    }
    if (cmd->outfile != NULL) {
        int fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror(cmd->outfile);
            _exit(1);
        }
        move_fd(fd, STDOUT_FILENO);
    }

    execvp(cmd->argv[0], cmd->argv);
    fprintf(stderr, "minishell: %s: %s\n", cmd->argv[0], strerror(errno));
    _exit(127);
}

/* Ждёт ребёнка, переживая EINTR от Ctrl+C. Возвращает статус как в bash,
 * в *by_sigint отмечает, что ребёнка убил SIGINT. */
static int wait_child(pid_t pid, int *by_sigint)
{
    int wstatus = 0;
    while (waitpid(pid, &wstatus, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }
    if (WIFSIGNALED(wstatus)) {
        if (WTERMSIG(wstatus) == SIGINT)
            *by_sigint = 1;
        return 128 + WTERMSIG(wstatus);
    }
    return WEXITSTATUS(wstatus);
}

/* Запускает n команд, соединяя соседние каналом. Возвращает код последней. */
static int run_pipeline(const struct command *cmds, int n)
{
    pid_t pids[MAX_CMDS];
    int started = 0;
    int in_fd = -1; /* откуда читает очередная команда */

    fflush(stdout); /* чтобы недописанный буфер stdio не скопировался в детей */

    sigset_t sigint_only, old_mask;
    sigemptyset(&sigint_only);
    sigaddset(&sigint_only, SIGINT);

    for (int i = 0; i < n; i++) {
        int pipe_fd[2] = { -1, -1 };
        if (i < n - 1 && pipe(pipe_fd) < 0) {
            perror("pipe");
            break;
        }

        sigprocmask(SIG_BLOCK, &sigint_only, &old_mask);
        pid_t pid = fork();
        if (pid == 0)
            exec_child(&cmds[i], in_fd, pipe_fd, &old_mask);
        sigprocmask(SIG_SETMASK, &old_mask, NULL);

        if (pid < 0) {
            perror("fork");
            if (pipe_fd[0] != -1) {
                close(pipe_fd[0]);
                close(pipe_fd[1]);
            }
            break;
        }
        pids[started++] = pid;

        /* Родителю концы канала не нужны. Если не закрыть пишущий конец,
         * следующая команда никогда не получит EOF. */
        if (in_fd != -1)
            close(in_fd);
        if (pipe_fd[1] != -1)
            close(pipe_fd[1]);
        in_fd = pipe_fd[0];
    }
    if (in_fd != -1)
        close(in_fd); /* остаётся, только если цикл прервался по ошибке */

    /* Ждём всех, а не только последнего: иначе остальные станут зомби. */
    int status = 1;
    int by_sigint = 0;
    for (int i = 0; i < started; i++)
        status = wait_child(pids[i], &by_sigint);
    if (by_sigint)
        putchar('\n'); /* один перевод строки на весь конвейер, как в bash */
    if (started < n)
        status = 1;
    return status;
}

/* cd и exit в конвейере ничего бы не сделали: они попали бы в ребёнка. */
static int builtin_in_pipeline(const struct command *cmds, int n)
{
    for (int i = 0; i < n; i++) {
        if (is_builtin(cmds[i].argv[0])) {
            fprintf(stderr, "minishell: %s: в конвейере не поддерживается\n",
                    cmds[i].argv[0]);
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    if (install_sigint_handler() < 0) {
        perror("sigaction");
        return 1;
    }

    int interactive = isatty(STDIN_FILENO);
    if (interactive) {
        signal(SIGQUIT, SIG_IGN); /* Ctrl+\ не должен убивать оболочку */
    } else {
        /* Без буфера getline читает по байту и не забирает чужие строки:
         * в printf 'cat\nhello\n' | ./minishell строку hello прочтёт cat. */
        setvbuf(stdin, NULL, _IONBF, 0);
    }
    char *line = NULL;
    size_t cap = 0;
    int status = 0;
    int should_exit = 0;
    struct command cmds[MAX_CMDS];

    while (!should_exit) {
        if (interactive)
            print_prompt();

        errno = 0;
        if (getline(&line, &cap, stdin) < 0) {
            if (errno == EINTR && got_sigint) {
                /* Ctrl+C во время ввода: начать новую строку. */
                clearerr(stdin);
                got_sigint = 0;
                putchar('\n');
                continue;
            }
            if (interactive)
                putchar('\n'); /* Ctrl+D: выйти с новой строки */
            break;
        }

        int n = parse_line(line, cmds);
        if (n < 0) {
            status = 2;
            continue;
        }
        if (n == 0)
            continue;

        if (n == 1 && run_builtin(&cmds[0], &status, &should_exit))
            continue;
        if (n > 1 && builtin_in_pipeline(cmds, n)) {
            status = 2;
            continue;
        }

        status = run_pipeline(cmds, n);
        got_sigint = 0;
    }

    free(line);
    return status;
}
