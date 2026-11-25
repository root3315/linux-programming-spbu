# Практика 3. Multistage и scratch

`Dockerfile` и `Dockerfile.scratch` из Chapter 06 собирал как есть, в `~/docker-lab`. Свой
файл тут один: [Dockerfile.dyn](Dockerfile.dyn), им проверял, зачем `scratch` нужен
`-static`.

## Шаг 1. Multistage

```text
$ time docker build --progress=plain -t hello:multi .
...
#5 [builder 1/5] FROM docker.io/library/alpine:3.20@sha256:d9e853e87e55526f6b2917df91a2115c36dd7c696a35be12163d44e6e2a4b6bc
#5 DONE 0.1s
#6 [stage-1 2/4] WORKDIR /app
#6 DONE 0.0s
#7 [builder 2/5] RUN apk add --no-cache build-base
#7 0.256 fetch https://dl-cdn.alpinelinux.org/alpine/v3.20/main/x86_64/APKINDEX.tar.gz
#7 1.412 fetch https://dl-cdn.alpinelinux.org/alpine/v3.20/community/x86_64/APKINDEX.tar.gz
...
#7 13.74 OK: 224 MiB in 35 packages
#7 DONE 13.9s
#8 [builder 3/5] WORKDIR /src
#8 DONE 0.1s
#9 [builder 4/5] COPY app/ .
#9 DONE 0.1s
#10 [builder 5/5] RUN make
#10 0.374 gcc -std=c11 -Wall -Wextra -O2 -o hello hello.c
#10 DONE 0.5s
#11 [stage-1 3/4] COPY --from=builder /src/hello /app/hello
#11 DONE 0.0s
#12 [stage-1 4/4] COPY app/data.txt /data.txt
#12 DONE 0.0s
...
real	0m15.752s
```

Этапы `builder` и `stage-1` (второй `FROM` без имени) идут параллельно: `WORKDIR /app`
второго этапа сделался ещё до `apk add`. Ждёт он только на `COPY --from=builder`.

## Шаг 2. Сравнение и запуск

```text
$ docker images hello
IMAGE         ID             DISK USAGE   CONTENT SIZE   EXTRA
hello:multi   aa641245ffca       12.2MB         3.64MB
hello:naive   d0523ba88acd        406MB          108MB
$ docker run --rm hello:multi
Hello, world!
данные из образа: этот файл попал сюда через COPY
второй строкой — чтобы видеть, что прочитано всё
$ docker run --rm hello:multi id
uid=65534(nobody) gid=65534(nobody) groups=65534(nobody)
```

406MB против 12.2MB, вывод тот же. Работает от `nobody`, это `USER 65534:65534`.

Попробовал передать свои аргументы, как в README из архива, и споткнулся:

```text
$ docker run --rm -e GREETING="Привет" hello:multi /app/hello Степан /data.txt
docker: Error response from daemon: failed to create task for container: failed to create shim task: OCI runtime create failed: runc create failed: unable to start container process: error during container init: exec: "C:/Program Files/Git/app/hello": stat C:/Program Files/Git/app/hello: no such file or directory
```

Это не Docker, а Git Bash на Windows: он видит аргумент, похожий на путь `/app/hello`, и
переписывает его в виндовый. Отключается переменной `MSYS_NO_PATHCONV=1`:

```text
$ export MSYS_NO_PATHCONV=1
$ docker run --rm -e GREETING="Привет" hello:multi /app/hello Степан /data.txt
Привет, Степан!
данные из образа: этот файл попал сюда через COPY
второй строкой — чтобы видеть, что прочитано всё
```

Здесь `ENTRYPOINT` нет, поэтому аргументы целиком заменили `CMD`, и путь к программе
пришлось писать самому.

Сколько весит выброшенный этап, собрал его отдельно через `--target`:

```text
$ docker build -q --target builder -t hello:builder .
sha256:5cfbad22fd9fe0a86e9749df8add7431db12fc1ee9728be4f9cc7401847f99ea
$ docker run --rm --entrypoint ls hello:builder -l /src
total 32
-rwxr-xr-x    1 root     root           306 Nov 23 19:41 Makefile
-rwxr-xr-x    1 root     root           176 Nov 23 19:47 data.txt
-rwxr-xr-x    1 root     root         18760 Nov 23 19:48 hello
-rwxr-xr-x    1 root     root          1197 Nov 23 19:41 hello.c
```

`builder` весит 318MB (в таблице ниже), а в финальный образ из него уехал один файл
`hello` на 18.7kB.

## Шаг 3. scratch

