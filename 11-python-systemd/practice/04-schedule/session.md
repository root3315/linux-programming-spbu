# Практика 4. Своё расписание

Ubuntu 24.04 в контейнере с systemd 255 в роли PID 1, пользователь `stepan`, Python 3.12.3. Почему контейнер, а не WSL, написано в [README практик](../README.md). Мои [mylog.service](mylog.service) и [mylog.timer](mylog.timer). Скрипт и журнал специально положил в каталог с пробелом, `~/auto-lab/my lab`, чтобы проверить кавычки.

## Служба с моим путём

```text
stepan@systemd-lab:~/auto-lab$ mkdir -p 'my lab' && cp ~/linux-programming-spbu/11-python-systemd/practice/02-report/logsum.py ~/linux-programming-spbu/11-python-systemd/practice/01-scripts/app.log 'my lab/' && ls 'my lab'
app.log
logsum.py
stepan@systemd-lab:~/auto-lab$ mkdir -p ~/.config/systemd/user && install -m 644 ~/linux-programming-spbu/11-python-systemd/practice/04-schedule/mylog.service ~/linux-programming-spbu/11-python-systemd/practice/04-schedule/mylog.timer ~/.config/systemd/user/
stepan@systemd-lab:~/auto-lab$ cd ~/.config/systemd/user
stepan@systemd-lab:~/.config/systemd/user$ systemd-analyze --user verify mylog.service mylog.timer && echo 'ошибок нет'
ошибок нет
stepan@systemd-lab:~/.config/systemd/user$ systemctl --user daemon-reload
stepan@systemd-lab:~/.config/systemd/user$ systemctl --user start mylog.service; systemctl --user status mylog.service --no-pager | head -3
○ mylog.service - Мой подсчёт уровней в журнале
     Loaded: loaded (/home/stepan/.config/systemd/user/mylog.service; static)
     Active: inactive (dead)
stepan@systemd-lab:~/.config/systemd/user$ journalctl --user -u mylog.service -n 20 --no-pager -o short
Nov 26 13:36:16 systemd-lab systemd[115]: Starting mylog.service - Мой подсчёт уровней в журнале...
Nov 26 13:36:16 systemd-lab python3[856]: ERROR    3   37.5%
Nov 26 13:36:16 systemd-lab python3[856]: INFO     3   37.5%
Nov 26 13:36:16 systemd-lab python3[856]: WARN     2   25.0%
Nov 26 13:36:16 systemd-lab python3[856]: всего строк: 8
Nov 26 13:36:16 systemd-lab python3[856]: отчёт сохранён: /home/stepan/auto-lab/my lab/report.json
Nov 26 13:36:16 systemd-lab systemd[115]: Finished mylog.service - Мой подсчёт уровней в журнале.
stepan@systemd-lab:~/.config/systemd/user$ cat ~/auto-lab/'my lab'/report.json | head -6
{
  "total": 8,
  "levels": {
    "ERROR": {
      "count": 3,
      "percent": 37.5
```

Юниты копирую через `install -m 644`: у файла с битом выполнения `systemd-analyze verify` ругается
`is marked executable`. В `ExecStart` абсолютные пути через `%h` (домашний каталог) и в кавычках.
`verify` молчит, ручной запуск прошёл, отчёт записался.

## Что будет без кавычек

```text
stepan@systemd-lab:~/.config/systemd/user$ cp mylog.service /tmp/mylog.service.good && sed -i 's/"//g' mylog.service && grep ExecStart mylog.service
# в имени каталога пробел, поэтому оба пути в ExecStart в кавычках.
ExecStart=/usr/bin/python3 %h/auto-lab/my lab/logsum.py %h/auto-lab/my lab/app.log --save %h/auto-lab/my lab/report.json
stepan@systemd-lab:~/.config/systemd/user$ systemd-analyze --user verify mylog.service && echo 'ошибок нет'
ошибок нет
stepan@systemd-lab:~/.config/systemd/user$ systemctl --user daemon-reload && systemctl --user start mylog.service; echo "код=$?"
Job for mylog.service failed because the control process exited with error code.
See "systemctl --user status mylog.service" and "journalctl --user -xeu mylog.service" for details.
код=1
stepan@systemd-lab:~/.config/systemd/user$ systemctl --user status mylog.service --no-pager | head -4
× mylog.service - Мой подсчёт уровней в журнале
     Loaded: loaded (/home/stepan/.config/systemd/user/mylog.service; static)
     Active: failed (Result: exit-code) since Thu 2026-11-26 13:36:16 MSK; 26ms ago
    Process: 875 ExecStart=/usr/bin/python3 /home/stepan/auto-lab/my lab/logsum.py /home/stepan/auto-lab/my lab/app.log --save /home/stepan/auto-lab/my lab/report.json (code=exited, status=2)
stepan@systemd-lab:~/.config/systemd/user$ journalctl --user -u mylog.service -n 4 --no-pager -o cat
/usr/bin/python3: can't open file '/home/stepan/auto-lab/my': [Errno 2] No such file or directory
mylog.service: Main process exited, code=exited, status=2/INVALIDARGUMENT
mylog.service: Failed with result 'exit-code'.
Failed to start mylog.service - Мой подсчёт уровней в журнале.
stepan@systemd-lab:~/.config/systemd/user$ cp /tmp/mylog.service.good mylog.service && systemctl --user daemon-reload && systemctl --user reset-failed mylog.service
```

