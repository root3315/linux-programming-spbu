# Практика 1. Карта процессов

Контейнер с Ubuntu 24.04, вошёл обычным пользователем `stepan` через `su - stepan`, поэтому
родитель моей оболочки — `su`, а не `sshd`, как на слайде. Сначала скопировал примеры и
собрал их.

## Подготовка

```text
stepan@devbox:~$ mkdir -p ~/proc-lab && cd ~/proc-lab
stepan@devbox:~/proc-lab$ cp -r ~/Chapter05/. .
stepan@devbox:~/proc-lab$ for s in *.c; do gcc -Wall -Wextra -pthread -o "${s%.c}" "$s"; done
stepan@devbox:~/proc-lab$ ls
README.md  exec_ls.c   fork_basic.c  mem_layout.c  pipe_demo.c	pipe_dup2.c  signals.c	threads.c  zombie-finder.sh
exec_ls    fork_basic  mem_layout    pipe_demo	   pipe_dup2	signals      threads	zombie	   zombie.c
```

Все восемь примеров собрались без предупреждений.

## 1. Мой PID и родитель

Предсказание: PID какой-то небольшой (контейнер свежий), родитель — `su`.

```text
stepan@devbox:~/proc-lab$ echo $$
30
stepan@devbox:~/proc-lab$ ps -o pid,ppid,comm -p $$
    PID    PPID COMMAND
     30      29 bash
```

`$$` — PID самой оболочки. PPID 29 — это `su`, через который я вошёл.

## 2. Два запуска одной программы

На слайде `./threads &`, но с мьютексом он считает до миллиона быстро, и я боялся, что `ps`
его не застанет. Поэтому запустил два раза и `ps` в той же строке:

```text
stepan@devbox:~/proc-lab$ ./threads & ./threads & ps -C threads -o pid,stat,nlwp,comm
[1] 85
[2] 86
    PID STAT NLWP COMMAND
     85 Sl      5 threads
     86 Sl      5 threads
stepan@devbox:~/proc-lab$ режим: с мьютексом
ожидалось: 1000000
получено:  1000000
режим: с мьютексом
ожидалось: 1000000
получено:  1000000
jobs
[1]-  Done                    ./threads
[2]+  Done                    ./threads
```

Одна программа, два процесса с разными PID. `NLWP 5`: главный поток и четыре рабочих.
`l` в `Sl` значит «многопоточный». Вывод программ пришёл уже после приглашения, поэтому
`jobs` напечатался без него.

## 3. Дерево

```text
stepan@devbox:~/proc-lab$ pstree -p $$
bash(30)───pstree(96)
```

Ждал увидеть что-то похожее на слайд с `systemd(1)`, но `pstree -p $$` рисует дерево
только от моей оболочки вниз. Её единственный ребёнок в этот момент — сам `pstree`.

## 4. /proc/$$/status

```text
stepan@devbox:~/proc-lab$ head -9 /proc/$$/status
Name:	bash
Umask:	0002
State:	S (sleeping)
Tgid:	30
Ngid:	0
Pid:	30
PPid:	29
TracerPid:	0
Uid:	1000	1000	1000	1000
stepan@devbox:~/proc-lab$ grep -E '^(State|PPid|Threads)' /proc/$$/status
State:	S (sleeping)
PPid:	29
Threads:	1
stepan@devbox:~/proc-lab$ grep State /proc/self/status
State:	R (running)
```

Оболочка в `S`: пока работает `head` или `grep`, bash спит в `wait` и ждёт ребёнка. А
`/proc/self` — это тот процесс, который читает файл, то есть сам `grep`, и он в момент
чтения выполняется, поэтому `R`. `Tgid` — номер группы потоков, у главного потока он равен
`Pid`, а `Threads: 1` говорит, что других потоков у bash нет.

## 5. Дескрипторы оболочки

```text
stepan@devbox:~/proc-lab$ ls -l /proc/$$/fd
total 0
lrwx------ 1 stepan stepan 64 Nov 23 20:33 0 -> /dev/pts/0
lrwx------ 1 stepan stepan 64 Nov 23 20:33 1 -> /dev/pts/0
lrwx------ 1 stepan stepan 64 Nov 23 20:33 2 -> /dev/pts/0
lrwx------ 1 stepan stepan 64 Nov 23 20:33 255 -> /dev/pts/0
```

0, 1 и 2 смотрят на один терминал: я в нём печатаю (stdin), в него приходит вывод и ошибки.
Лишний 255 bash открывает сам: это его копия терминала для управления заданиями, она не
зависит от того, куда перенаправят 0–2.

## 6. Аргументы глазами ядра

```text
stepan@devbox:~/proc-lab$ cat /proc/$$/cmdline | tr '\0' ' '; echo
-bash 
stepan@devbox:~/proc-lab$ cat /proc/self/cmdline | tr '\0' ' '; echo
cat /proc/self/cmdline 
```

Аргументы лежат через `\0`, `tr` превращает их в пробелы. У моей оболочки `argv[0]` равен
`-bash`: минус в начале значит login shell, так её запустил `su -`.

## Что понял

`State: S` — это не «завис», а «ждёт события», и почти все процессы большую часть времени
в нём. `R` видно только у того, кто прямо сейчас работает, как `grep` в `/proc/self`. Три
дескриптора на одном терминале появляются потому, что терминал открыт один раз и дальше
наследуется через `fork`.
