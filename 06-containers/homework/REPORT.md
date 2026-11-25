# Отчёт по ДЗ 6. Образ для своего приложения

Приложение — проект `hello` из ДЗ 4 (main, greet, stats). Для контейнера добавил две вещи:
слово приветствия из переменной `GREETING` и режим `--serve`, в котором программа ждёт
SIGTERM и выходит с кодом 0, а если задана `LOG_FILE`, пишет в неё строки о старте и
остановке. Docker Desktop 29.5.3, все три образа собраны `./check.sh --no-cache`.

Собрать и проверить: `./check.sh --no-cache`, потом `docker compose up` (остановка Ctrl+C) и
`docker compose down -v`. В Git Bash на Windows перед ручными командами нужен
`export MSYS_NO_PATHCONV=1`.

## 1. Dockerfile и размер

Основной `Dockerfile` (multistage):

```dockerfile
FROM alpine:3.20 AS builder

RUN apk add --no-cache build-base
WORKDIR /src
COPY app/ .
RUN make

FROM alpine:3.20

RUN mkdir /data && chown 65534:65534 /data

COPY --from=builder /src/hello /usr/local/bin/hello

USER 65534:65534

ENTRYPOINT ["/usr/local/bin/hello"]
CMD ["Docker"]
```

Размеры после сборки с нуля:

```text
$ docker images hello-hw
IMAGE              ID             DISK USAGE   CONTENT SIZE   EXTRA
hello-hw:multi     11dc307eeb7b       12.2MB         3.64MB
hello-hw:naive     2ba478f60099        406MB          108MB
hello-hw:scratch   bdbde6a8b8a0       93.4kB           32kB
```

| Вариант | Размер | Что внутри |
|---|---|---|
| naive | 406MB | Ubuntu 24.04, gcc, make, заголовки, исходники, `.o`, бинарник; root |
| multi | 12.2MB | Alpine 3.20 (musl, busybox), бинарник 19 КБ, пустой `/data`; uid 65534 |
| scratch | 93.4kB | только статический бинарник 54 КБ (`-static -s`); uid 65534 |

Финальный образ 12.2MB, меньше 20 МБ. Все три печатают одно и то же (`check.sh` запускает
каждый), например scratch:

```text
$ docker run --rm hello-hw:scratch
Hello, Docker!
длина 6: букв 6, цифр 0, прочих 0
greet() вызван 1 раз
$ docker run --rm -e GREETING=Привет hello-hw:scratch Степан
Привет, Степан!
длина 12: букв 0, цифр 0, прочих 12
greet() вызван 1 раз
```

Теги везде фиксированные (`alpine:3.20`, `ubuntu:24.04`). `.dockerignore` пропускает в
контекст только `app/`: без `.o`, бинарника, Dockerfile, отчётов и `.env`.

## 2. docker history: самый большой слой

```text
$ docker history hello-hw:naive
IMAGE          CREATED          CREATED BY                                      SIZE      COMMENT
2ba478f60099   2 minutes ago   CMD ["./hello" "Docker"]                        0B        buildkit.dockerfile.v0
<missing>      2 minutes ago   RUN /bin/sh -c make # buildkit                  45.1kB    buildkit.dockerfile.v0
<missing>      2 minutes ago   COPY app/ . # buildkit                          32.8kB    buildkit.dockerfile.v0
<missing>      2 minutes ago   WORKDIR /app                                    8.19kB    buildkit.dockerfile.v0
<missing>      2 minutes ago   RUN /bin/sh -c apt-get update     && apt-get…   210MB     buildkit.dockerfile.v0
<missing>      13 days ago     /bin/sh -c #(nop)  CMD ["/bin/bash"]            0B
<missing>      13 days ago     /bin/sh -c #(nop) ADD file:54dafd97985c3ec22…   87.7MB
...
```

Самый большой слой наивного образа — `apt-get install gcc make libc6-dev`, 210MB. Из них
внутри образа `/usr/libexec/gcc` 59M, `/usr/lib/gcc` 17M, `/usr/include` 13M. Второй по
весу — база Ubuntu, 87.7MB. Наш код (исходники и результат `make`) — 78kB.

