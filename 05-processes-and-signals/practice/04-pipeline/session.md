# Практика 4. Конвейер на C

Основа — `pipe_dup2.c` из Chapter 05. Сломанные копии `pipe_keep_read.c` и `pipe_hang.c`
делал из него через `sed`, они нужны были на один запуск, в репозиторий их не кладу: `diff`
ниже показывает всю правку. Свой файл тут один, [pipe3.c](pipe3.c), конвейер из трёх
процессов.

## 1. Сравнение с оболочкой

```text
stepan@devbox:~$ cd ~/proc-lab
stepan@devbox:~/proc-lab$ ./pipe_dup2 /etc
112
stepan@devbox:~/proc-lab$ ls -1 /etc | wc -l
112
```

## 2. Писатель не закрыл fd[0]

Предсказание: ничего не сломается. Висеть может только тот, кто читает, а `ls` не читает.

```text
stepan@devbox:~/proc-lab$ sed '0,/close(fd\[0\]);/{/close(fd\[0\]);/d}' pipe_dup2.c > pipe_keep_read.c
stepan@devbox:~/proc-lab$ diff pipe_dup2.c pipe_keep_read.c
28d27
<         close(fd[0]);
stepan@devbox:~/proc-lab$ gcc -Wall -Wextra -o pipe_keep_read pipe_keep_read.c && ./pipe_keep_read /etc
112
```

Так и есть. Лишний читающий конец у писателя мешает в другом случае: если читатель умрёт
раньше (как `head`), писатель не получит `SIGPIPE`, ведь читатель у канала формально ещё
есть.

## 3. Родитель не закрыл fd[1]

```text
stepan@devbox:~/proc-lab$ sed '/^    close(fd\[1\]);/d' pipe_dup2.c > pipe_hang.c
stepan@devbox:~/proc-lab$ diff pipe_dup2.c pipe_hang.c
47d46
<     close(fd[1]);
stepan@devbox:~/proc-lab$ gcc -Wall -Wextra -o pipe_hang pipe_hang.c
stepan@devbox:~/proc-lab$ ./pipe_hang /etc &
[1] 297
stepan@devbox:~/proc-lab$ ps -o pid,ppid,stat,wchan:20,comm -C pipe_hang,wc
    PID    PPID STAT WCHAN                COMMAND
    297     264 S    do_wait              pipe_hang
    299     297 S    anon_pipe_read       wc
stepan@devbox:~/proc-lab$ ls -l /proc/$!/fd
total 0
lrwx------ 1 stepan stepan 64 Nov 23 20:59 0 -> /dev/pts/0
lrwx------ 1 stepan stepan 64 Nov 23 20:59 1 -> /dev/pts/0
lrwx------ 1 stepan stepan 64 Nov 23 20:59 2 -> /dev/pts/0
l-wx------ 1 stepan stepan 64 Nov 23 20:59 4 -> 'pipe:[2109313]'
stepan@devbox:~/proc-lab$ ls -l /proc/$(pgrep -x wc)/fd
total 0
lr-x------ 1 stepan stepan 64 Nov 23 20:59 0 -> 'pipe:[2109313]'
lrwx------ 1 stepan stepan 64 Nov 23 20:59 1 -> /dev/pts/0
lrwx------ 1 stepan stepan 64 Nov 23 20:59 2 -> /dev/pts/0
stepan@devbox:~/proc-lab$ kill $!
stepan@devbox:~/proc-lab$ 112
jobs
[1]+  Terminated              ./pipe_hang /etc
```

Всё видно по `/proc`. У `wc` stdin — канал `2109313`, и у родителя на тот же канал открыт
дескриптор 4 на запись (`l-wx`). `ls` давно закончил, но писатель у канала ещё есть,
поэтому `wc` спит в ядре в `anon_pipe_read` и ждёт данных. На слайде функция называлась
`pipe_read`, у меня ядро 6.18, видимо, её переименовали. Родитель тем временем спит в
`do_wait`, ждёт `wc`. Друг друга они не дождутся.

Как только я убил родителя, ядро закрыло его дескрипторы, у канала не осталось писателей,
`wc` получил EOF и сразу напечатал `112`. Значит, канал держал именно забытый
`close(fd[1])` в родителе.

## 4. Три процесса: ls | sort | head -3

Написал [pipe3.c](pipe3.c): два канала, три `fork`, в каждом ребёнке `dup2` и закрытие
всех четырёх концов, в родителе тоже закрываются все четыре.

