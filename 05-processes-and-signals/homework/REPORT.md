# Отчёт по ДЗ 5: minishell

Степан, курс «Программирование в Linux», СПбГУ, 2026. Всё собирал и запускал в контейнере с
Ubuntu 24.04 (gcc 13.3.0, GNU Make 4.3) под обычным пользователем. Исходник один,
`minishell.c`, около 400 строк, C11. Сборка:

```text
stepan@devbox:~/proc-hw$ make clean && make && make asan
rm -f minishell minishell-asan
rm -rf dist-tmp
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -o minishell minishell.c
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all -o minishell-asan minishell.c
```

С `-Werror` собирается без единого предупреждения. `minishell-asan` — та же программа с
AddressSanitizer и UBSan, на ней гонял тесты и часть ручной сессии.

## 1. Как устроен разбор строки

Строку читает `getline()` в `main`. Дальше два уровня, оба в `minishell.c`:

1. `parse_line()` ищет `|` через `strchr`, ставит на его место `'\0'` и отдаёт каждый кусок
   в `parse_command()`. Получается массив `struct command cmds[8]`. Пустая строка даёт 0
   команд, пустой кусок (`ls |`) — ошибка «пустая команда».
2. `parse_command()` режет кусок на слова через `strtok_r` по пробелам и табуляции.
   Слово `>` (или `>файл` слитно) не попадает в `argv`, следующее за ним слово становится
   `outfile`. В конце `argv` стоит `NULL`, как ждёт `execvp`.

Строку я никуда не копирую: `argv` указывают прямо в буфер `getline`, разделители
заменены на `'\0'`. Буфер живёт до следующего `getline`, а к этому моменту дети уже
отработали. Поэтому в разборе нет ни одного `malloc`, и освобождать надо только сам буфер
строки в конце `main`.

Всё, чего оболочка не умеет, она отвергает с сообщением и кодом 2, а не угадывает: слово с
`<` или `>` не в начале (`ls>out`, `2>err`), `>>`, второе `>` в одной команде, `>` после
`>`. Ограничения: до 8 команд в конвейере и до 64 слов в команде, включая её имя.
Кавычек, переменных и `~` нет, про `~` ниже в разделе 5.

Встроенные `cd` и `exit` проверяет `run_builtin()`, и только если команда одна, без `|`.
В конвейере они ничего бы не сделали (попали бы в ребёнка), поэтому там
`builtin_in_pipeline()` пишет «в конвейере не поддерживается». `>` у встроенных тоже
отвергается. `cd` без аргумента идёт в `$HOME`. `exit N` выходит с кодом N; `exit abc` —
сообщение и выход с кодом 2; `exit 1 2` — сообщение «слишком много аргументов», оболочка
остаётся (так делает bash).

Если stdin не терминал, `main` выключает буфер stdin (`setvbuf(stdin, NULL, _IONBF, 0)`),
и `getline` читает по байту. Иначе в `printf 'cat\nhello\n' | ./minishell` getline забрал
бы в буфер и `hello`, и `cat` получил бы пустой ввод.

## 2. Где создаются процессы

Процессы создаёт только `run_pipeline()`: на каждую команду один `fork()`. В ребёнке
сразу вызывается `exec_child()`, который настраивает дескрипторы и делает `execvp`. Если
`execvp` вернулся, команды нет: сообщение в stderr и `_exit(127)`, как в bash. Именно
`_exit`, а не `exit`, чтобы ребёнок не сбросил второй раз скопированные буферы stdio.
Перед циклом стоит `fflush(stdout)` по той же причине.

`cd` и `exit` выполняются без `fork`, в самой оболочке. `chdir` в ребёнке поменял бы
каталог только ребёнку, и после его смерти всё вернулось бы назад.

Родитель ждёт всех детей по очереди в `wait_child()`, а не только последнего. `waitpid`
перезапускается, если его прервал `SIGINT` (`EINTR`).

Сигналы. Обработчик `SIGINT` ставится через `sigaction` без `SA_RESTART` и только
выставляет флаг. Без `SA_RESTART` `read` внутри `getline` прерывается с `EINTR`, `main`
видит это, сбрасывает ошибку потока (`clearerr`) и печатает новое приглашение. На время
`fork` `SIGINT` блокируется через `sigprocmask`. Ребёнок сначала ставит `SIG_DFL` и только
потом снимает блокировку, поэтому Ctrl+C, пришедший в любой момент между `fork` и `exec`,
убивает команду, а не запускает в ребёнке обработчик оболочки. Родитель снимает блокировку
сразу после `fork`. В интерактивном режиме оболочка игнорирует `SIGQUIT` (Ctrl+\), а в
ребёнке возвращает ему `SIG_DFL`: `SIG_IGN`, в отличие от обработчика, переживает `exec`.