```text
$ docker build --progress=plain -f Dockerfile.scratch -t hello:scratch .
...
#6 [builder 2/5] RUN apk add --no-cache build-base
#6 CACHED
#7 [builder 3/5] WORKDIR /src
#7 CACHED
#8 [builder 4/5] COPY app/ .
#8 CACHED
#9 [builder 5/5] RUN gcc -std=c11 -Wall -Wextra -O2 -static -o hello hello.c
#9 DONE 0.3s
#10 [stage-1 1/2] COPY --from=builder /src/hello /hello
#10 DONE 0.0s
#11 [stage-1 2/2] COPY app/data.txt /data.txt
#11 DONE 0.1s
#12 DONE 1.0s
$ docker images hello --format '{{.Tag}} {{.Size}}'
scratch 337kB
multi 12.2MB
naive 406MB
```

Первые три шага этапа `builder` совпали с `Dockerfile`, поэтому взялись из кэша, и сборка
заняла секунду. 337kB, ровно как на слайде.

## Шаги 4–5. Запуск и оболочка

```text
$ docker run --rm hello:scratch
Hello, world!
данные из образа: этот файл попал сюда через COPY
второй строкой — чтобы видеть, что прочитано всё
$ docker run --rm -it hello:scratch sh
cannot attach stdin to a TTY-enabled container because stdin is not a terminal
$ docker run --rm hello:scratch sh
Hello, sh!
данные из образа: этот файл попал сюда через COPY
второй строкой — чтобы видеть, что прочитано всё
```

С `-it` из моего окна не пустило вообще (нет TTY). Без `-it` вышло смешнее: у образа
`ENTRYPOINT ["/hello"]`, и `sh` просто заменил `CMD`, став именем для приветствия. Чтобы
правда попросить оболочку, нужно заменить саму точку входа:

```text
$ docker run --rm --entrypoint sh hello:scratch
docker: Error response from daemon: failed to create task for container: failed to create shim task: OCI runtime create failed: runc create failed: unable to start container process: error during container init: exec: "sh": executable file not found in $PATH
$ docker run --rm --entrypoint /bin/sh hello:scratch
docker: Error response from daemon: failed to create task for container: failed to create shim task: OCI runtime create failed: runc create failed: unable to start container process: error during container init: exec: "/bin/sh": stat /bin/sh: no such file or directory
```

В образе два файла, `/hello` и `/data.txt`. Оболочки нет, запускать нечего.

Зачем `-static`: положил в `scratch` динамический бинарник из `hello:multi`
([Dockerfile.dyn](Dockerfile.dyn)):

```text
$ cat Dockerfile.dyn
FROM scratch
COPY --from=hello:multi /app/hello /hello
COPY app/data.txt /data.txt
ENTRYPOINT ["/hello"]
$ docker build -q -f Dockerfile.dyn -t hello:dyn .
sha256:a41bd6d607390dfa7738f1976eb4a0ecd3183ceac8c002675f0b2693eb461ba0
$ docker run --rm hello:dyn
exec /hello: no such file or directory
```

Файл `/hello` на месте, но ядро при `exec` идёт за загрузчиком `/lib/ld-musl-x86_64.so.1`,
который прописан в ELF, а его в `scratch` нет. Отсюда и «no such file» про файл, который
вроде бы есть.

## Шаг 6. docker history

```text
$ docker history hello:multi
IMAGE          CREATED         CREATED BY                                      SIZE      COMMENT
aa641245ffca   2 minutes ago   CMD ["/app/hello" "world" "/data.txt"]          0B        buildkit.dockerfile.v0
<missing>      2 minutes ago   USER 65534:65534                                0B        buildkit.dockerfile.v0
<missing>      2 minutes ago   COPY app/data.txt /data.txt # buildkit          8.19kB    buildkit.dockerfile.v0
<missing>      2 minutes ago   COPY /src/hello /app/hello # buildkit           28.7kB    buildkit.dockerfile.v0
<missing>      3 minutes ago   WORKDIR /app                                    8.19kB    buildkit.dockerfile.v0
<missing>      7 months ago    CMD ["/bin/sh"]                                 0B        buildkit.dockerfile.v0
<missing>      7 months ago    ADD alpine-minirootfs-3.20.10-x86_64.tar.gz …   8.47MB    buildkit.dockerfile.v0
```

Самый большой слой — база alpine, 8.47MB. Всё, что добавил наш Dockerfile, меньше 50kB.
Слоёв этапа `builder` в истории нет вообще.

Итог по всем образам практики:

```text
$ docker images hello --format '{{.Tag}} {{.Size}}'
dyn 40.2kB
scratch 337kB
multi 12.2MB
builder 318MB
naive 406MB
```

## Что понял

В `scratch` нет ничего, кроме скопированных файлов, поэтому бинарник должен быть
статическим: динамическому нужен загрузчик и libc, которых там нет. По той же причине
внутрь нельзя «зайти»: нет оболочки, `ps`, `ls`. Отлаживать такой образ приходится
снаружи или пересборкой на alpine.