```text
stepan@devbox:~/proc-lab$ gcc -std=c11 -Wall -Wextra -o pipe3 pipe3.c
stepan@devbox:~/proc-lab$ ./pipe3 /etc
adduser.conf
alternatives
apparmor.d
stepan@devbox:~/proc-lab$ ls -1 /etc | sort | head -3
adduser.conf
alternatives
apparmor.d
```

## 5. strace

Команда со слайда:

```text
stepan@devbox:~/proc-lab$ strace -f -e trace=pipe,dup2,clone ./pipe_dup2 /etc
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLDstrace: Process 320 attached
, child_tidptr=0x7341b51c6a10) = 320
[pid   319] clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD <unfinished ...>
[pid   320] dup2(4, 1)                  = 1
strace: Process 321 attached
[pid   319] <... clone resumed>, child_tidptr=0x7341b51c6a10) = 321
[pid   321] dup2(3, 0)                  = 0
112
[pid   320] +++ exited with 0 +++
[pid   319] --- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=320, si_uid=1000, si_status=0, si_utime=0, si_stime=0} ---
[pid   321] +++ exited with 0 +++
--- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=321, si_uid=1000, si_status=0, si_utime=0, si_stime=0} ---
+++ exited with 0 +++
```

Вызова `pipe` нет вообще. glibc на x86-64 реализует `pipe()` через системный вызов `pipe2`
с флагами 0, а фильтр `trace=pipe` ловит только старый `pipe`. Добавил `pipe2` и `execve`:

```text
stepan@devbox:~/proc-lab$ strace -f -e trace=pipe,pipe2,dup2,clone,execve ./pipe_dup2 /etc
execve("./pipe_dup2", ["./pipe_dup2", "/etc"], 0x7ffe6c775e70 /* 15 vars */) = 0
pipe2([3, 4], 0)                        = 0
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLDstrace: Process 326 attached
, child_tidptr=0x7b1478039a10) = 326
[pid   325] clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD <unfinished ...>
[pid   326] dup2(4, 1strace: Process 327 attached
)                  = 1
[pid   325] <... clone resumed>, child_tidptr=0x7b1478039a10) = 327
[pid   327] dup2(3, 0)                  = 0
...
[pid   326] execve("/usr/bin/ls", ["ls", "-1", "/etc"], 0x7fffed694ef0 /* 15 vars */ <unfinished ...>
[pid   327] <... execve resumed>)       = -1 ENOENT (No such file or directory)
[pid   327] execve("/usr/bin/wc", ["wc", "-l"], 0x7fffed694ef0 /* 15 vars */) = 0
[pid   326] <... execve resumed>)       = 0
112
...
```

Порядок как в коде: `pipe2` дал дескрипторы 3 (чтение) и 4 (запись), потом два `clone`,
в первом ребёнке `dup2(4, 1)` и `execve ls`, во втором `dup2(3, 0)` и `execve wc`. Строки
от двух детей перемешаны: `strace` разрезает вызов на `<unfinished ...>` и `resumed`, когда
между ними вклинивается другой процесс.

На трёх процессах:

```text
stepan@devbox:~/proc-lab$ strace -f -e trace=pipe2,dup2,clone ./pipe3 /etc
pipe2([3, 4], 0)                        = 0
pipe2([5, 6], 0)                        = 0
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLDstrace: Process 332 attached
, child_tidptr=0x7034b4357a10) = 332
[pid   331] clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD <unfinished ...>
[pid   332] dup2(4, 1)                  = 1
strace: Process 333 attached
[pid   331] <... clone resumed>, child_tidptr=0x7034b4357a10) = 333
[pid   331] clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD <unfinished ...>
[pid   333] dup2(3, 0)                  = 0
strace: Process 334 attached
[pid   331] <... clone resumed>, child_tidptr=0x7034b4357a10) = 334
[pid   333] dup2(6, 1)                  = 1
[pid   334] dup2(5, 0)                  = 0
[pid   332] +++ exited with 0 +++
[pid   331] --- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=332, si_uid=1000, si_status=0, si_utime=0, si_stime=1 /* 0.01 s */} ---
adduser.conf
alternatives
apparmor.d
...
```

`ls` (332) пишет в 4, `sort` (333) читает из 3 и пишет в 6, `head` (334) читает из 5.

## Что понял

Читатель канала получает EOF, только когда закрыты все пишущие концы во всех процессах,
включая родителя, которому канал вообще не нужен. Забытый конец не даёт ошибку, а даёт
тихое вечное ожидание: в `ps` — `S` и функция чтения канала в `WCHAN`, в `/proc/<pid>/fd`
лишний `pipe:[...]` с правом записи.
