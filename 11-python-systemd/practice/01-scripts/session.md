# Практика 1. Свои скрипты

Ubuntu 24.04 в контейнере с systemd 255 в роли PID 1, пользователь `stepan`, Python 3.12.3. Почему контейнер, а не WSL, написано в [README практик](../README.md). Работаю в `~/auto-lab`, мой журнал [app.log](app.log) и [logsum.py](logsum.py) с ключом `--json` лежат рядом.

## Скачать и распаковать

```text
stepan@systemd-lab:~/auto-lab$ wget -q https://www.smartmelio.ru/chapter11.tar.gz && tar -xzvf chapter11.tar.gz
Chapter11/
Chapter11/README.md
Chapter11/python/
Chapter11/python/rename.py
Chapter11/python/logsum.py
Chapter11/python/httpcheck.py
Chapter11/python/watchdir.py
Chapter11/.gitignore
Chapter11/systemd/
Chapter11/systemd/logsum.timer
Chapter11/systemd/demo.service
Chapter11/systemd/uninstall-units.sh
Chapter11/systemd/logsum.service
Chapter11/systemd/install-units.sh
stepan@systemd-lab:~/auto-lab$ cd Chapter11
stepan@systemd-lab:~/auto-lab/Chapter11$ ls -l python systemd
python:
total 16
-rw-r--r-- 1 stepan stepan 1709 Nov  3 10:23 httpcheck.py
-rw-r--r-- 1 stepan stepan 2554 Nov  3 10:23 logsum.py
-rw-r--r-- 1 stepan stepan 2300 Nov  3 10:23 rename.py
-rw-r--r-- 1 stepan stepan 2017 Nov  3 10:23 watchdir.py

systemd:
total 20
-rw-r--r-- 1 stepan stepan  425 Nov  3 10:23 demo.service
-rw-r--r-- 1 stepan stepan 1025 Nov  3 10:23 install-units.sh
-rw-r--r-- 1 stepan stepan 1211 Nov  3 10:23 logsum.service
-rw-r--r-- 1 stepan stepan  942 Nov  3 10:23 logsum.timer
-rw-r--r-- 1 stepan stepan  577 Nov  3 10:23 uninstall-units.sh
stepan@systemd-lab:~/auto-lab/Chapter11$ file python/logsum.py systemd/install-units.sh
python/logsum.py:         Python script, Unicode text, UTF-8 text executable, with CRLF line terminators
systemd/install-units.sh: POSIX shell script, Unicode text, UTF-8 text executable, with CRLF line terminators
stepan@systemd-lab:~/auto-lab/Chapter11$ python3 --version
Python 3.12.3
```

Бита выполнения нет ни у одного файла, концы строк CRLF. Python это не мешает: `python3 файл.py`
читает `\r\n` спокойно.

## logsum.py

```text
stepan@systemd-lab:~/auto-lab/Chapter11$ cd python
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log; echo "код=$?"
нет такого файла: app.log
код=1
stepan@systemd-lab:~/auto-lab/Chapter11/python$ cp ~/linux-programming-spbu/11-python-systemd/practice/01-scripts/app.log . && cat app.log
INFO 2026-11-20 09:00:01 server started on port 8000
ERROR 2026-11-20 09:00:05 database connection refused
INFO 2026-11-20 09:01:12 GET / 200
WARN 2026-11-20 09:02:40 slow response 1850 ms
ERROR 2026-11-20 09:03:02 database connection refused
INFO 2026-11-20 09:04:18 GET /about 200
WARN 2026-11-20 09:05:51 disk usage 91%
ERROR 2026-11-20 09:06:30 worker crashed, restarting
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log
INFO   3
ERROR  3
WARN   2

всего строк: 8
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log --top 2
INFO   3
ERROR  3

всего строк: 8
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py nope.log; echo "код=$?"
нет такого файла: nope.log
код=1
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log --top два; echo "код=$?"
usage: logsum.py [-h] [--top TOP] file
logsum.py: error: argument --top: invalid int value: 'два'
код=2
```

Первый запуск сразу упал на коде 1: `app.log` в архиве нет. Положил свой журнал из восьми строк.
Дальше всё как на слайде: три уровня и итог, `--top 2` отрезает хвост. Ошибки двух видов: файла нет
(свой `except FileNotFoundError`, код 1) и неверный аргумент (это ловит argparse, код 2).

Что делает `logsum.py` по строкам. `count_levels` открывает файл в UTF-8, идёт по строкам, пустые
пропускает и прибавляет единицу в `Counter` по первому слову. Если файла нет, `open` бросает
`FileNotFoundError`, и функция его не ловит: что делать с ошибкой, решает `main`. `main` разбирает
аргументы через argparse, ловит `FileNotFoundError` и `PermissionError`, печатает сообщение в stderr
и возвращает 1. Пустой журнал не ошибка, код 0. `most_common(N)` даёт N самых частых (при `--top 0`
передаётся `None`, то есть все), ширина колонки считается по самому длинному имени. Последняя строка
`sys.exit(main(...))` делает возвращённое число кодом процесса. Код 1 нужен, чтобы тот, кто запустил
скрипт (shell, CI, systemd), понял, что результата нет.

