# Практика 2. fork, exec, wait

Работал в `~/proc-lab` с собранными примерами (см. практику 1). Правленые копии
`fork_no_nl.c`, `fork_nl.c` и `fork_flush.c` делал из `fork_basic.c` из Chapter 05 через
`sed`, исходник не трогал. Весь `diff` есть ниже, поэтому сами копии в репозиторий не кладу.

## 1. Почему «до fork» напечатано дважды

```text
stepan@devbox:~$ cd ~/proc-lab
stepan@devbox:~/proc-lab$ ./fork_basic
до fork (без \n, строка в буфере)
[родитель] pid=115, мой ребёнок=116
до fork (без \n, строка в буфере)
[ребёнок]  pid=116, родитель=115
[родитель] ребёнок завершился с кодом 0
stepan@devbox:~/proc-lab$ ./fork_basic > out.txt; cat out.txt
до fork (без \n, строка в буфере)
[ребёнок]  pid=118, родитель=117
до fork (без \n, строка в буфере)
[родитель] pid=117, мой ребёнок=118
[родитель] ребёнок завершился с кодом 0
```

Первый `printf` без `\n`, строка остаётся в буфере stdout. `fork` копирует память, а буфер —
часть памяти, так что недописанная строка есть у обоих. Каждый процесс потом сбрасывает
свою копию, отсюда две строки. Порядок родителя и ребёнка от запуска к запуску разный, его
решает планировщик.

## 2. Убрать `\n` из второго printf

Сначала посмотрел, какой `printf` второй:

```text
stepan@devbox:~/proc-lab$ grep -n 'printf("' fork_basic.c
19:    printf("до fork (без \\n, строка в буфере)");
30:        printf("\n[ребёнок]  pid=%d, родитель=%d\n", (int)getpid(), (int)getppid());
34:        printf("\n[родитель] pid=%d, мой ребёнок=%d\n", (int)getpid(), (int)pid);
41:        printf("[родитель] ребёнок завершился с кодом %d\n", WEXITSTATUS(status));
```

Второй — у ребёнка, строка 30. Убрал у него `\n` в начале:

```text
stepan@devbox:~/proc-lab$ sed 's/printf("\\n\[ребёнок\]/printf("[ребёнок]/' fork_basic.c > fork_no_nl.c
stepan@devbox:~/proc-lab$ diff fork_basic.c fork_no_nl.c
30c30
<         printf("\n[ребёнок]  pid=%d, родитель=%d\n", (int)getpid(), (int)getppid());
---
>         printf("[ребёнок]  pid=%d, родитель=%d\n", (int)getpid(), (int)getppid());
stepan@devbox:~/proc-lab$ gcc -Wall -Wextra -o fork_no_nl fork_no_nl.c
stepan@devbox:~/proc-lab$ ./fork_no_nl
до fork (без \n, строка в буфере)
[родитель] pid=128, мой ребёнок=129
до fork (без \n, строка в буфере)[ребёнок]  pid=129, родитель=128
[родитель] ребёнок завершился с кодом 0
stepan@devbox:~/proc-lab$ ./fork_no_nl > out.txt; cat out.txt
до fork (без \n, строка в буфере)[ребёнок]  pid=131, родитель=130
до fork (без \n, строка в буфере)
[родитель] pid=130, мой ребёнок=131
[родитель] ребёнок завершился с кодом 0
```

Теперь видно, откуда берётся вторая копия «до fork»: она приклеилась к строке ребёнка. Раньше
её отделял как раз этот `\n`. Копия у ребёнка — недописанный буфер, и первым, что ребёнок в
него допишет, оказывается `[ребёнок] ...`. У родителя строка по-прежнему начинается с `\n`,
поэтому его копия стоит отдельно.

Потом проверил обратное: поставил `\n` в первый `printf`, чтобы строка закончилась до
`fork`.

