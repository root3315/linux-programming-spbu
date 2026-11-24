# ДЗ 3. Bash: журналы и report.sh

## Условие

Сдаётся файл `bash-homework.txt` с командами и краткими выводами. Всё делается только в
каталоге `~/bash-homework`:

1. Создать `data` и в нём три файла `*.log` с разными уровнями сообщений.
2. Дописать по строке ERROR в два файла через `>>`.
3. Найти все ERROR рекурсивно и сохранить результат без номеров строк.
4. Построить сводку: сколько сообщений каждого уровня.
5. Написать `report.sh`: принимает каталог, находит в нём `*.log`, пишет `summary.txt`.
6. Сделать сценарий исполняемым и проверить на каталоге с ошибками.

## Критерии

Базовый: команды работают, пути не прибиты абсолютно к одной машине.

Продвинутый: в сценарии есть проверки аргументов, `set -u` и понятные сообщения об ошибках.

Сделаны оба. Сверх этого `report.sh` проходит `shellcheck` без замечаний, ищет журналы и в
подкаталогах, не ломается на именах с пробелами и пишет сводку через временный файл.

## Что где лежит

| Файл | Что это |
|---|---|
| [REPORT.md](REPORT.md) | отчёт: команды, вывод и выводы по каждому шагу |
| [report.sh](report.sh) | сценарий из шага 5 |
| [data/](data/) | журналы после шагов 1–2: `app.log`, `db.log`, `nginx/web.log` |
| [errors.txt](errors.txt) | результат шага 3 |
| [summary.txt](summary.txt) | сводка, которую написал `./report.sh data` |
| [dist.sh](dist.sh) | делает `bash-homework.txt` из `REPORT.md` и собирает архив |
| `bash-homework.tar.gz` | архив для сдачи |

## Как запустить

Нужен только Bash, `find`, `awk` и `sort`, всё есть в базовом `ubuntu:24.04`. Из папки
`03-bash`:

```bash
docker run --rm -it --hostname lab -v "${PWD}:/work" -w /work ubuntu:24.04 bash
cd homework
bash report.sh data
cat summary.txt
```

Проверка сценария: `apt update && apt install -y shellcheck`, затем `shellcheck report.sh`.

## Как собрать архив

В том же контейнере, из папки `homework`:

```bash
bash dist.sh
tar tzf bash-homework.tar.gz
```

В архиве каталог `bash-homework/`: `bash-homework.txt` (тот же REPORT.md простым текстом,
заголовки подчёркнуты `===` и `---`, код с отступом в два пробела), `report.sh` и `data/` с
журналами. Проверить архив можно так:

```bash
mkdir -p /tmp/check && tar xzf bash-homework.tar.gz -C /tmp/check
cd /tmp/check/bash-homework && ./report.sh data && cat summary.txt
```
