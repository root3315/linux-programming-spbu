# Практики занятия 6

Все примеры из архива Chapter 06 преподавателя: `app/` (hello.c, Makefile, data.txt), три
Dockerfile, `.dockerignore` и `compose.yml`. Как на слайдах, копировал архив в
`~/docker-lab` и собирал там. В папках ниже только моё: сессии с командами и выводом и пара
своих файлов.

Работал в Docker Desktop 29.5.3 на Windows, команды набирал в Git Bash. Нужен Docker с
плагином compose и доступ к Docker Hub (образы `hello-world`, `alpine:3.20`, `ubuntu:24.04`).
Проверка, что всё на месте: `docker run --rm hello-world`.

## Практика 1. Первый контейнер

Ничего не собирать, только запускать. `docker run --rm hello-world` и разобрать, что
произошло. Зайти в `alpine:3.20`, посмотреть `/etc/os-release`, `ps aux`, `ls /`, `hostname`
и сравнить с хостом. Выйти и понять, почему контейнера нет в `docker ps -a`. Запустить
`sleep 300` в фоне, зайти в него через `docker exec`, посмотреть `docker logs`, остановить и
удалить. Критерий: объяснить, почему `ps` внутри показывает два процесса, а не сотни.

Решение: [01-first-container/session.md](01-first-container/session.md). Сверх задания
посмотрел namespaces в `/proc/1/ns`, лимиты cgroups и сколько длится `docker stop` с
`--init` и без.

## Практика 2. Наивный образ

Собрать `Dockerfile.naive`, где компилятор остаётся в образе, измерить `docker images`,
запустить, разобрать `docker history`. Критерий: назвать два слоя, которые дают весь вес, и
объяснить, почему в готовом образе они не нужны.

Решение: [02-naive-image/](02-naive-image/): [session.md](02-naive-image/session.md) и
[Dockerfile.naive](02-naive-image/Dockerfile.naive) с базой `ubuntu:24.04` вместо
`debian:bookworm`. Там же проверка кэша слоёв.

```bash
cp 02-naive-image/Dockerfile.naive ~/docker-lab/ && cd ~/docker-lab
docker build -f Dockerfile.naive -t hello:naive . && docker run --rm hello:naive
docker history hello:naive
```

## Практика 3. Multistage и scratch

Собрать `hello:multi` (multistage, `Dockerfile`) и `hello:scratch` (статический бинарник на
пустой базе), сравнить с наивным. Запустить `scratch` и попробовать зайти в него оболочкой.
Найти самый большой слой в `docker history hello:multi`. Критерий: объяснить, почему
`scratch` требует `-static` и почему в такой образ нельзя «зайти и посмотреть».

Решение: [03-multistage-scratch/session.md](03-multistage-scratch/session.md) и
[Dockerfile.dyn](03-multistage-scratch/Dockerfile.dyn): `scratch` с динамическим
бинарником, чтобы увидеть, как он падает.

```bash
cp 03-multistage-scratch/Dockerfile.dyn ~/docker-lab/ && cd ~/docker-lab
docker build -t hello:multi . && docker build -f Dockerfile.scratch -t hello:scratch .
docker images hello --format '{{.Tag}} {{.Size}}'
docker build -f Dockerfile.dyn -t hello:dyn . && docker run --rm hello:dyn
```

## Практика 4. compose из двух сервисов

Поднять два контейнера из одного образа с разными `GREETING`: `docker compose config`,
`up`, `up -d` и `ps`, поменять `GREETING` и перезапустить один сервис, `logs`, `down`.
Критерий: объяснить, почему сервисы не «висят», и чем `docker compose down` отличается от
`stop`.

Решение: [04-compose/session.md](04-compose/session.md). Своих файлов нет. Там же пример с
томом `demo-data` со слайда про данные.

```bash
cd ~/docker-lab
docker compose up && docker compose ps -a && docker compose down
```

## Уборка

Образы практик живут в Docker, а не в каталоге:

```bash
docker rmi hello:naive hello:multi hello:scratch hello:builder hello:dyn \
    docker-lab-hello-ru docker-lab-hello-en
```
