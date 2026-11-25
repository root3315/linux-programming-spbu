# Практика 1. Первый контейнер

Ничего не собирал, только запускал готовые образы. Docker Desktop 29.5.3 на Windows, команды
из Git Bash. Своих файлов в этой практике нет.

## Шаг 1. hello-world

```text
$ docker run --rm hello-world

Hello from Docker!
This message shows that your installation appears to be working correctly.

To generate this message, Docker took the following steps:
 1. The Docker client contacted the Docker daemon.
 2. The Docker daemon pulled the "hello-world" image from the Docker Hub.
    (amd64)
 3. The Docker daemon created a new container from that image which runs the
    executable that produces the output you are currently reading.
 4. The Docker daemon streamed that output to the Docker client, which sent it
    to your terminal.
...
```

Что произошло, программа сама и рассказала: клиент `docker` попросил демон, демон скачал
образ, создал из него контейнер, запустил единственный бинарник, и его stdout пришёл мне в
терминал. Бинарник отработал, контейнер завершился, `--rm` его удалил.

## Шаги 2–3. Внутри alpine

Команды внутри контейнера:

```text
$ docker run --rm -i alpine:3.20 sh
cat /etc/os-release
ps aux
ls /
hostname
uname -r
```

Вывод:

```text
NAME="Alpine Linux"
ID=alpine
VERSION_ID=3.20.10
PRETTY_NAME="Alpine Linux v3.20"
HOME_URL="https://alpinelinux.org/"
BUG_REPORT_URL="https://gitlab.alpinelinux.org/alpine/aports/-/issues"
PID   USER     TIME  COMMAND
    1 root      0:00 sh
    8 root      0:00 ps aux
bin
dev
etc
home
lib
media
mnt
opt
proc
root
run
sbin
srv
sys
tmp
usr
var
2527c8bb60ee
6.18.58.1-microsoft-standard-WSL2
```

Процессов два: `sh` с PID 1 и сам `ps`. Имя узла — короткий ID контейнера. Корень выглядит
как обычный Linux, но это файлы образа alpine, а не хоста.

Сравнил с «хостом». У меня хост для контейнеров — это виртуалка WSL2, которую поднимает
Docker Desktop. Её процессы можно увидеть, если не отделять PID namespace (`--pid=host`):

```text
$ docker run --rm alpine:3.20 sh -c 'ps aux | wc -l'
4
$ docker run --rm --pid=host alpine:3.20 sh -c 'ps aux | wc -l; ps aux | head -5'
68
PID   USER     TIME  COMMAND
    1 root      0:15 /initd
   19 root     15:20 /initd services
   39 root      0:00 /bin/sh /usr/bin/rungetty.sh
   40 root      0:00 /bin/login -f
```

4 строки (заголовок, `sh`, `ps`, `wc`) против 68, и PID 1 там уже `/initd`, а не моя
команда. Ядро одно, проверил на другом образе:

```text
$ docker run --rm ubuntu:24.04 uname -r
6.18.58.1-microsoft-standard-WSL2
```

## Шаг 4. Куда делся контейнер

```text
$ echo 'cat /etc/hostname' | docker run --rm -i alpine:3.20 sh
d2c7f2a3f1fd
$ docker ps -a --filter ancestor=alpine:3.20 --format '{{.Names}} {{.Status}}'
```

Вывод `docker ps -a` пустой: после `exit` (у меня просто конец ввода) `sh` с PID 1
завершился, контейнер остановился, а `--rm` его сразу удалил. Для контраста тот же запуск
без `--rm`:

```text
$ docker run -i alpine:3.20 true
$ docker ps -a --filter ancestor=alpine:3.20 --format '{{.Names}} {{.Status}}'
funny_ishizaka Exited (0) Less than a second ago
$ docker rm funny_ishizaka
funny_ishizaka
```

Без `--rm` остановленный контейнер остаётся в списке со своим записываемым слоем, пока его
не удалят руками.

## Шаги 5–6. Контейнер в фоне

```text
$ docker run -d --name demo alpine:3.20 sleep 300
1308f73b4475d90b2aa9d16bd0677f1c6d18a9cc6aeedff84a2e22e68e9ece2a
$ docker ps --filter name=demo --format 'table {{.Names}}\t{{.Image}}\t{{.Command}}\t{{.Status}}'
NAMES     IMAGE         COMMAND       STATUS
demo      alpine:3.20   "sleep 300"   Up 20 seconds
$ docker exec demo ps aux
PID   USER     TIME  COMMAND
    1 root      0:00 sleep 300
    7 root      0:00 ps aux
$ docker exec demo hostname
1308f73b4475
```