```text
stepan@devbox:~/proc-lab$ sed 's/printf("до fork (без \\\\n, строка в буфере)");/printf("до fork\\n");/' fork_basic.c > fork_nl.c
stepan@devbox:~/proc-lab$ diff fork_basic.c fork_nl.c
19c19
<     printf("до fork (без \\n, строка в буфере)");
---
>     printf("до fork\n");
stepan@devbox:~/proc-lab$ gcc -Wall -Wextra -o fork_nl fork_nl.c
stepan@devbox:~/proc-lab$ ./fork_nl
до fork

[родитель] pid=140, мой ребёнок=141

[ребёнок]  pid=141, родитель=140
[родитель] ребёнок завершился с кодом 0
stepan@devbox:~/proc-lab$ ./fork_nl > out.txt; cat out.txt
до fork

[ребёнок]  pid=143, родитель=142
до fork

[родитель] pid=142, мой ребёнок=143
[родитель] ребёнок завершился с кодом 0
```

В терминале строка одна: stdout там буферизуется построчно, `\n` сразу сбросил буфер, и
копировать было нечего. В файл снова две: при выводе в файл буфер блочный, `\n` его не
сбрасывает. Пустые строки появились потому, что `printf` у ребёнка и родителя начинается
с `\n`, а предыдущая строка теперь уже закончена.

Работающий вариант, как на слайде, — `fflush(stdout)` перед `fork`:

```text
stepan@devbox:~/proc-lab$ sed 's/^    pid_t pid = fork();/    fflush(stdout);\n    pid_t pid = fork();/' fork_nl.c > fork_flush.c
stepan@devbox:~/proc-lab$ diff fork_nl.c fork_flush.c
20a21
>     fflush(stdout);
stepan@devbox:~/proc-lab$ gcc -Wall -Wextra -o fork_flush fork_flush.c && ./fork_flush > out.txt; cat out.txt
до fork

[ребёнок]  pid=153, родитель=152

[родитель] pid=152, мой ребёнок=153
[родитель] ребёнок завершился с кодом 0
```

## 3. exec_ls

```text
stepan@devbox:~/proc-lab$ ./exec_ls /etc | tail -4
-rw-r--r-- 1 root root    4942 Aug 20 00:00 wgetrc
-rw-r--r-- 1 root root     681 Apr  8  2024 xattr.conf

[родитель] ls завершился с кодом 0
```

После `execvp` в ребёнке уже `ls`, кода `exec_ls` там больше нет, поэтому `perror` и
`_exit(127)` не выполнились. Строку про код печатает родитель, он `exec` не делал.

Как это выглядит в `strace`:

```text
stepan@devbox:~/proc-lab$ strace -f -e trace=clone,execve ./exec_ls /tmp
execve("./exec_ls", ["./exec_ls", "/tmp"], 0x7ffede274100 /* 15 vars */) = 0
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLDstrace: Process 164 attached
, child_tidptr=0x7c3022f15a10) = 164
[pid   164] execve("/usr/local/sbin/ls", ["ls", "-l", "--color=never", "/tmp"], 0x7ffe8c564510 /* 15 vars */) = -1 ENOENT (No such file or directory)
[pid   164] execve("/usr/local/bin/ls", ["ls", "-l", "--color=never", "/tmp"], 0x7ffe8c564510 /* 15 vars */) = -1 ENOENT (No such file or directory)
[pid   164] execve("/usr/sbin/ls", ["ls", "-l", "--color=never", "/tmp"], 0x7ffe8c564510 /* 15 vars */) = -1 ENOENT (No such file or directory)
[pid   164] execve("/usr/bin/ls", ["ls", "-l", "--color=never", "/tmp"], 0x7ffe8c564510 /* 15 vars */) = 0
total 0
[pid   164] +++ exited with 0 +++
--- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=164, si_uid=1000, si_status=0, si_utime=0, si_stime=2 /* 0.02 s */} ---

[родитель] ls завершился с кодом 0
+++ exited with 0 +++
```

