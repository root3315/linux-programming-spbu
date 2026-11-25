# Практики занятия 5

Все примеры из архива Chapter 05 преподавателя (`fork_basic.c`, `exec_ls.c`, `zombie.c`,
`signals.c`, `pipe_dup2.c` и остальные). Как на слайдах, скопировал их в `~/proc-lab` и
собрал там, исходники архива не трогал:

```bash
mkdir -p ~/proc-lab && cd ~/proc-lab
cp -r ~/Chapter05/. .
for s in *.c; do gcc -Wall -Wextra -pthread -o "${s%.c}" "$s"; done
```

В папках ниже только моё: сессии с командами и выводом и файлы, которые я написал сам.
Правки примеров (убрать `close`, добавить `fflush`) делал копией через `sed`, команды
и `diff` есть в сессиях.

Запускал в контейнере с Ubuntu 24.04. Для `strace -p` и `gdb -p` контейнеру нужен
`--cap-add SYS_PTRACE`:

```bash
docker run --rm -it --cap-add SYS_PTRACE -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential strace gdb psmisc procps
```

## Практика 1. Карта процессов

Для каждого пункта сначала предсказать вывод. `echo $$` и `ps -o pid,ppid,comm -p $$`;
запустить `./threads &` и найти его через `ps -C threads`; найти себя в `pstree -p $$`;
в `/proc/$$/status` найти `State`, `PPid`, `Threads`; посмотреть `ls -l /proc/$$/fd` и
`/proc/$$/cmdline`. Критерий: объяснить, чем `State: S` отличается от `R` и почему у
оболочки три дескриптора указывают на один терминал.

Решение: [01-process-map/session.md](01-process-map/session.md). Своих файлов нет.

## Практика 2. fork, exec, wait

На `fork_basic.c`, `exec_ls.c` и `zombie.c`. Объяснить, почему строка «до fork»
печатается дважды, и что меняется, если убрать `\n` из второго `printf`. Почему после
`execvp` код не продолжается. Какой код вернул `ls` в `./exec_ls /nope` и как его увидел
родитель.
Поймать зомби через `./zombie 120 &` и `ps`, потом то же с `wait`, и `zombie-finder.sh` до
и после. Критерий: объяснить, почему зомби исчезает после `wait` и почему на него не
действует `kill -9`.

Решение: [02-fork-exec-wait/session.md](02-fork-exec-wait/session.md). Своих файлов нет.

## Практика 3. Обработчик сигналов

На `signals.c`, нужны два терминала: в одном программа, в другом `kill`. Послать
`SIGUSR1`, `SIGINT` (и через Ctrl+C), `SIGTERM`. Нажать Ctrl+Z и сравнить состояние `T` с
`Z`. Убить через `kill -KILL` и убедиться, что обработчик не вызвался. Критерий: объяснить,
почему `SIGKILL` не перехватывается и чем `SIGSTOP` отличается от Ctrl+Z (`SIGTSTP`).

Решение: [03-signals/session.md](03-signals/session.md). Своих файлов нет.

## Практика 4. Конвейер на C

Сравнить `./pipe_dup2 /etc` с `ls -1 /etc | wc -l`. Убрать `close(fd[0])` в
ребёнке-писателе, потом `close(fd[1])` в родителе и объяснить, почему `wc` завис. Собрать
цепочку из трёх процессов `ls | sort | head -3`. Посмотреть на всё через
`strace -f -e trace=pipe,dup2,clone` и `ls -l /proc/<pid>/fd`. Критерий: объяснить, почему
незакрытый конец канала превращается в вечное ожидание и как это увидеть в `ps`.

Решение: [04-pipeline/](04-pipeline/): мой [pipe3.c](04-pipeline/pipe3.c) и
[session.md](04-pipeline/session.md).

```bash
gcc -std=c11 -Wall -Wextra -o pipe3 04-pipeline/pipe3.c && ./pipe3 /etc
```

## Задание РПД №5. Скрипт зомби-процессов

На слайде отдельно: «напишите скрипт, который выводит все зомби-процессы». В Chapter 05
уже лежит решение преподавателя, `zombie-finder.sh`, оно читает `/proc/<pid>/stat`. Я
написал свой вариант через `ps`, который заодно печатает родителя зомби, и проверил оба
на процессе с пробелом в имени.

Решение: [05-zombie-script/](05-zombie-script/): [zombies.sh](05-zombie-script/zombies.sh)
и [session.md](05-zombie-script/session.md).

```bash
sh 05-zombie-script/zombies.sh
```
