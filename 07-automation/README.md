# Занятие 7. Автоматическая сборка

Лекция-практикум о том, как сделать, чтобы сборку, тесты и пакет делал скрипт, а не я по
памяти. Git: индекс, ветки, слияние и конфликт, `.gitignore`, `git bisect`. CI: `ci.yml` для
GitHub Actions и локальный `ci.sh` с `set -eu`. Пакет `.deb`: `control`, версии,
`DESTDIR` и `dpkg-deb`. Профилирование: `time`, `gprof`, зачем `perf`. Примеры из архива
Chapter 07 преподавателя. В ДЗ свой проект `hello` из ДЗ 4 получил версию в `--version`,
`ci.sh`, workflow, пакет 1.1.0 и ускорение горячей функции по профилю.

## Что внутри

- [notes.md](notes.md): конспект лекции;
- [notes.pdf](notes.pdf): он же в PDF;
- [practice/](practice/): четыре практики, условия в [practice/README.md](practice/README.md);
- [homework/](homework/): ДЗ, условие и критерии в [homework/README.md](homework/README.md),
  отчёт в [homework/REPORT.md](homework/REPORT.md), архив `automation-homework.tar.gz`.

Workflow ДЗ лежит в корне репозитория: [.github/workflows/ci.yml](../.github/workflows/ci.yml).

## Как запустить

В контейнере с Ubuntu 24.04, как в остальных занятиях:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential git
cd homework/hello && ./ci.sh
```

Для практик нужен ещё архив Chapter 07 с примерами (его в репозитории нет, это материалы
преподавателя), подготовка и команды в [practice/README.md](practice/README.md). Подробнее про
контейнер написано в [CONTRIBUTING.md](../CONTRIBUTING.md).
