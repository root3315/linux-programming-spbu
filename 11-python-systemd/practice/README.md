# Практики занятия 11

Четыре практики на примерах из архива преподавателя: четыре Python-скрипта (`python/`) и юниты
systemd (`systemd/`). Вторая практика продолжает первую, четвёртая берёт скрипт из второй. В
папках ниже только моё: изменённые и новые файлы и сессии с командами и выводом.

## Подготовка

```bash
mkdir -p ~/auto-lab && cd ~/auto-lab
wget https://www.smartmelio.ru/chapter11.tar.gz
tar -xzvf chapter11.tar.gz
```

Получится `Chapter11/` (на слайде и в README архива каталог называется `Chapter 11` с пробелом, в
самом архиве пробела нет). Нужны только `python3` и systemd, в Ubuntu 24.04 оба есть: Python
3.12.3, systemd 255.

Что я поправил в архиве, прежде чем работать:

- В архиве нет `python/app.log`, хотя его ждут и `logsum.py`, и `logsum.service`. Свой журнал на
  восемь строк положил в [01-scripts/app.log](01-scripts/app.log): три INFO, три ERROR, два WARN,
  как в выводе на слайде.
- Все файлы с концами строк CRLF и без бита выполнения. Python это не мешает, а
  `./systemd/install-units.sh` не запускается: `Permission denied`, а через `sh` падает на
  `set: Illegal option -`, потому что видит `set -eu\r`. Лечится так:

  ```bash
  sed -i 's/\r$//' systemd/*.sh systemd/*.service systemd/*.timer
  chmod +x systemd/*.sh
  ```

systemd у меня в WSL выключен (PID 1 там `init`), поэтому всё, что касается systemd, проверял в
контейнере Ubuntu 24.04, где PID 1 — настоящий systemd 255, а у пользователя `stepan` запущена
своя сессия (`systemctl --user is-system-running` даёт `running`). Отсюда имя машины
`systemd-lab` в сессиях.

## Практика 1. Свои скрипты

Запустить и разобрать четыре готовых скрипта: `logsum.py` (уровни в журнале, с `--top 2`),
`watchdir.py /var/log --top 3` (что занимает место), `httpcheck.py` (жив ли сайт),
`rename.py ~/tmp --prefix trip_` (почему ничего не изменилось). Добавить в `logsum.py` ключ
`--json`. Критерий: объяснить каждую строку `logsum.py` и почему ошибка возвращает код 1.

Решение: [01-scripts/session.md](01-scripts/session.md), мой [logsum.py](01-scripts/logsum.py).

## Практика 2. Отчёт с процентами

К уровням добавить проценты (`count / total * 100`), сортировать по проценту, сохранять отчёт через
`json.dump`, запустить на двух файлах и сравнить, проверить код 1 при отсутствии файла. Критерий:
проценты в сумме дают 100, отчёт сохраняется, битый вход не роняет программу, пустой журнал не
даёт деления на ноль.

Решение: [02-report/session.md](02-report/session.md), мой [logsum.py](02-report/logsum.py).

## Практика 3. Служба и таймер

Поставить юниты из архива в свою сессию (`install-units.sh`), запустить `logsum.service`,
посмотреть `status` и журнал, включить `logsum.timer` через `enable --now`, дождаться запуска,
посмотреть `list-timers`, убрать всё за собой (`uninstall-units.sh`). Критерий: различать `start`,
`enable` и `--now`, найти в журнале вывод своей программы.

Решение: [03-units/session.md](03-units/session.md). Своих файлов нет, работал с юнитами архива.

## Практика 4. Своё расписание

Взять `logsum.py` с процентами, написать `mylog.service` со своим путём и `mylog.timer` раз в минуту,
проверить `systemd-analyze verify`, включить, найти запуск в журнале, потом поменять расписание на
«каждый день в 3:00». Критерий: объяснить, зачем `--user`, чем service отличается от timer и где
искать результат.

Решение: [04-schedule/session.md](04-schedule/session.md), мои
[mylog.service](04-schedule/mylog.service) и [mylog.timer](04-schedule/mylog.timer).
