# Практика 3. Служба и таймер

Ubuntu 24.04 в контейнере с systemd 255 в роли PID 1, пользователь `stepan`, Python 3.12.3. Почему контейнер, а не WSL, написано в [README практик](../README.md). Работаю в `~/auto-lab/Chapter11`, юниты из архива. В `python/` уже лежат мой `app.log` и `logsum.py` с процентами из второй практики, поэтому в журнале проценты.

## Установка юнитов

```text
stepan@systemd-lab:~/auto-lab/Chapter11$ ps -p 1 -o comm=
systemd
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user is-system-running
running
stepan@systemd-lab:~/auto-lab/Chapter11$ ./systemd/install-units.sh
bash: ./systemd/install-units.sh: Permission denied
stepan@systemd-lab:~/auto-lab/Chapter11$ sh systemd/install-units.sh
systemd/install-units.sh: 8: set: Illegal option -
stepan@systemd-lab:~/auto-lab/Chapter11$ sed -i 's/\r$//' systemd/*.sh systemd/*.service systemd/*.timer && chmod +x systemd/*.sh
stepan@systemd-lab:~/auto-lab/Chapter11$ ./systemd/install-units.sh
каталог проекта: /home/stepan/auto-lab/Chapter11
установлен: /home/stepan/.config/systemd/user/logsum.service
установлен: /home/stepan/.config/systemd/user/logsum.timer
установлен: /home/stepan/.config/systemd/user/demo.service

готово. Дальше:
  systemctl --user start logsum.service     # запустить один раз
  systemctl --user enable --now logsum.timer # каждую минуту
  systemctl --user status logsum.timer
  journalctl --user -u logsum.service -n 20
stepan@systemd-lab:~/auto-lab/Chapter11$ ls ~/.config/systemd/user
demo.service
logsum.service
logsum.timer
stepan@systemd-lab:~/auto-lab/Chapter11$ grep -E 'WorkingDirectory|ExecStart' ~/.config/systemd/user/logsum.service
# Пути в ExecStart можно писать относительно WorkingDirectory — так короче.
# в кавычки — иначе systemd разрежет путь на части. В WorkingDirectory
WorkingDirectory=/home/stepan/auto-lab/Chapter11
ExecStart=/usr/bin/python3 python/logsum.py python/app.log
stepan@systemd-lab:~/auto-lab/Chapter11$ systemd-analyze --user verify ~/.config/systemd/user/logsum.service ~/.config/systemd/user/logsum.timer && echo 'ошибок нет'
ошибок нет
```

Скрипт из архива сначала не запустился: нет бита выполнения, а через `sh` мешает `\r` в конце
строк. После `sed` и `chmod` `install-units.sh` подставил вместо `@PROJECT@` путь к каталогу и
положил три юнита в `~/.config/systemd/user`, потом сделал `daemon-reload`. Путь в `ExecStart`
относительный, от `WorkingDirectory`.

## Один запуск

```text
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user start logsum.service
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user status logsum.service --no-pager
○ logsum.service - Подсчёт уровней в журнале (учебный пример)
     Loaded: loaded (/home/stepan/.config/systemd/user/logsum.service; static)
     Active: inactive (dead)
       Docs: file:///home/stepan/auto-lab/Chapter11/README.md

Nov 26 13:34:17 systemd-lab systemd[115]: Starting logsum.service - Подсчёт уровней в журнале (учебный пример)...
Nov 26 13:34:17 systemd-lab python3[680]: ERROR    3   37.5%
Nov 26 13:34:17 systemd-lab python3[680]: INFO     3   37.5%
Nov 26 13:34:17 systemd-lab python3[680]: WARN     2   25.0%
Nov 26 13:34:17 systemd-lab python3[680]: всего строк: 8
Nov 26 13:34:17 systemd-lab systemd[115]: Finished logsum.service - Подсчёт уровней в журнале (учебный пример).
stepan@systemd-lab:~/auto-lab/Chapter11$ journalctl --user -u logsum.service -n 20 --no-pager -o short
Nov 26 13:34:17 systemd-lab systemd[115]: Starting logsum.service - Подсчёт уровней в журнале (учебный пример)...
Nov 26 13:34:17 systemd-lab python3[680]: ERROR    3   37.5%
Nov 26 13:34:17 systemd-lab python3[680]: INFO     3   37.5%
Nov 26 13:34:17 systemd-lab python3[680]: WARN     2   25.0%
Nov 26 13:34:17 systemd-lab python3[680]: всего строк: 8
Nov 26 13:34:17 systemd-lab systemd[115]: Finished logsum.service - Подсчёт уровней в журнале (учебный пример).
```

`inactive (dead)` для oneshot нормально: служба посчитала и вышла. В журнале строки с меткой
`python3[PID]` — это вывод скрипта, а `Starting` и `Finished` пишет сам systemd.

## Таймер

