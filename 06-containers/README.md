# Занятие 6. Образы корневых файловых систем

Лекция-практикум про Docker. Разбирали, чем контейнер отличается от виртуалки, из каких
слоёв собран образ и как читать `docker history`, инструкции Dockerfile, `CMD` против
`ENTRYPOINT` и кэш слоёв. Потом доводили образ маленькой программы на C от сотен мегабайт
до сотен килобайт через multistage и `scratch`, смотрели namespaces, cgroups и PID 1 с
сигналами изнутри контейнера, в конце тома и compose. Примеры из архива Chapter 06
преподавателя. В ДЗ упаковал свой проект `hello` из ДЗ 4: три варианта образа, обработчик
SIGTERM и compose с двумя сервисами и томом.

## Что внутри

- [notes.md](notes.md): конспект лекции;
- [notes.pdf](notes.pdf): он же в PDF;
- [practice/](practice/): четыре практики, условия в [practice/README.md](practice/README.md);
- [homework/](homework/): ДЗ, условие и критерии в [homework/README.md](homework/README.md),
  отчёт в [homework/REPORT.md](homework/REPORT.md), архив `docker-homework.tar.gz`.

## Как запустить

Для этого занятия нужен сам Docker с плагином compose: всё делал в Docker Desktop 29.5.3,
базовые образы `alpine:3.20` и `ubuntu:24.04`. ДЗ целиком:

```bash
cd homework
./check.sh --no-cache
docker compose up        # Ctrl+C, потом docker compose down -v
```

Приложение из ДЗ без Docker собирается в контейнере с Ubuntu 24.04, как в остальных
занятиях:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential
cd homework/app && make && ./hello
```

Для практик нужен ещё архив Chapter 06 с примерами (его в репозитории нет, это материалы
преподавателя), команды по шагам в `session.md` каждой практики. Подробнее про контейнер
написано в [CONTRIBUTING.md](../CONTRIBUTING.md).