`fork` из glibc — это `clone`. Три `ENOENT` — это `execvp` перебирает `PATH`, пока не найдёт
`/usr/bin/ls`. PID у ребёнка до и после `exec` один и тот же, 164.

## 4. Код возврата ls

```text
stepan@devbox:~/proc-lab$ ./exec_ls /nope; echo "код=$?"
ls: cannot access '/nope': No such file or directory

[родитель] ls завершился с кодом 2
код=0
```

`ls` вернул 2, родитель достал его через `WEXITSTATUS(status)`. Наружу `exec_ls` вернул
свой `return 0`, поэтому bash видит 0.

## 5–6. Зомби

Перед зомби хотел посмотреть, что зомби нет, и сразу споткнулся:

```text
stepan@devbox:~/proc-lab$ ./zombie-finder.sh
-bash: ./zombie-finder.sh: cannot execute: required file not found
stepan@devbox:~/proc-lab$ sed -i 's/\r$//' zombie-finder.sh
stepan@devbox:~/proc-lab$ ./zombie-finder.sh
PID      PPID     COMMAND
(зомби не найдено)
```

Файлы в архиве с концами строк CRLF. В первой строке получилось `#!/bin/sh\r`, а файла
`/bin/sh\r` нет, отсюда «required file not found», хотя сам скрипт на месте. `.c` с CRLF gcc
собирает спокойно, поэтому раньше я этого не замечал.

Теперь зомби. Взял 120 секунд вместо 20, чтобы успеть всё посмотреть:

```text
stepan@devbox:~/proc-lab$ ./zombie 120 &
[1] 174
stepan@devbox:~/proc-lab$ [родитель 174] ребёнок 175 завершился, ждём 120 с
[ребёнок 175] работаю и выхожу
смотрите: ps -o pid,ppid,stat,comm -p 175
ps -o pid,ppid,stat,comm --ppid $!
    PID    PPID STAT COMMAND
    175     174 Z    zombie
stepan@devbox:~/proc-lab$ ./zombie-finder.sh
PID      PPID     COMMAND
175      174      zombie
stepan@devbox:~/proc-lab$ kill -9 $(pgrep -P $!)
stepan@devbox:~/proc-lab$ ps -o pid,ppid,stat,comm --ppid $!
    PID    PPID STAT COMMAND
    175     174 Z    zombie
```

`kill -9` прошёл без ошибки, но зомби на месте: процесса уже нет, убивать нечего. Осталась
только запись с кодом возврата, и забрать её может родитель через `wait`.

Дождался, пока родитель досчитает свои 120 секунд и выйдет:

```text
stepan@devbox:~/proc-lab$ [родитель] завершаюсь; зомби заберёт init
./zombie-finder.sh
PID      PPID     COMMAND
(зомби не найдено)
[1]+  Done                    ./zombie 120
```

Родитель умер, зомби усыновил PID 1 и сразу его убрал. Версия с `waitpid`:

```text
stepan@devbox:~/proc-lab$ ./zombie 5 wait
[родитель 198] ребёнок 199 завершился, ждём 5 с
смотрите: ps -o pid,ppid,stat,comm -p 199
[ребёнок 199] работаю и выхожу
[родитель] waitpid выполнен — записи о зомби больше нет
stepan@devbox:~/proc-lab$ ./zombie-finder.sh
PID      PPID     COMMAND
(зомби не найдено)
```

С `wait` программа не ждёт свои 5 секунд, а сразу забирает ребёнка и выходит: `sleep` в
`zombie.c` есть только в ветке без `wait`.

## Что понял

Удвоение вывода объясняется буфером stdio: он лежит в памяти, которую `fork` копирует.
Зомби исчезает только после `wait` в родителе (или после смерти родителя, тогда `wait`
делает PID 1). Сигналом его не убрать, потому что сигналы доставляются живым процессам.
