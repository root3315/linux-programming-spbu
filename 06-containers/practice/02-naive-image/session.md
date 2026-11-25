# Практика 2. Наивный образ

Скопировал Chapter 06 в `~/docker-lab` и собирал там. В
[Dockerfile.naive](Dockerfile.naive) поменял одну строку: `FROM debian:bookworm` на
`FROM ubuntu:24.04`, весь остальной курс у меня на Ubuntu 24.04. Остальное как в архиве.

## Сборка

```text
$ time docker build --progress=plain -f Dockerfile.naive -t hello:naive .
...
#3 [internal] load .dockerignore
#3 transferring context: 202B done
...
#5 [internal] load build context
#5 transferring context: 1.83kB 0.0s done
...
#6 [2/5] RUN apt-get update     && apt-get install -y --no-install-recommends gcc make libc6-dev     && rm -rf /var/lib/apt/lists/*
#6 13.80 Suggested packages:
#6 13.80 Recommended packages:
#6 15.33 2 upgraded, 38 newly installed, 0 to remove and 2 not upgraded.
#6 28.08 debconf: delaying package configuration, since apt-utils is not installed
...
#6 29.06 debconf: unable to initialize frontend: Dialog
#6 29.06 debconf: (TERM is not set, so the dialog frontend is not usable.)
...
#6 DONE 58.0s

#7 [3/5] WORKDIR /app
#7 DONE 0.1s

#8 [4/5] COPY app/ .
#8 DONE 0.1s

#9 [5/5] RUN make
#9 0.217 gcc -std=c11 -Wall -Wextra -O2 -o hello hello.c
#9 DONE 0.3s

#10 exporting to image
...
#10 DONE 35.0s

real	1m35.845s
```

Контекст 1.83kB: `.dockerignore` выкинул README, Dockerfile и compose.yml. Почти минута на
`apt-get install` (38 пакетов) и ещё полминуты на экспорт слоёв. Строки `debconf: unable to
initialize frontend` — не ошибка: apt хочет задавать вопросы, а терминала нет. Убирается
через `ENV DEBIAN_FRONTEND=noninteractive`, в файле преподавателя этого нет, я не трогал.

## Размер и запуск

```text
$ docker images hello:naive
IMAGE         ID             DISK USAGE   CONTENT SIZE   EXTRA
hello:naive   73cc98424f1c        406MB          108MB
$ docker run --rm hello:naive | head -2
Hello, world!
данные из образа: этот файл попал сюда через COPY
```

У меня Docker 29, и `docker images` показывает две колонки. `DISK USAGE` 406MB (распакованные
слои плюс сжатые), `CONTENT SIZE` 108MB (столько качать из реестра). Для сравнения база:

```text
$ docker images ubuntu:24.04
IMAGE          ID             DISK USAGE   CONTENT SIZE   EXTRA
ubuntu:24.04   9c7f200c07e3        119MB         31.7MB   U
```

## Откуда вес

```text
$ docker history hello:naive
IMAGE          CREATED              CREATED BY                                      SIZE      COMMENT
73cc98424f1c   About a minute ago   CMD ["./hello" "world" "/app/data.txt"]         0B        buildkit.dockerfile.v0
<missing>      About a minute ago   RUN /bin/sh -c make # buildkit                  24.6kB    buildkit.dockerfile.v0
<missing>      About a minute ago   COPY app/ . # buildkit                          20.5kB    buildkit.dockerfile.v0
<missing>      About a minute ago   WORKDIR /app                                    8.19kB    buildkit.dockerfile.v0
<missing>      About a minute ago   RUN /bin/sh -c apt-get update     && apt-get…   210MB     buildkit.dockerfile.v0
<missing>      13 days ago          /bin/sh -c #(nop)  CMD ["/bin/bash"]            0B
<missing>      13 days ago          /bin/sh -c #(nop) ADD file:54dafd97985c3ec22…   87.7MB
<missing>      13 days ago          /bin/sh -c #(nop)  LABEL org.opencontainers.…   0B
<missing>      13 days ago          /bin/sh -c #(nop)  ARG LAUNCHPAD_BUILD_ARCH     0B
<missing>      13 days ago          /bin/sh -c #(nop)  ARG RELEASE                  0B
```

Два слоя дают почти весь вес: `apt-get install gcc make libc6-dev` 210MB и база Ubuntu
87.7MB. Наша часть: исходники 20.5kB и результат `make` 24.6kB, то есть около 45kB из
~300MB распакованных слоёв. Компилятор и заголовки нужны только во время сборки, программе
при запуске хватает libc, которая и так есть в базе.

Сумма сходится: 87.7 + 210 ≈ 298MB распакованного, плюс 108MB сжатого = 406MB `DISK USAGE`.
На слайде с debian было 477MB, у Ubuntu база меньше.

## Кэш слоёв

Собрал второй раз без изменений:

```text
$ time docker build --progress=plain -f Dockerfile.naive -t hello:naive . 2>&1 | grep -E '^#[0-9]+ (\[|CACHED|DONE)'
...
#6 [2/5] RUN apt-get update     && apt-get install -y --no-install-recommends gcc make libc6-dev     && rm -rf /var/lib/apt/lists/*
#6 CACHED
#7 [3/5] WORKDIR /app
#7 CACHED
#8 [4/5] COPY app/ .
#8 CACHED
#9 [5/5] RUN make
#9 CACHED
#10 DONE 0.5s

real	0m3.247s
```

Потом дописал строку в `app/data.txt` и собрал ещё раз:

```text
$ echo 'третья строка: дописал её, чтобы проверить кэш' >> app/data.txt
$ time docker build --progress=plain -f Dockerfile.naive -t hello:naive . 2>&1 | grep -E '^#[0-9]+ (\[[0-9]|CACHED|DONE)'
...
#6 [2/5] RUN apt-get update     && apt-get install -y --no-install-recommends gcc make libc6-dev     && rm -rf /var/lib/apt/lists/*
#6 CACHED
#7 [3/5] WORKDIR /app
#7 CACHED
#8 [4/5] COPY app/ .
#8 DONE 0.1s
#9 [5/5] RUN make
#9 DONE 0.6s
#10 DONE 0.3s

real	0m2.103s
$ docker run --rm hello:naive
Hello, world!
данные из образа: этот файл попал сюда через COPY
второй строкой — чтобы видеть, что прочитано всё
третья строка: дописал её, чтобы проверить кэш
```

Изменился файл в `app/`, значит, пересобрался `COPY app/ .` и всё ниже него. Установка
пакетов выше и осталась в кэше. Строку потом убрал, чтобы дальше сравнивать с исходным
вариантом.

## Что понял

Весь вес наивного образа — база и установка компилятора, программа в нём занимает около
0,01 % (45kB из 406MB). Компилятор нужен один раз при сборке, а в таком Dockerfile он уезжает вместе с
образом навсегда.