### Сессия

Первый терминал:

```text
stepan@devbox:~/proc-hw$ ./minishell
minishell:/home/stepan/proc-hw$ ls /etc | wc -l
112
minishell:/home/stepan/proc-hw$ cd /etc
minishell:/etc$ ls | wc -l
112
minishell:/etc$ cd
minishell:/home/stepan$ pwd
/home/stepan
minishell:/home/stepan$ ls -1 /usr > dirs.txt
minishell:/home/stepan$ cat dirs.txt
bin
games
include
lib
lib64
libexec
local
sbin
share
src
minishell:/home/stepan$ cat dirs.txt | wc -l
10
minishell:/home/stepan$ sleep 30
^C
minishell:/home/stepan$ sleep 30 | wc -l
^C
minishell:/home/stepan$ ls /e^C
minishell:/home/stepan$ 
minishell:/home/stepan$ ^\ps -o pid,ppid,stat,comm
    PID    PPID STAT COMMAND
    410     409 S    bash
    443     410 S+   minishell
    468     443 R+   ps
minishell:/home/stepan$ sh ~/proc-lab/zombie-finder.sh
sh: 0: cannot open ~/proc-lab/zombie-finder.sh: No such file
minishell:/home/stepan$ sh /home/stepan/proc-lab/zombie-finder.sh
PID      PPID     COMMAND
(зомби не найдено)
minishell:/home/stepan$ cd /nope
cd: /nope: No such file or directory
minishell:/home/stepan$ nosuchcmd
minishell: nosuchcmd: No such file or directory
minishell:/home/stepan$ ls / | grep bin
bin
sbin
minishell:/home/stepan$ ls>out
minishell: ls>out: поддерживается только "> файл"
minishell:/home/stepan$ ls / 2>err
minishell: 2>err: поддерживается только "> файл"
minishell:/home/stepan$ cd / | cat
minishell: cd: в конвейере не поддерживается
minishell:/home/stepan$ exit 1 2
minishell: exit: слишком много аргументов
minishell:/home/stepan$ exit
stepan@devbox:~/proc-hw$ echo $?
1
stepan@devbox:~/proc-hw$ ls /etc | wc -l
112
```

`ls /etc | wc -l` в minishell и в bash дают одно и то же, 112.

Ctrl+C я нажимал три раза. Во время `sleep 30` и `sleep 30 | wc -l` он убил команды, а
оболочка напечатала новое приглашение. Посреди набранного `ls /e` строка сбросилась. Потом,
пока оболочка ждала ввода, я послал ей `SIGINT` из второго терминала. Поэтому после
`ls /e^C` стоит пустое приглашение без `^C`: терминал ничего не напечатал, оболочка просто
начала новую строку. `^\` перед `ps` — это Ctrl+\: оболочка его проигнорировала, а
терминал при этом сбросил набранное, и я напечатал `ps` заново. Второй терминал, по порядку:

```text
stepan@devbox:~$ ps -o pid,ppid,stat,comm --ppid $(pgrep -x minishell)
    PID    PPID STAT COMMAND
    453     443 S+   sleep
stepan@devbox:~$ ps -o pid,ppid,stat,comm --ppid $(pgrep -x minishell)
    PID    PPID STAT COMMAND
    457     443 S+   sleep
    458     443 S+   wc
stepan@devbox:~$ kill -INT $(pgrep -x minishell)
```

Пока шёл `sleep 30`, у оболочки был один ребёнок, во время `sleep 30 | wc -l` — два. В `ps` изнутри видно, что детей, кроме самого `ps`, у неё нет.

Что `kill -INT` оболочку не убивает, проверил ещё одним запуском, `ps` до и после сигнала:

```text
$ ps -o pid,stat,comm -p "$(pgrep -x minishell)"
    PID STAT COMMAND
     42 Ss+  minishell
$ kill -INT "$(pgrep -x minishell)"
$ ps -o pid,stat,comm -p "$(pgrep -x minishell)"
    PID STAT COMMAND
     42 Ss+  minishell
