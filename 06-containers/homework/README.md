# ДЗ 6. Образ для своего приложения

## Условие

Сдать `docker-homework.tar.gz`: приложение, Dockerfile, `compose.yml` и `report.txt` с
размерами.

1. Взять своё приложение из прошлых работ (или `app/` из Chapter 06).
2. Написать наивный Dockerfile и измерить размер.
3. Переписать его в multistage и измерить снова.
4. Добавить `USER`, фиксированный тег и `.dockerignore`.
5. Описать в `compose.yml` два сервиса с разными переменными окружения.
6. В `report.txt` таблица «вариант → размер → что внутри».

Структура отчёта со слайда: Dockerfile и размер; docker history, самый большой слой; что
изменилось после multistage; вывод compose up; что было непонятно.

## Критерии

Базовый: все образы собираются с нуля, `docker run` даёт ожидаемый вывод, финальный образ
меньше 20 МБ. Выполнен: `./check.sh --no-cache` собирает три образа без кэша и запускает
каждый. Финальный `hello-hw:multi` 12.2MB.

Продвинутый: вариант на `scratch`, обработчик SIGTERM с корректным завершением и
`compose.yml` с томом. Выполнен: `hello-hw:scratch` 93.4kB; в режиме `--serve` программа
ловит SIGTERM и выходит с кодом 0, `docker stop -t 10` занимает 0.5 с вместо 10; оба
сервиса в compose пишут журнал в общий том `visits`. Замеры в [REPORT.md](REPORT.md).

## Что где лежит

| Файл | Что это |
|---|---|
| `app/main.c` | точка входа; режим `--serve`: ждёт SIGTERM/SIGINT через `sigsuspend`, пишет в `LOG_FILE` |
| `app/greet.c`, `app/greet.h` | приветствие, слово берётся из `GREETING` |
| `app/stats.c`, `app/stats.h` | статистика по имени, из ДЗ 4 без изменений |
| `app/Makefile` | цели `all`, `static` (`-static -s`), `clean` |
| [Dockerfile.naive](Dockerfile.naive) | `ubuntu:24.04` + gcc, сборка прямо в образе |
| [Dockerfile](Dockerfile) | multistage на `alpine:3.20`, `USER 65534`, каталог `/data` под том |
| [Dockerfile.scratch](Dockerfile.scratch) | статический бинарник на `scratch` |
| [.dockerignore](.dockerignore) | в контекст попадает только `app/` с исходниками |
| [compose.yml](compose.yml) | `hello-ru` и `hello-en` из одного образа, разные `GREETING`, том `visits` |
| [check.sh](check.sh) | собирает три образа, печатает размеры, запускает, проверяет `docker stop` |
| [REPORT.md](REPORT.md) | отчёт |
| [dist.sh](dist.sh) | делает `report.txt` из `REPORT.md` и собирает архив |
| `docker-homework.tar.gz` | архив для сдачи |

Исходники приложения взяты из ДЗ 4 ([../../04-build-toolchain/homework/](../../04-build-toolchain/homework/)),
поменялись `main.c`, `greet.c`, `greet.h` и `Makefile`.

## Как собрать и запустить

Нужен Docker с плагином compose. Из этой папки:

```bash
./check.sh --no-cache          # три образа с нуля, размеры, запуск, проверка SIGTERM
docker run --rm hello-hw:multi
docker run --rm -e GREETING=Привет hello-hw:scratch Степан

docker run -d --name srv hello-hw:multi --serve Stepan
time docker stop -t 10 srv && docker logs srv && docker rm srv

docker compose up              # остановить: Ctrl+C или docker compose stop
docker compose down -v
```

В Git Bash на Windows перед командами с путями внутри контейнера нужен
`export MSYS_NO_PATHCONV=1`, иначе `/usr/local/bin/hello` превратится в путь Windows.
`check.sh` выставляет это сам.

Само приложение собирается и без Docker, например в контейнере с Ubuntu 24.04:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential
cd app && make && ./hello && make clean
```

## Как сделать архив

```bash
./dist.sh
```

Скрипт переводит `REPORT.md` в простой текст `report.txt` (заголовки подчёркнуты `===` и
`---`, код с отступом в два пробела) и пакует `docker-homework/` с `app/`, тремя
Dockerfile, `.dockerignore`, `compose.yml`, `check.sh` и `report.txt`. Архив проверил:
распаковал в чистом `ubuntu:24.04` и собрал `app/` через `make` и `make static`, а на хосте
из распакованного каталога прошли `./check.sh` и `docker compose up -d`.

## Уборка

```bash
docker compose down -v
docker rmi hello-hw:naive hello-hw:multi hello-hw:scratch
```
