# Практика 2. Отчёт с процентами

Ubuntu 24.04 в контейнере с systemd 255 в роли PID 1, пользователь `stepan`, Python 3.12.3. Почему контейнер, а не WSL, написано в [README практик](../README.md). Продолжаю в `~/auto-lab/Chapter11/python`. Мой [logsum.py](logsum.py) заменяет файл из первой практики.

## До правки

```text
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log --top 2
INFO   3
ERROR  3

всего строк: 8
stepan@systemd-lab:~/auto-lab/Chapter11/python$ cp ~/linux-programming-spbu/11-python-systemd/practice/02-report/logsum.py logsum.py
```

## Проценты

```text
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log
ERROR    3   37.5%
INFO     3   37.5%
WARN     2   25.0%
всего строк: 8
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log --top 2
ERROR    3   37.5%
INFO     3   37.5%
всего строк: 8
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 -c 'print(3/8*100 + 3/8*100 + 2/8*100)'
100.0
```

Процент — `count / total * 100`, сортировка по проценту от большего (ключ `(-процент, имя)`). При
равных процентах порядок по имени, поэтому ERROR теперь выше INFO: в исходной версии порядок
при равенстве зависел от того, какой уровень раньше встретился в файле. В сумме 100.

## Отчёт в JSON

```text
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log --save report.json && cat report.json
ERROR    3   37.5%
INFO     3   37.5%
WARN     2   25.0%
всего строк: 8
отчёт сохранён: report.json
{
  "total": 8,
  "levels": {
    "ERROR": {
      "count": 3,
      "percent": 37.5
    },
    "INFO": {
      "count": 3,
      "percent": 37.5
    },
    "WARN": {
      "count": 2,
      "percent": 25.0
    }
  }
}
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 -m json.tool report.json >/dev/null && echo 'JSON читается'
JSON читается
```

`--save report.json` пишет счётчики, проценты и итог через `json.dump`. Проценты округляю до одного
знака, чтобы в файле не было `37.49999`.

## Два журнала

```text
stepan@systemd-lab:~/auto-lab/Chapter11/python$ printf '%s\n' 'INFO 2026-11-21 09:00:01 server started' 'INFO 2026-11-21 09:01:00 GET / 200' 'INFO 2026-11-21 09:02:00 GET / 200' 'WARN 2026-11-21 09:03:10 slow response 2100 ms' 'DEBUG 2026-11-21 09:03:11 cache miss' > app2.log
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log app2.log
== app.log
ERROR    3   37.5%
INFO     3   37.5%
WARN     2   25.0%
всего строк: 8

== app2.log
INFO     3   60.0%
DEBUG    1   20.0%
WARN     1   20.0%
всего строк: 5

уровень  app.log -> app2.log
DEBUG      0 -> 1   (+1)
ERROR      3 -> 0   (-3)
INFO       3 -> 3   (+0)
WARN       2 -> 1   (-1)
```

Сравнение — это разность двух `Counter`: уровни из обоих файлов, `b[level] - a[level]`. У `Counter`
отсутствующий ключ равен нулю, поэтому ERROR во втором журнале без `KeyError` даёт 0.

## Плохой вход

```text
stepan@systemd-lab:~/auto-lab/Chapter11/python$ : > empty.log && python3 logsum.py empty.log; echo "код=$?"
журнал пустой
код=0
stepan@systemd-lab:~/auto-lab/Chapter11/python$ printf 'INFO ok\n\xff\xfe broken bytes\nERROR \xc3 half\n\n   \n' > broken.log && python3 logsum.py broken.log; echo "код=$?"
ERROR    1   33.3%
INFO     1   33.3%
��       1   33.3%
всего строк: 3
код=0
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py nope.log; echo "код=$?"
нет такого файла: nope.log
код=1
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py app.log nope.log; echo "код=$?"
нет такого файла: nope.log
код=1
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py /tmp; echo "код=$?"
не могу прочитать: /tmp
код=1
stepan@systemd-lab:~/auto-lab/Chapter11/python$ python3 logsum.py a b c; echo "код=$?"
usage: logsum.py [-h] [--top TOP] [--save FILE] files [files ...]
logsum.py: error: сравнить можно не больше двух файлов
код=2
stepan@systemd-lab:~/auto-lab/Chapter11/python$ rm -f report.json empty.log broken.log
```

Пустой журнал: `total = 0` проверяется до деления, программа пишет «журнал пустой» и выходит с 0.
Битые байты: файл открывается с `errors="replace"`, непонятные байты становятся `�`, строка
считается под своим «словом» и ничего не падает. Строки из одних пробелов не считаются. Нет файла,
каталог вместо файла: сообщение в stderr и код 1, даже если первый из двух файлов прочитался.
Больше двух файлов: `parser.error`, код 2.

## Вывод

Проценты в сумме дают 100, отчёт сохраняется и читается `json.tool`, пустой и битый журнал программу
не роняют, нет файла — код 1. Деление на ноль закрыл проверкой `total == 0` в `make_report`.