## watchdir.py

```text
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 watchdir.py /var/log --top 3
    24.0 МБ  journal
     0.0 МБ  apt
     0.0 МБ  private
stepan@systemd-lab:~/auto-lab/Chapter11/python$ du -s --block-size=1 /var/log/* 2>/dev/null | sort -nr | head -3
25178112	/var/log/journal
212992	/var/log/dpkg.log
61440	/var/log/bootstrap.log
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 watchdir.py /нет; echo "код=$?"
это не каталог: /нет
код=1
```

`watchdir.py` суммирует размеры файлов через `os.walk`. С `du` цифры чуть расходятся: `du` считает
занятые блоки, а скрипт длину файлов, и `du` показывает и файлы в корне `/var/log`, а скрипт только
подкаталоги.

## httpcheck.py

```text
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 -m http.server 8000 --bind 127.0.0.1 >/dev/null 2>&1 & sleep 1
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 httpcheck.py http://127.0.0.1:8000/; echo "код=$?"
200      37 мс  http://127.0.0.1:8000/
код=0
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 httpcheck.py http://127.0.0.1:8000/nope http://127.0.0.1:9/; echo "код=$?"
404      92 мс  http://127.0.0.1:8000/nope
НЕ ОТВЕТИЛ: http://127.0.0.1:9/ — <urlopen error [Errno 111] Connection refused>
код=1
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 httpcheck.py; echo "код=$?"
использование: httpcheck.py URL [URL …]
код=2
stepan@systemd-lab:~/auto-lab/Chapter11/python$ kill %1
```

Три исхода, как на слайде: 200, ответ ошибкой (404 — это тоже ответ, `HTTPError` с кодом) и «не
ответил» (`URLError`, порт 9 закрыт). Если хоть один адрес не ответил, код 1. Без аргументов код 2.
404 при этом скрипт плохим не считает, код 0 был бы, если бы не второй адрес. Для ночной проверки в
ДЗ я это поменял: там 4xx и 5xx тоже проблема.

## rename.py

```text
stepan@systemd-lab:~/auto-lab/Chapter11/python$ mkdir -p ~/tmp && touch ~/tmp/beach.jpg ~/tmp/sunset.jpg ~/tmp/trip_old.jpg && ls ~/tmp
beach.jpg
sunset.jpg
trip_old.jpg
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 rename.py ~/tmp --prefix trip_; ls ~/tmp
beach.jpg  ->  trip_beach.jpg
sunset.jpg  ->  trip_sunset.jpg

это пример. Чтобы выполнить: --apply (файлов: 2)
beach.jpg
sunset.jpg
trip_old.jpg
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 rename.py ~/tmp --prefix trip_ --apply; ls ~/tmp
beach.jpg  ->  trip_beach.jpg
sunset.jpg  ->  trip_sunset.jpg

переименовано файлов: 2
trip_beach.jpg
trip_old.jpg
trip_sunset.jpg
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 rename.py ~/tmp --prefix trip_
нечего переименовывать
```

Почему ничего не изменилось: без `--apply` скрипт только печатает план. Файл `trip_old.jpg` уже с
префиксом, его он пропускает. Второй запуск с `--apply` переименовал два файла, третий ничего не
нашёл.

## Ключ --json

```text
stepan@systemd-lab:~/auto-lab/Chapter11/python$ cp ~/linux-programming-spbu/11-python-systemd/practice/01-scripts/logsum.py logsum.py
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log --json
{
  "levels": {
    "INFO": 3,
    "ERROR": 3,
    "WARN": 2
  },
  "total": 8
}
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log --json --top 1
{
  "levels": {
    "INFO": 3
  },
  "total": 8
}
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log --json | python3 -c 'import json,sys; print(json.load(sys.stdin)["levels"]["ERROR"])'
3
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py --help
usage: logsum.py [-h] [--top TOP] [--json] file

Счётчик уровней в журнале

positional arguments:
  file        файл журнала

options:
  -h, --help  show this help message and exit
  --top TOP   показать только N самых частых уровней
  --json      напечатать результат в JSON вместо таблицы
```

`--json` печатает то же самое в JSON: уровни в порядке частоты и итог. `--top` работает и тут, а
`total` остаётся общим числом строк. JSON читается другой программой, это видно на однострочнике с
`json.load(sys.stdin)`. `ensure_ascii=False`, чтобы русские буквы не превращались в `\uXXXX`.

## Вывод

Готовые скрипты устроены одинаково: argparse для аргументов, функция, которая делает работу и
бросает исключение, и `main`, которая ловит ошибку, пишет в stderr и возвращает код. Самая полезная
находка — `app.log` в архиве нет, а служба из практики 3 без него падала бы.