Убрал кавычки. `verify` по-прежнему пишет, что ошибок нет: для него это просто команда с большим
числом аргументов. А запуск падает: python3 получил `/home/stepan/auto-lab/my` как имя скрипта.
Служба в `failed`, причина в журнале. Вернул файл с кавычками и сбросил `failed`.

## Таймер раз в минуту

```text
stepan@systemd-lab:~/.config/systemd/user$ systemctl --user enable --now mylog.timer
Created symlink /home/stepan/.config/systemd/user/timers.target.wants/mylog.timer → /home/stepan/.config/systemd/user/mylog.timer.
stepan@systemd-lab:~/.config/systemd/user$ journalctl --user -u mylog.service --since '-40s' --no-pager -o short
Nov 26 13:37:00 systemd-lab systemd[115]: Starting mylog.service - Мой подсчёт уровней в журнале...
Nov 26 13:37:00 systemd-lab python3[894]: ERROR    3   37.5%
Nov 26 13:37:00 systemd-lab python3[894]: INFO     3   37.5%
Nov 26 13:37:00 systemd-lab python3[894]: WARN     2   25.0%
Nov 26 13:37:00 systemd-lab python3[894]: всего строк: 8
Nov 26 13:37:00 systemd-lab python3[894]: отчёт сохранён: /home/stepan/auto-lab/my lab/report.json
Nov 26 13:37:00 systemd-lab systemd[115]: Finished mylog.service - Мой подсчёт уровней в журнале.
stepan@systemd-lab:~/.config/systemd/user$ systemctl --user list-timers mylog.timer --no-pager
NEXT                        LEFT LAST                        PASSED UNIT        ACTIVATES
Thu 2026-11-26 13:38:00 MSK  57s Thu 2026-11-26 13:37:00 MSK 1s ago mylog.timer mylog.service

1 timers listed.
Pass --all to see loaded but inactive timers, too.
```

## Каждый день в 3:00

```text
stepan@systemd-lab:~/.config/systemd/user$ systemd-analyze calendar '*-*-* 03:00:00'
Normalized form: *-*-* 03:00:00
    Next elapse: Fri 2026-11-27 03:00:00 MSK
       (in UTC): Fri 2026-11-27 00:00:00 UTC
       From now: 13h left
stepan@systemd-lab:~/.config/systemd/user$ sed -i -e 's/^OnCalendar=.*/OnCalendar=*-*-* 03:00:00/' -e '/^AccuracySec/d' -e '/^Description/s/раз в минуту/каждый день в 3:00/' mylog.timer && cat mylog.timer
# Раз в минуту, чтобы проверить быстро. Рабочий вариант, каждый день в 3:00, в session.md.
[Unit]
Description=Запускать мой подсчёт каждый день в 3:00

[Timer]
OnCalendar=*-*-* 03:00:00
Persistent=true

[Install]
WantedBy=timers.target
stepan@systemd-lab:~/.config/systemd/user$ systemd-analyze --user verify mylog.timer && systemctl --user daemon-reload
stepan@systemd-lab:~/.config/systemd/user$ systemctl --user list-timers mylog.timer --no-pager
NEXT                        LEFT LAST                        PASSED UNIT        ACTIVATES
Fri 2026-11-27 03:00:00 MSK  13h Thu 2026-11-26 13:37:00 MSK 1s ago mylog.timer mylog.service

1 timers listed.
Pass --all to see loaded but inactive timers, too.
```

Строка расписания меняется с `*:*:00` на `*-*-* 03:00:00`. `AccuracySec=1s` убрал: ночному запуску
сдвиг на минуту не мешает. После `daemon-reload` `NEXT` стал завтра в 03:00, а `LAST` остался от
последнего запуска.

## Уборка

```text
stepan@systemd-lab:~/.config/systemd/user$ systemctl --user disable --now mylog.timer
Removed "/home/stepan/.config/systemd/user/timers.target.wants/mylog.timer".
stepan@systemd-lab:~/.config/systemd/user$ rm mylog.service mylog.timer && systemctl --user daemon-reload
stepan@systemd-lab:~/.config/systemd/user$ systemctl --user list-timers --all --no-pager
NEXT LEFT LAST PASSED UNIT ACTIVATES

0 timers listed.
```

## Вывод

Расписание раз в минуту — только для проверки, рабочее `*-*-* 03:00:00`: ночью машина свободна, и
запуск один раз в сутки. `--user` нужен, чтобы работать в своей сессии без root и не трогать
системные службы. Service описывает, что запустить, timer — когда. Результат ищу в двух местах:
вывод и ошибки в `journalctl --user -u mylog.service`, сам отчёт в `~/auto-lab/my lab/report.json`.