```text
$ docker history hello-hw:multi
IMAGE          CREATED              CREATED BY                                      SIZE      COMMENT
11dc307eeb7b   About a minute ago   CMD ["Docker"]                                  0B        buildkit.dockerfile.v0
<missing>      About a minute ago   ENTRYPOINT ["/usr/local/bin/hello"]             0B        buildkit.dockerfile.v0
<missing>      About a minute ago   USER 65534:65534                                0B        buildkit.dockerfile.v0
<missing>      About a minute ago   COPY /src/hello /usr/local/bin/hello # build…   36.9kB    buildkit.dockerfile.v0
<missing>      About a minute ago   RUN /bin/sh -c mkdir /data && chown 65534:65…   8.19kB    buildkit.dockerfile.v0
<missing>      7 months ago         CMD ["/bin/sh"]                                 0B        buildkit.dockerfile.v0
<missing>      7 months ago         ADD alpine-minirootfs-3.20.10-x86_64.tar.gz …   8.47MB    buildkit.dockerfile.v0
$ docker history hello-hw:scratch
IMAGE          CREATED          CREATED BY                          SIZE      COMMENT
bdbde6a8b8a0   28 seconds ago   CMD ["Docker"]                      0B        buildkit.dockerfile.v0
<missing>      28 seconds ago   ENTRYPOINT ["/hello"]               0B        buildkit.dockerfile.v0
<missing>      28 seconds ago   USER 65534:65534                    0B        buildkit.dockerfile.v0
<missing>      28 seconds ago   COPY /src/hello /hello # buildkit   61.4kB    buildkit.dockerfile.v0
```

В multistage самый большой слой — база alpine, 8.47MB. В `scratch` слой один, сам
бинарник.

## 3. Что изменилось после multistage

- 406MB → 12.2MB, в 33 раза. Ушли компилятор и база Ubuntu: `apk add build-base` остался в
  этапе `builder`, а в финальный образ скопирован один файл.
- В образе нет исходников и объектников, только `/usr/local/bin/hello`. Поправить что-то
  «на месте» нельзя, только пересобрать.
- Программа работает от `nobody` (`USER 65534:65534`), в наивном варианте от root.
- Бинарник собран под musl, а не glibc, поэтому этапы должны быть на одной базе: alpine
  собирает, alpine запускает.
- `scratch` урезает ещё в 130 раз, до 93.4kB. Для этого бинарник статический
  (`make static`: `-static -s`), иначе ему не хватит `/lib/ld-musl-x86_64.so.1`.

### SIGTERM и docker stop

В режиме `--serve` программа ставит `sigaction` на SIGTERM и SIGINT и спит в `sigsuspend`.
`ENTRYPOINT` в exec-форме, так что PID 1 — сама программа:

```text
$ docker run -d --name hw-serve hello-hw:multi --serve Stepan
$ docker exec hw-serve ps aux
PID   USER     TIME  COMMAND
    1 nobody    0:00 /usr/local/bin/hello --serve Stepan
    7 nobody    0:00 ps aux
$ time docker stop -t 10 hw-serve
hw-serve

real	0m0.495s
...
$ docker logs hw-serve | tail -1
получен SIGTERM, работал 35 с, выхожу
$ docker inspect hw-serve --format 'exit={{.State.ExitCode}}'
exit=0
```

Для сравнения та же программа, но запущенная через оболочку, как было бы с shell-формой
`CMD`:

```text
$ docker run -d --name hw-shell --entrypoint sh hello-hw:multi \
    -c '/usr/local/bin/hello --serve Stepan; echo "hello вышел"'
$ docker exec hw-shell ps aux
PID   USER     TIME  COMMAND
    1 nobody    0:00 sh -c /usr/local/bin/hello --serve Stepan; echo "hello вышел"
    7 nobody    0:00 /usr/local/bin/hello --serve Stepan
    8 nobody    0:00 ps aux
$ time docker stop -t 10 hw-shell
hw-shell

real	0m10.366s
...
$ docker logs hw-shell | tail -2
greet() вызван 1 раз
жду SIGTERM
$ docker inspect hw-shell --format 'exit={{.State.ExitCode}}'
exit=137
```

0.5 с и код 0 против 10.4 с и SIGKILL: SIGTERM получил `sh`, а он как PID 1 без обработчика
сигнал проигнорировал. Строки «получен SIGTERM» в логе нет. `scratch` ведёт себя так же,
как multi: `check.sh` показал меньше секунды и код 0 для обоих.

