/* pipe3.c — конвейер из трёх процессов: ls -1 <каталог> | sort | head -3.
 *
 * Сделан по образцу pipe_dup2.c из Chapter 05: там два процесса и один
 * канал, здесь три процесса и два канала.
 *
 * Сборка: gcc -std=c11 -Wall -Wextra -o pipe3 pipe3.c
 * Запуск: ./pipe3 [каталог]
 * Сравнить: ls -1 <каталог> | sort | head -3
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

/* Запускает программу с stdin = in и stdout = out. Все четыре конца
 * каналов ребёнку не нужны: после dup2 он их закрывает, иначе sort
 * или head не дождутся EOF. */
static pid_t spawn(char *const argv[], int in, int out, const int fds[4])
{
    pid_t pid = fork();
    if (pid != 0)
        return pid;

    if (in != STDIN_FILENO)
        dup2(in, STDIN_FILENO);
    if (out != STDOUT_FILENO)
        dup2(out, STDOUT_FILENO);
    for (int i = 0; i < 4; i++)
        close(fds[i]);
    execvp(argv[0], argv);
    perror(argv[0]);
    _exit(127);
}

int main(int argc, char **argv)
{
    char *dir = (argc > 1) ? argv[1] : ".";
    int a[2], b[2]; /* a: ls -> sort, b: sort -> head */

    if (pipe(a) < 0 || pipe(b) < 0) {
        perror("pipe");
        return 1;
    }
    const int fds[4] = { a[0], a[1], b[0], b[1] };

    char *ls[]   = { "ls", "-1", dir, NULL };
    char *sort[] = { "sort", NULL };
    char *head[] = { "head", "-3", NULL };

    pid_t pids[3];
    pids[0] = spawn(ls, STDIN_FILENO, a[1], fds);
    pids[1] = spawn(sort, a[0], b[1], fds);
    pids[2] = spawn(head, b[0], STDOUT_FILENO, fds);

    /* Родитель ничего не читает и не пишет: закрываем всё. */
    for (int i = 0; i < 4; i++)
        close(fds[i]);

    int status = 0;
    for (int i = 0; i < 3; i++) {
        if (pids[i] < 0) {
            perror("fork");
            continue;
        }
        waitpid(pids[i], &status, 0);
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
