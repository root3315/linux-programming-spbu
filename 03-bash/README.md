# Занятие 3. Bash: от команды к автоматизации

Лекция-практикум по командной строке. Начали с того, как Bash разбирает строку до запуска
программы (раскрытие имён, кавычки, `printf`), прошли файлы и права, `find` и `grep`,
редакторы, потоки и конвейеры, затем сценарии с `set -euo pipefail`. В конце SSH с ключами,
`~/.ssh/config` и туннелями и tmux для долгой удалённой работы. По ходу лекции было четыре
практики, финальный сценарий `count_txt.sh` и практики по SSH и tmux. Домашнее задание:
разобрать журналы командами и написать `report.sh`, который строит по ним сводку.

## Что внутри

- [notes.md](notes.md): конспект лекции;
- [notes.pdf](notes.pdf): он же в PDF;
- [practice/](practice/README.md): условия семи практик и решения с записью сеансов;
- [homework/](homework/README.md): ДЗ, `report.sh`, отчёт и архив `bash-homework.tar.gz`.

## Как запустить

Всё проверялось в контейнере с Ubuntu 24.04 под обычным пользователем. Из папки `03-bash`:

```bash
docker run --rm -it --hostname lab -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y shellcheck tmux openssh-server rsync iproute2 curl python3
useradd -m -s /bin/bash stepan
```

Для `report.sh`, `count_txt.sh` и практик 1–4 ничего ставить не нужно, хватает базового
образа (кроме `man`, см. ниже). `shellcheck` нужен для проверки сценариев, `tmux`
для практики 7, остальное для практики 6 с SSH. Для `man` в урезанном образе ещё
`unminimize` и `apt install man-db`. `--hostname lab` нужен, только чтобы приглашение было
как в записях сеансов.

Сводка по журналам из ДЗ, там же, в `/work`:

```bash
cd homework
bash report.sh data
cat summary.txt
```

Для ручного повтора сеансов из `practice/` дальше `su - stepan`.

Как запускать контейнер, написано в [CONTRIBUTING.md](../CONTRIBUTING.md).