## 4. Вывод compose up

`compose.yml`: два сервиса из одного образа `hello-hw:multi` (собирается один раз, у
`hello-en` только `image`), разные `GREETING` и общий именованный том `visits` в `/data`.

```text
$ docker compose up
 Network hello-compose_default Creating
 Network hello-compose_default Created
 Volume hello-compose_visits Creating
 Volume hello-compose_visits Created
 Container hello-compose-hello-ru-1 Creating
 Container hello-compose-hello-ru-1 Created
 Container hello-compose-hello-en-1 Creating
 Container hello-compose-hello-en-1 Created
Attaching to hello-en-1, hello-ru-1
 Container hello-compose-hello-ru-1 Starting
 Container hello-compose-hello-ru-1 Started
 Container hello-compose-hello-en-1 Starting
hello-ru-1  | Привет, Степан!
hello-ru-1  | длина 12: букв 0, цифр 0, прочих 12
hello-ru-1  | greet() вызван 1 раз
hello-ru-1  | жду SIGTERM
hello-en-1  | Hello, Stepan!
hello-en-1  | длина 6: букв 6, цифр 0, прочих 0
hello-en-1  | greet() вызван 1 раз
hello-en-1  | жду SIGTERM
 Container hello-compose-hello-en-1 Started
hello-en-1  | получен SIGTERM, работал 40 с, выхожу
hello-en-1 exited with code 0
hello-ru-1  | получен SIGTERM, работал 41 с, выхожу
hello-ru-1 exited with code 0
```

Последние четыре строки появились, когда я из второго терминала остановил сервисы:

```text
$ docker compose ps --format 'table {{.Name}}\t{{.Image}}\t{{.Status}}'
NAME                       IMAGE            STATUS
hello-compose-hello-en-1   hello-hw:multi   Up 29 seconds
hello-compose-hello-ru-1   hello-hw:multi   Up 29 seconds
$ time docker compose stop
 Container hello-compose-hello-en-1 Stopping
 Container hello-compose-hello-en-1 Stopped
 Container hello-compose-hello-ru-1 Stopping
 Container hello-compose-hello-ru-1 Stopped

real	0m1.061s
```

Том пережил `docker compose down` (без `-v`) и второй запуск, журнал от обоих сервисов
лежит в нём:

```text
$ docker compose down
...
$ docker volume ls --filter name=hello-compose
DRIVER    VOLUME NAME
local     hello-compose_visits
$ docker compose up -d
...
$ docker compose down
...
$ docker run --rm -v hello-compose_visits:/data alpine:3.20 cat /data/visits.log
2026-11-23 20:10:23 UTC Степан start
2026-11-23 20:10:23 UTC Stepan start
2026-11-23 20:11:03 UTC Stepan stop
2026-11-23 20:11:04 UTC Степан stop
2026-11-23 20:11:53 UTC Степан start
2026-11-23 20:11:53 UTC Stepan start
2026-11-23 20:12:16 UTC Stepan stop
2026-11-23 20:12:17 UTC Степан stop
$ docker compose down -v
...
 Volume hello-compose_visits Removed
```

## 5. Что было непонятно

1. Первый запуск `--serve` в фоне: `docker logs` показывал только `Hello, Stepan!`, а
   остальные строки появились лишь после остановки. stdout в контейнере не терминал, и
   вывод копится в буфере. Добавил `setvbuf(stdout, NULL, _IOLBF, 0)` в начало `main`.
2. В первом `compose up` оба сервиса писали `/data/visits.log: Permission denied`. Пустой
   том создаётся с владельцем root, а процесс работает от `nobody`. Помогло создать `/data`
   в образе с владельцем 65534: Docker при подключении пустого тома копирует владельца
   каталога. После этого сработало и на старом томе, `down -v` не понадобился.
3. Положил для проверки `app/.env`, а в `.dockerignore` было просто `.env`. Файл доехал до
   образа: шаблон без `**/` закрывает только корень контекста. Исправил на `**/.env`.
4. `docker stop` у контейнера со `sleep` занял 1.5 с, а не 10, как на слайде. В Docker
   Desktop у моих контейнеров `StopTimeout` равен 1 (видно в `docker inspect`), поэтому все
   замеры делал с явным `docker stop -t 10`. Откуда берётся эта единица, так и не нашёл.