PID 1 — `sleep`, а `ps` подселился в тот же PID namespace через `exec`. Посмотрел
namespaces и cgroups:

```text
$ docker exec demo ls -l /proc/1/ns
total 0
lrwxrwxrwx    1 root     root             0 Nov 23 19:40 cgroup -> cgroup:[4026533009]
lrwxrwxrwx    1 root     root             0 Nov 23 19:40 ipc -> ipc:[4026533007]
lrwxrwxrwx    1 root     root             0 Nov 23 19:40 mnt -> mnt:[4026533005]
lrwxrwxrwx    1 root     root             0 Nov 23 19:40 net -> net:[4026533010]
lrwxrwxrwx    1 root     root             0 Nov 23 19:40 pid -> pid:[4026533008]
lrwxrwxrwx    1 root     root             0 Nov 23 19:40 pid_for_children -> pid:[4026533008]
lrwxrwxrwx    1 root     root             0 Nov 23 19:40 time -> time:[4026533079]
lrwxrwxrwx    1 root     root             0 Nov 23 19:40 time_for_children -> time:[4026533079]
lrwxrwxrwx    1 root     root             0 Nov 23 19:40 user -> user:[4026531837]
lrwxrwxrwx    1 root     root             0 Nov 23 19:40 uts -> uts:[4026533006]
$ docker exec demo cat /proc/1/cgroup
0::/
$ docker exec demo sh -c 'cat /sys/fs/cgroup/memory.max /sys/fs/cgroup/pids.max'
max
max
$ docker run --rm --memory 64m --pids-limit 50 alpine:3.20 \
    sh -c 'cat /sys/fs/cgroup/memory.max /sys/fs/cgroup/pids.max'
67108864
50
```

Свои `pid`, `net`, `mnt`, `uts`, `ipc`, `cgroup`. У `user` номер маленький, похоже, общий с
хостом: user namespace Docker по умолчанию не включает. Внутри своя cgroup видна как `/`,
лимиты без флагов `max`, с `--memory 64m` ровно 64·1024·1024.

```text
$ docker logs demo
$ time docker stop demo
demo

real	0m1.578s
...
$ docker ps -a --filter name=demo --format '{{.Names}} {{.Status}}'
demo Exited (137) Less than a second ago
```

`docker logs` пустой: `sleep` ничего не печатает. А `docker stop` удивил: на слайде
сказано, что ждать 10 секунд, а тут 1.5 с, и код 137 (128 + 9, SIGKILL). Посмотрел:

```text
$ docker inspect demo --format '{{.Config.StopTimeout}} {{.Config.StopSignal}} {{.State.ExitCode}}'
1  137
$ docker rm demo
demo
```

В моём Docker Desktop у контейнеров таймаут остановки 1 с. С явным `-t 10` всё как на
слайде:

```text
$ docker run -d --name demo2 alpine:3.20 sleep 300
$ time docker stop -t 10 demo2
demo2

real	0m10.494s
...
$ docker inspect demo2 --format '{{.State.ExitCode}}'
137
$ docker rm demo2
```

И с `--init`:

```text
$ docker run --rm --init alpine:3.20 ps aux
PID   USER     TIME  COMMAND
    1 root      0:00 /sbin/docker-init -- ps aux
    7 root      0:00 ps aux
$ docker run -d --init --name initdemo alpine:3.20 sleep 300
$ time docker stop -t 10 initdemo
initdemo

real	0m0.334s
...
$ docker inspect initdemo --format '{{.State.ExitCode}}'
143
```

С `docker-init` в роли PID 1 сигнал дошёл до `sleep`, и тот умер от SIGTERM (143 = 128 + 15)
сразу.

## Что понял

`ps` внутри показывает два процесса, потому что у контейнера свой PID namespace: видны
только процессы, запущенные в нём, и первый из них получает PID 1. Ядро не применяет к PID 1
действие сигнала по умолчанию, поэтому `sleep` без обработчика игнорирует SIGTERM, и
`docker stop` ждёт таймаут до SIGKILL.