```text
stepan@systemd-lab:~/auto-lab/Chapter11$ systemd-analyze calendar '*:*:00' --iterations=2
  Original form: *:*:00
Normalized form: *-*-* *:*:00
    Next elapse: Thu 2026-11-26 13:35:00 MSK
       (in UTC): Thu 2026-11-26 10:35:00 UTC
       From now: 42s left
   Iteration #2: Thu 2026-11-26 13:36:00 MSK
       (in UTC): Thu 2026-11-26 10:36:00 UTC
       From now: 1min 42s left
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user enable --now logsum.timer
Created symlink /home/stepan/.config/systemd/user/timers.target.wants/logsum.timer → /home/stepan/.config/systemd/user/logsum.timer.
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user list-timers --no-pager
NEXT                        LEFT LAST PASSED UNIT         ACTIVATES
Thu 2026-11-26 13:35:00 MSK  42s -         - logsum.timer logsum.service

1 timers listed.
Pass --all to see loaded but inactive timers, too.
stepan@systemd-lab:~/auto-lab/Chapter11$ date '+%H:%M:%S'
13:35:02
stepan@systemd-lab:~/auto-lab/Chapter11$ journalctl --user -u logsum.service --since '-2min' --no-pager -o short
Nov 26 13:34:17 systemd-lab systemd[115]: Starting logsum.service - Подсчёт уровней в журнале (учебный пример)...
Nov 26 13:34:17 systemd-lab python3[680]: ERROR    3   37.5%
Nov 26 13:34:17 systemd-lab python3[680]: INFO     3   37.5%
Nov 26 13:34:17 systemd-lab python3[680]: WARN     2   25.0%
Nov 26 13:34:17 systemd-lab python3[680]: всего строк: 8
Nov 26 13:34:17 systemd-lab systemd[115]: Finished logsum.service - Подсчёт уровней в журнале (учебный пример).
Nov 26 13:35:00 systemd-lab systemd[115]: Starting logsum.service - Подсчёт уровней в журнале (учебный пример)...
Nov 26 13:35:00 systemd-lab python3[696]: ERROR    3   37.5%
Nov 26 13:35:00 systemd-lab python3[696]: INFO     3   37.5%
Nov 26 13:35:00 systemd-lab python3[696]: WARN     2   25.0%
Nov 26 13:35:00 systemd-lab python3[696]: всего строк: 8
Nov 26 13:35:00 systemd-lab systemd[115]: Finished logsum.service - Подсчёт уровней в журнале (учебный пример).
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user list-timers --no-pager
NEXT                        LEFT LAST                        PASSED UNIT         ACTIVATES
Thu 2026-11-26 13:36:00 MSK  57s Thu 2026-11-26 13:35:00 MSK 2s ago logsum.timer logsum.service

1 timers listed.
Pass --all to see loaded but inactive timers, too.
```

`list-timers` показал следующий запуск через 42 секунды, в 13:35:00. Подождал: в журнале новый
запуск ровно в `13:35:00`, благодаря `AccuracySec=1s`. После него `LAST` заполнился, а `NEXT`
сдвинулся на минуту.

## Долгая служба demo.service

```text
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user start demo.service
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user status demo.service --no-pager
● demo.service - Учебный долгоживущий сервис
     Loaded: loaded (/home/stepan/.config/systemd/user/demo.service; disabled; preset: enabled)
     Active: active (running) since Thu 2026-11-26 13:35:02 MSK; 20ms ago
   Main PID: 702 (sleep)
      Tasks: 1 (limit: 9336)
     Memory: 252.0K (peak: 256.0K)
        CPU: 2ms
     CGroup: /docker/fd99bee530b6c84716898708c12382ad6ad6c6589520490e89d7b2706db75b5b/user.slice/user-1002.slice/user@1002.service/app.slice/demo.service
             └─702 /usr/bin/sleep infinity

Nov 26 13:35:02 systemd-lab systemd[115]: Started demo.service - Учебный долгоживущий сервис.
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user stop demo.service; systemctl --user is-active demo.service
inactive
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user is-enabled logsum.timer logsum.service demo.service
enabled
static
disabled
```

`demo.service` — `Type=simple` с `sleep infinity`: висит в `active (running)`, пока не остановишь.
`is-enabled` показывает разницу: `logsum.timer` включён (`enable`), `logsum.service` — `static`,
его запускает таймер, своей секции `[Install]` у него нет, `demo.service` выключен, хотя только что
работал. `start` его запустил, но в автозапуск не добавил.

## Уборка

```text
stepan@systemd-lab:~/auto-lab/Chapter11$ ./systemd/uninstall-units.sh
юниты удалены, таймер выключен
stepan@systemd-lab:~/auto-lab/Chapter11$ systemctl --user list-timers --all --no-pager
NEXT LEFT LAST PASSED UNIT ACTIVATES

0 timers listed.
stepan@systemd-lab:~/auto-lab/Chapter11$ ls ~/.config/systemd/user
```

`uninstall-units.sh` выключил таймер, остановил службы и удалил файлы. Таймеров не осталось,
каталог юнитов пустой.

## Вывод

`start` — запустить сейчас один раз. `enable` — включить на будущее: для таймера это значит
«завести будильник», для службы с `[Install]` — запускать вместе с сессией. `enable --now` — то и
другое сразу. Вывод программы лежит в журнале: `journalctl --user -u logsum.service`.