```

Ошибки и отказы ведут себя как надо: `ls>out`, `2>err` и `cd / | cat` отвергнуты,
`exit 1 2` не вышел. Код выхода 1 после `exit` — это код последней команды, того самого
`exit 1 2`.

Та же проверка Ctrl+C на `minishell-asan`:

```text
stepan@devbox:~/proc-hw$ ./minishell-asan
minishell:/home/stepan/proc-hw$ ls /etc | sort | head -3
adduser.conf
alternatives
apparmor.d
minishell:/home/stepan/proc-hw$ sleep 20
^C
minishell:/home/stepan/proc-hw$ exit 5
stepan@devbox:~/proc-hw$ echo $?
5
```

ASan не напечатал ничего, утечек при выходе тоже нет.

### Тесты

`test.sh` гоняет оболочку без терминала, подавая команды в stdin, и сравнивает с bash.
Ctrl+C там имитируется `kill -INT` всей группе процессов оболочки, как это делает терминал.

```text
stepan@devbox:~/proc-hw$ make test
./test.sh ./minishell
ok    ls | wc -l
ok    ls | sort | head -3
ok    > файл
ok    cd и pwd
ok    cd без аргумента
ok    exit 3
ok    нет команды: код 127
ok    exit abc: код 2
ok    exit 1 2: не выходит
ok    отказ: ls>out
ok    отказ: ls 2>err
ok    отказ: ls >> out
ok    отказ: cat < in
ok    отказ: cd / | cat
ok    отказ: cd > out
ok    после отказов файлов нет
ok    cat читает остаток stdin
ok    зомби во время конвейера
ok    оболочка жива после SIGINT
ok    после SIGINT команды выполняются
ok    выход по EOF с кодом последней команды
ok    зомби после выхода
все проверки прошли
./test.sh ./minishell-asan
...
ok    зомби после выхода
все проверки прошли
```

Чтобы убедиться, что тесты вообще что-то ловят, ломал копию `minishell.c` через `sed`.
Ждать только последнего ребёнка: падает «зомби во время конвейера» (`ждал [0], получил [1]`).
Не ставить обработчик `SIGINT`: падают «оболочка жива после SIGINT» и две проверки после
неё. Не закрыть в родителе пишущий конец канала: `ls /etc | wc -l` висит, пока его не
снимет `timeout 3` (код 124).

## 3. Где закрываются дескрипторы

Все закрытия в `run_pipeline()` и `exec_child()`. Правило одно: после `fork` у каждого
процесса должен остаться только тот конец канала, которым он пользуется, и только как 0
или 1. В ребёнке `dup2` и закрытие старого номера делает `move_fd()`; если `dup2` не
удался, ребёнок выходит с кодом 126, а не запускает команду с чужими дескрипторами.

| Где | Что закрываю | Почему |
|---|---|---|
| ребёнок, после `dup2(in_fd, 0)` | `in_fd` | читаю уже через 0, копия не нужна |
| ребёнок, после `dup2(pipe_fd[1], 1)` | `pipe_fd[1]` | пишу через 1 |
| ребёнок, там же | `pipe_fd[0]` | читать из своего канала будет следующая команда; лишний читатель не даст писателю получить `SIGPIPE` |
| ребёнок, после `dup2(fd, 1)` для `> файл` | `fd` файла | пишу через 1 |
| родитель, после каждого `fork` | `pipe_fd[1]` | главное место: пока родитель держит пишущий конец, следующая команда не получит EOF и повиснет в `read` |
| родитель, после каждого `fork` | `in_fd` (читающий конец прошлого канала) | он уже передан ребёнку; иначе дескрипторы копились бы с каждой командой |
| родитель, после цикла | `in_fd`, если остался | бывает, только если `pipe` или `fork` упали посреди конвейера |

Родитель закрывает концы сразу после каждого `fork`, а не в конце. Поэтому следующий
ребёнок наследует только читающий конец прошлого канала и свой новый канал, а не все
каналы конвейера.

Проверял через `strace` на `ls /etc | wc -l`. 700 — minishell, 701 — `ls`, 702 — `wc`:

```text
stepan@devbox:~/proc-hw$ echo 'ls /etc | wc -l' | strace -f -e trace=pipe2,dup2,close,clone,execve,wait4 -e signal=none -o trace.txt ./minishell
112
stepan@devbox:~/proc-hw$ grep -v ENOENT trace.txt | head -26
700   execve("./minishell", ["./minishell"], 0x7fff1944fc58 /* 15 vars */) = 0
700   close(3)                          = 0
700   close(3)                          = 0
700   pipe2([3, 4], 0)                  = 0
700   clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7240ac141a10) = 701
700   close(4)                          = 0
700   clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD <unfinished ...>
701   dup2(4, 1)                        = 1
700   <... clone resumed>, child_tidptr=0x7240ac141a10) = 702
701   close(4 <unfinished ...>
700   close(3 <unfinished ...>
701   <... close resumed>)              = 0
700   <... close resumed>)              = 0
700   wait4(701,  <unfinished ...>
701   close(3)                          = 0
702   dup2(3, 0 <unfinished ...>
701   execve("/usr/local/bin/ls", ["ls", "/etc"], 0x7ffce2957798 /* 15 vars */ <unfinished ...>
702   <... dup2 resumed>)               = 0
702   close(3 <unfinished ...>
701   execve("/usr/sbin/ls", ["ls", "/etc"], 0x7ffce2957798 /* 15 vars */ <unfinished ...>
702   <... close resumed>)              = 0
702   execve("/usr/local/sbin/wc", ["wc", "-l"], 0x7ffce2957798 /* 15 vars */ <unfinished ...>
701   execve("/usr/bin/ls", ["ls", "/etc"], 0x7ffce2957798 /* 15 vars */ <unfinished ...>
702   execve("/usr/bin/wc", ["wc", "-l"], 0x7ffce2957798 /* 15 vars */ <unfinished ...>
701   <... execve resumed>)             = 0
702   <... execve resumed>)             = 0
stepan@devbox:~/proc-hw$ grep -v ENOENT trace.txt | tail -12
701   close(1)                          = 0
701   close(2)                          = 0
702   close(0)                          = 0
702   close(1)                          = 0
702   close(2 <unfinished ...>
701   +++ exited with 0 +++
700   <... wait4 resumed>[{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 701
702   <... close resumed>)              = 0
700   wait4(702,  <unfinished ...>
702   +++ exited with 0 +++
700   <... wait4 resumed>[{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 702
700   +++ exited with 0 +++
```

Первые два `close(3)` у 700 делает загрузчик библиотек, к моему коду они не относятся.
Дальше всё по таблице: `pipe2` дал 3 и 4, после первого `fork` родитель закрыл 4, после
второго 3. `ls` сделал `dup2(4, 1)` и закрыл 4 и 3, `wc` сделал `dup2(3, 0)` и закрыл 3.
Только после этого `execve`. Родитель ждёт сначала 701, потом 702.

## 4. Вывод zombie-finder.sh

`zombie-finder.sh` из Chapter 05 лежит у меня в `~/proc-lab`. До запуска minishell:

```text
stepan@devbox:~/proc-hw$ sh ~/proc-lab/zombie-finder.sh
PID      PPID     COMMAND
(зомби не найдено)
```

Во время работы, изнутри minishell, после нескольких конвейеров и трёх Ctrl+C (первая
попытка с `~` не сработала, см. раздел 5):

```text
minishell:/home/stepan$ sh ~/proc-lab/zombie-finder.sh
sh: 0: cannot open ~/proc-lab/zombie-finder.sh: No such file
minishell:/home/stepan$ sh /home/stepan/proc-lab/zombie-finder.sh
PID      PPID     COMMAND
(зомби не найдено)
```

После выхода из minishell:

```text
stepan@devbox:~/proc-hw$ sh ~/proc-lab/zombie-finder.sh
PID      PPID     COMMAND
(зомби не найдено)
```

И в самом конце, после `minishell-asan`, тестов и `strace`:

```text
stepan@devbox:~/proc-hw$ sh ~/proc-lab/zombie-finder.sh
PID      PPID     COMMAND
(зомби не найдено)
```

Что скрипт вообще умеет находить зомби, проверял на практике 2 с `./zombie 120 &`.

## 5. Что было непонятно

1. `sh ~/proc-lab/zombie-finder.sh` внутри minishell не нашёл файл. Оказалось, `~`
   раскрывает сама оболочка до запуска команды, а я передаю строку в `execvp` как есть, и
   `sh` получил аргумент с буквальной тильдой. Раскрытие `~` делать не стал, просто писал
   полный путь.
2. Первый вариант `test.sh` запускал minishell в фоне через `setsid ... &`, и проверка
   «оболочка жива после SIGINT» проходила даже у копии вообще без обработчика. Не сразу
   понял почему: bash без управления заданиями ставит фоновым командам `SIGINT` в
   `SIG_IGN`, и они его просто не видят. С `set -m` вокруг запуска сломанная копия стала
   падать, как и должна.
3. В первой версии на Ctrl+C во время `sleep 30 | wc -l` оболочка печатала лишнюю пустую
   строку. Перевод строки стоял в `wait_child()`, а `SIGINT` получают все в группе, то
   есть оба ребёнка. Перенёс печать в `run_pipeline()`, один раз на конвейер.
4. Сначала в ребёнке стоял только `signal(SIGINT, SIG_DFL)` перед `exec`, и я думал, что
   этого хватает. Но `SIGINT`, пришедший между `fork` и этой строкой, всё равно запустил бы
   в ребёнке обработчик оболочки, и команда бы не умерла: строка окно сужала, а не
   закрывала. Помогла блокировка `SIGINT` через `sigprocmask` на время `fork`, порядок
   описан в разделе 2.
