# Практика 3. Обработчик сигналов

`signals.c` из Chapter 05 ставит через `sigaction` один обработчик на `SIGINT`, `SIGTERM`
и `SIGUSR1`. Работал, как на слайде, в двух терминалах: в первом программа на переднем
плане, во втором `kill`.

## 1–4. USR1, INT, TERM

Первый терминал:

```text
stepan@devbox:~/proc-lab$ ./signals
pid=234. Жду сигналов: Ctrl+C, kill -USR1 234, kill -TERM 234
Выход: kill -KILL 234 (SIGKILL перехватить нельзя)
SIGUSR1
получен сигнал 10, продолжаю работу
SIGINT (Ctrl+C)
получен сигнал 2, продолжаю работу
^CSIGINT (Ctrl+C)
получен сигнал 2, продолжаю работу
SIGTERM
SIGTERM обработан — завершаюсь штатно
```

Второй терминал, по очереди, с паузами между командами:

```text
stepan@devbox:~$ kill -USR1 $(pgrep -x signals)
stepan@devbox:~$ kill -INT $(pgrep -x signals)
stepan@devbox:~$ kill -TERM $(pgrep -x signals)
```

Между `kill -INT` и `kill -TERM` я нажал Ctrl+C в первом терминале, это строка с `^C`.
Ctrl+C и `kill -INT` для программы неотличимы: обе строки одинаковые, `SIGINT (Ctrl+C)`.
Строку с именем сигнала печатает обработчик через `write`, строку «получен сигнал N» —
основной цикл после `pause()`. На `SIGTERM` цикл заканчивается, и программа выходит сама.

## 5–6. Ctrl+Z, T, CONT, STOP, KILL

```text
stepan@devbox:~/proc-lab$ ./signals
pid=244. Жду сигналов: Ctrl+C, kill -USR1 244, kill -TERM 244
Выход: kill -KILL 244 (SIGKILL перехватить нельзя)
^Z
[1]+  Stopped                 ./signals
stepan@devbox:~/proc-lab$ jobs
[1]+  Stopped                 ./signals
stepan@devbox:~/proc-lab$ ps -o pid,stat,comm -C signals
    PID STAT COMMAND
    244 T    signals
stepan@devbox:~/proc-lab$ kill -CONT %1
stepan@devbox:~/proc-lab$ ps -o pid,stat,comm -C signals
    PID STAT COMMAND
    244 S    signals
stepan@devbox:~/proc-lab$ kill -STOP %1
stepan@devbox:~/proc-lab$ ps -o pid,stat,comm -C signals
    PID STAT COMMAND
    244 T    signals

[1]+  Stopped                 ./signals
stepan@devbox:~/proc-lab$ kill -KILL %1

[1]+  Stopped                 ./signals
stepan@devbox:~/proc-lab$ jobs
[1]+  Killed                  ./signals
stepan@devbox:~/proc-lab$ ps -o pid,stat,comm -C signals
    PID STAT COMMAND
```

Ctrl+Z остановил программу, `ps` показал `T`. После `SIGCONT` процесс снова `S`: он спит в
`pause()`, хотя работает уже в фоне. bash сообщает «Stopped» с опозданием, перед
следующим приглашением, поэтому строка про `STOP` вылезла уже после `ps`. После `SIGKILL`
нет ни строки от обработчика, процесс просто исчез, bash пишет `Killed`. `SIGKILL` убил даже
остановленный процесс, будить его через `CONT` не понадобилось.

`T` и `Z` — разные вещи: `T` — живой процесс, у него есть память и дескрипторы, его можно
продолжить. `Z` уже завершился, от него осталась одна запись.

## Что понял

`SIGKILL` и `SIGSTOP` ядро обрабатывает само, до обработчиков процесса дело не доходит:
`sigaction` на них возвращает ошибку `EINVAL` (проверил маленькой программой). Так у
системы всегда есть способ остановить или убить любой процесс. Ctrl+Z посылает другой
сигнал, `SIGTSTP`, и его программа может перехватить, `signals.c` этого просто не делает.
