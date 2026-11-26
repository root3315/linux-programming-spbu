# Отчёт по ДЗ 11: ночной сторож

Проверял в Ubuntu 24.04: Python 3.12.3, systemd 255. В WSL systemd у меня выключен, поэтому
запускал в контейнере, где PID 1 — systemd, а у пользователя `stepan` своя сессия
(`systemctl --user is-system-running` даёт `running`). Сайт для проверки — `python3 -m http.server`
на `127.0.0.1:8000`, запущенный тоже через systemd (`systemd-run --user --unit=site`), чтобы его
можно было остановить и посмотреть, как сторож это заметит. Проверяемый каталог `~/www`, 4.8 МБ.

## 1. Что делает скрипт

`nightcheck.py` делает две проверки и пишет отчёт:

- каталог: суммирует размеры файлов через `os.walk` (символьные ссылки не считает) и сравнивает с
  лимитом `--max-mb`; каталога нет — тоже проблема;
- адрес: `urlopen` с таймаутом 5 секунд, хорошо — код 2xx или 3xx, плохо — 4xx/5xx (`HTTPError`)
  или «не ответил» (`URLError`, таймаут). `--url` можно дать несколько раз.

Отчёт — JSON с датой и временем (`datetime.now().astimezone()`, с часовым поясом), общим `ok` и
списком проверок. Пишется сначала во временный файл, потом подменяет старый, так что недописанного
отчёта не бывает. Каждая проверка печатается одной строкой: `ok` в stdout, `ПЛОХО` с причиной в
stderr. Обе попадают в журнал. Код возврата: 0 — всё хорошо, 1 — хоть одна проверка плохая или
отчёт не записался, 2 — ошибка в аргументах (это argparse).

Что проверять, служба берёт из `~/.config/nightcheck.env`, поэтому юнит одинаковый на любой машине.

Тесты скрипта без systemd: свой сайт на свободном порту, каталог на 3 МБ и все исходы.

```text
$ ./tests.sh
ok    всё в порядке (код 0)
ok    отчёт: ok, дата, две проверки
ok    отчёт: 3.0 МБ и код 200
ok    каталог больше лимита (код 1)
ok    отчёт: причина про лимит
ok    нет каталога (код 1)
ok    сайт ответил 404 (код 1)
ok    отчёт: код 404
ok    сайт не отвечает (код 1)
ok    отчёт: причина «не ответил»
ok    один адрес из двух плохой (код 1)
ok    нет обязательного --url (код 2)
пройдено: 12, упало: 0
```

Ручной запуск, хороший и плохой:

```text
$ ./nightcheck.py --dir ~/www --max-mb 500 --url http://127.0.0.1:8000/ --report /tmp/report.json; echo "код=$?"
ok     /home/stepan/www: 4.8 МБ из 500
ok     http://127.0.0.1:8000/: 200 за 29 мс
итог: всё в порядке, отчёт: /tmp/report.json
код=0
$ cat /tmp/report.json
{
  "date": "2026-11-26T13:38:20+03:00",
  "ok": true,
  "checks": [
    {
      "check": "dir",
      "path": "/home/stepan/www",
      "max_mb": 500.0,
      "size_mb": 4.8,
      "ok": true
    },
    {
      "check": "url",
      "url": "http://127.0.0.1:8000/",
      "status": 200,
      "ms": 29,
      "ok": true
    }
  ]
}
```

```text
$ ./nightcheck.py --dir ~/www --max-mb 1 --url http://127.0.0.1:8000/ --url http://127.0.0.1:8000/nope --report /tmp/report.json; echo "код=$?"
ПЛОХО  /home/stepan/www: занято 4.8 МБ при лимите 1 МБ
ok     http://127.0.0.1:8000/: 200 за 32 мс
ПЛОХО  http://127.0.0.1:8000/nope: ответил кодом 404
итог: проблем: 2, отчёт: /tmp/report.json
код=1
```

### Как проверить, что всё работает

1. `./tests.sh` — 12 проверок, все `ok`.
2. `./install.sh`, потом `systemctl --user list-timers nightcheck.timer`: следующий запуск завтра
   в 03:00.
3. `systemctl --user start nightcheck.service`, затем `status` (`status=0/SUCCESS`) и
   `journalctl --user -u nightcheck.service -n 20` (строки `ok` и итог).
4. `cat ~/.local/state/nightcheck/report.json`: дата — время этого запуска.
5. Утром после ночного запуска: `list-timers` (в `LAST` сегодня 03:00), журнал за ночь
   `journalctl --user -u nightcheck.service --since today`, `systemctl --user list-units --failed`
   (пусто, если всё хорошо).

## 2. Текст юнитов

`nightcheck.service` в проекте, `@PROJECT@` заменяет `install.sh`:

```ini
# Один ночной обход: проверить диск и сайт, записать отчёт и выйти.
# @PROJECT@ при установке заменяется на каталог с nightcheck.py (install.sh).
[Unit]
Description=Ночная проверка диска и сайта

[Service]
Type=oneshot
# Что проверять, лежит в отдельном файле: юнит не надо править под каждую машину.
EnvironmentFile=%h/.config/nightcheck.env
# Создаёт ~/.local/state/nightcheck, %S в пользовательском юните и есть ~/.local/state.
StateDirectory=nightcheck
# Путь к скрипту в кавычках: в нём может быть пробел. ${VAR} в фигурных
# скобках systemd подставляет одним аргументом, даже если в значении пробел.
ExecStart=/usr/bin/python3 "@PROJECT@/nightcheck.py" --dir ${NIGHTCHECK_DIR} --max-mb ${NIGHTCHECK_MAX_MB} --url ${NIGHTCHECK_URL} --report %S/nightcheck/report.json
```

`nightcheck.timer`:

```ini
# Будильник для nightcheck.service: каждый день в 3:00.
[Unit]
Description=Ночная проверка диска и сайта, каждый день в 3:00

[Timer]
OnCalendar=*-*-* 03:00:00
# Машина была выключена в 3:00: проверка пройдёт сразу после включения.
Persistent=true

[Install]
WantedBy=timers.target
```

Установка и то, что получилось в `~/.config/systemd/user`:

```text
$ ./install.sh
установлен: /home/stepan/.config/systemd/user/nightcheck.service
установлен: /home/stepan/.config/systemd/user/nightcheck.timer
настройки: /home/stepan/.config/nightcheck.env (поправьте каталог и адрес)
Created symlink /home/stepan/.config/systemd/user/timers.target.wants/nightcheck.timer → /home/stepan/.config/systemd/user/nightcheck.timer.
NEXT                        LEFT LAST PASSED UNIT             ACTIVATES
Fri 2026-11-27 03:00:00 MSK  13h -         - nightcheck.timer nightcheck.service

1 timers listed.
Pass --all to see loaded but inactive timers, too.
```

```text
$ grep -v '^#' ~/.config/systemd/user/nightcheck.service
[Unit]
Description=Ночная проверка диска и сайта

[Service]
Type=oneshot
EnvironmentFile=%h/.config/nightcheck.env
StateDirectory=nightcheck
ExecStart=/usr/bin/python3 "/home/stepan/linux-programming-spbu/11-python-systemd/homework/nightcheck.py" --dir ${NIGHTCHECK_DIR} --max-mb ${NIGHTCHECK_MAX_MB} --url ${NIGHTCHECK_URL} --report %S/nightcheck/report.json
```

`Type=oneshot`: проверил и вышел. `EnvironmentFile` читает настройки, `${NIGHTCHECK_DIR}` в фигурных
скобках подставляется одним аргументом. `StateDirectory=nightcheck` создаёт
`~/.local/state/nightcheck`, а `%S` в пользовательском юните — это `~/.local/state`. Путь к скрипту
в кавычках на случай пробела в пути к проекту. У службы нет `[Install]`: её запускает таймер,
поэтому `is-enabled` у неё `static`. `Persistent=true`: если в 3:00 машина была выключена, проверка
пройдёт сразу после включения. Время последнего запуска systemd хранит в
`~/.local/share/systemd/timers/stamp-nightcheck.timer` (виден в разделе 4).

## 3. systemctl status

Ручной запуск через systemd, всё в порядке:

```text
$ systemctl --user start nightcheck.service; echo "код=$?"
код=0
$ systemctl --user status nightcheck.service --no-pager
○ nightcheck.service - Ночная проверка диска и сайта
     Loaded: loaded (/home/stepan/.config/systemd/user/nightcheck.service; static)
     Active: inactive (dead) since Thu 2026-11-26 13:38:21 MSK; 20ms ago
TriggeredBy: ● nightcheck.timer
    Process: 1166 ExecStart=/usr/bin/python3 /home/stepan/linux-programming-spbu/11-python-systemd/homework/nightcheck.py --dir ${NIGHTCHECK_DIR} --max-mb ${NIGHTCHECK_MAX_MB} --url ${NIGHTCHECK_URL} --report /home/stepan/.local/state/nightcheck/report.json (code=exited, status=0/SUCCESS)
   Main PID: 1166 (code=exited, status=0/SUCCESS)
        CPU: 150ms

Nov 26 13:38:21 systemd-lab systemd[115]: Starting nightcheck.service - Ночная проверка диска и сайта...
Nov 26 13:38:21 systemd-lab python3[1166]: ok     /home/stepan/www: 4.8 МБ из 500
Nov 26 13:38:21 systemd-lab python3[1166]: ok     http://127.0.0.1:8000/: 200 за 31 мс
Nov 26 13:38:21 systemd-lab python3[1166]: итог: всё в порядке, отчёт: /home/stepan/.local/state/nightcheck/report.json
Nov 26 13:38:21 systemd-lab systemd[115]: Finished nightcheck.service - Ночная проверка диска и сайта.
```

`inactive (dead)` для oneshot нормально, успех виден по `status=0/SUCCESS`. В строке `Process`
systemd показывает `${NIGHTCHECK_DIR}` как есть, без подстановки, а `%S` уже раскрыт.

Остановил сайт и запустил ещё раз:

```text
$ systemctl --user stop site.service
$ systemctl --user start nightcheck.service; echo "код=$?"
Job for nightcheck.service failed because the control process exited with error code.
See "systemctl --user status nightcheck.service" and "journalctl --user -xeu nightcheck.service" for details.
код=1
$ systemctl --user status nightcheck.service --no-pager
× nightcheck.service - Ночная проверка диска и сайта
     Loaded: loaded (/home/stepan/.config/systemd/user/nightcheck.service; static)
     Active: failed (Result: exit-code) since Thu 2026-11-26 13:39:02 MSK; 14ms ago
TriggeredBy: ● nightcheck.timer
    Process: 1197 ExecStart=/usr/bin/python3 /home/stepan/linux-programming-spbu/11-python-systemd/homework/nightcheck.py --dir ${NIGHTCHECK_DIR} --max-mb ${NIGHTCHECK_MAX_MB} --url ${NIGHTCHECK_URL} --report /home/stepan/.local/state/nightcheck/report.json (code=exited, status=1/FAILURE)
   Main PID: 1197 (code=exited, status=1/FAILURE)
        CPU: 115ms

Nov 26 13:39:02 systemd-lab systemd[115]: Starting nightcheck.service - Ночная проверка диска и сайта...
Nov 26 13:39:02 systemd-lab python3[1197]: ok     /home/stepan/www: 4.8 МБ из 500
Nov 26 13:39:02 systemd-lab python3[1197]: ПЛОХО  http://127.0.0.1:8000/: не ответил: [Errno 111] Connection refused
Nov 26 13:39:02 systemd-lab python3[1197]: итог: проблем: 1, отчёт: /home/stepan/.local/state/nightcheck/report.json
Nov 26 13:39:02 systemd-lab systemd[115]: nightcheck.service: Main process exited, code=exited, status=1/FAILURE
Nov 26 13:39:02 systemd-lab systemd[115]: nightcheck.service: Failed with result 'exit-code'.
Nov 26 13:39:02 systemd-lab systemd[115]: Failed to start nightcheck.service - Ночная проверка диска и сайта.
```

```text
$ systemctl --user list-units --failed --no-pager
  UNIT               LOAD   ACTIVE SUB    DESCRIPTION
● nightcheck.service loaded failed failed Ночная проверка диска и сайта

Legend: LOAD   → Reflects whether the unit definition was properly loaded.
        ACTIVE → The high-level unit activation state, i.e. generalization of SUB.
        SUB    → The low-level unit activation state, values depend on unit type.

1 loaded units listed.
```

После того как сайт снова поднят, следующий запуск снимает `failed`:

```text
$ systemctl --user start nightcheck.service && systemctl --user is-failed nightcheck.service
inactive
```

## 4. journalctl --user -u nightcheck.service

Отчёт после ручного запуска:

```text
$ cat ~/.local/state/nightcheck/report.json
{
  "date": "2026-11-26T13:38:21+03:00",
  "ok": true,
  "checks": [
    {
      "check": "dir",
      "path": "/home/stepan/www",
      "max_mb": 500.0,
      "size_mb": 4.8,
      "ok": true
    },
    {
      "check": "url",
      "url": "http://127.0.0.1:8000/",
      "status": 200,
      "ms": 31,
      "ok": true
    }
  ]
}
```

Срабатывание таймера. Ждать 3:00 не стал: на время поставил drop-in с расписанием раз в минуту,
дождался запуска и убрал его. Заодно видно файл, по которому работает `Persistent=true`.

```text
$ systemd-analyze calendar '*-*-* 03:00:00'
Normalized form: *-*-* 03:00:00
    Next elapse: Fri 2026-11-27 03:00:00 MSK
       (in UTC): Fri 2026-11-27 00:00:00 UTC
       From now: 13h left
$ mkdir -p ~/.config/systemd/user/nightcheck.timer.d && printf '[Timer]\nOnCalendar=\nOnCalendar=*:*:00\nAccuracySec=1s\n' > ~/.config/systemd/user/nightcheck.timer.d/test.conf && systemctl --user daemon-reload
$ systemctl --user list-timers nightcheck.timer --no-pager
NEXT                        LEFT LAST PASSED UNIT             ACTIVATES
Thu 2026-11-26 13:39:00 MSK  38s -         - nightcheck.timer nightcheck.service

1 timers listed.
Pass --all to see loaded but inactive timers, too.
```

```text
$ journalctl --user -u nightcheck.service --since '-30s' --no-pager -o short
Nov 26 13:39:00 systemd-lab systemd[115]: Starting nightcheck.service - Ночная проверка диска и сайта...
Nov 26 13:39:00 systemd-lab python3[1182]: ok     /home/stepan/www: 4.8 МБ из 500
Nov 26 13:39:00 systemd-lab python3[1182]: ok     http://127.0.0.1:8000/: 200 за 25 мс
Nov 26 13:39:00 systemd-lab python3[1182]: итог: всё в порядке, отчёт: /home/stepan/.local/state/nightcheck/report.json
Nov 26 13:39:00 systemd-lab systemd[115]: Finished nightcheck.service - Ночная проверка диска и сайта.
$ ls ~/.local/share/systemd/timers/
stamp-nightcheck.timer
$ rm -r ~/.config/systemd/user/nightcheck.timer.d && systemctl --user daemon-reload && systemctl --user list-timers nightcheck.timer --no-pager
NEXT                        LEFT LAST                        PASSED UNIT             ACTIVATES
Fri 2026-11-27 03:00:00 MSK  13h Thu 2026-11-26 13:39:00 MSK 2s ago nightcheck.timer nightcheck.service

1 timers listed.
Pass --all to see loaded but inactive timers, too.
```

Журнал при ошибке: строка с причиной от скрипта, дальше systemd пишет, что процесс вышел с кодом 1:

```text
$ journalctl --user -u nightcheck.service -n 8 --no-pager -o short
Nov 26 13:39:00 systemd-lab systemd[115]: Finished nightcheck.service - Ночная проверка диска и сайта.
Nov 26 13:39:02 systemd-lab systemd[115]: Starting nightcheck.service - Ночная проверка диска и сайта...
Nov 26 13:39:02 systemd-lab python3[1197]: ok     /home/stepan/www: 4.8 МБ из 500
Nov 26 13:39:02 systemd-lab python3[1197]: ПЛОХО  http://127.0.0.1:8000/: не ответил: [Errno 111] Connection refused
Nov 26 13:39:02 systemd-lab python3[1197]: итог: проблем: 1, отчёт: /home/stepan/.local/state/nightcheck/report.json
Nov 26 13:39:02 systemd-lab systemd[115]: nightcheck.service: Main process exited, code=exited, status=1/FAILURE
Nov 26 13:39:02 systemd-lab systemd[115]: nightcheck.service: Failed with result 'exit-code'.
Nov 26 13:39:02 systemd-lab systemd[115]: Failed to start nightcheck.service - Ночная проверка диска и сайта.
```

```text
$ python3 -c 'import json; r = json.load(open("/home/stepan/.local/state/nightcheck/report.json")); print(r["date"], r["ok"], r["checks"][1]["error"])'
2026-11-26T13:39:02+03:00 False не ответил: [Errno 111] Connection refused
```

Удаление:

```text
$ ./uninstall.sh
таймер выключен, юниты удалены
$ systemctl --user list-timers --all --no-pager; ls ~/.config/systemd/user; ls ~/.config/nightcheck.env ~/.local/state/nightcheck
NEXT LEFT LAST PASSED UNIT ACTIVATES

0 timers listed.
/home/stepan/.config/nightcheck.env

/home/stepan/.local/state/nightcheck:
report.json
$ ./uninstall.sh --purge; ls ~/.config/nightcheck.env ~/.local/state/nightcheck
таймер выключен, юниты удалены
настройки и отчёт удалены
ls: cannot access '/home/stepan/.config/nightcheck.env': No such file or directory
ls: cannot access '/home/stepan/.local/state/nightcheck': No such file or directory
```

## 5. Что было непонятно

- В архиве к занятию нет `python/app.log`, хотя на него ссылаются и `logsum.py`, и
  `logsum.service`. Пришлось сделать свой журнал. Ещё в архиве CRLF и нет бита выполнения:
  `install-units.sh` не запускался ни как `./`, ни через `sh` (`set: Illegal option -` из-за `\r`).
- `systemd-analyze verify` не находит пути с пробелом без кавычек. Синтаксис правильный, а падает
  только запуск: python3 получает половину пути. Поэтому в юните путь к скрипту в кавычках.
- Сначала в журнале строка `ПЛОХО` стояла раньше строк, которые скрипт напечатал до неё. stdout,
  когда он не терминал, буферизуется, а stderr нет, и в журнал они приходят в разном порядке.
  Помог `flush=True` у `print`.
- Не сразу понял, как проверить таймер на 3:00, не дожидаясь ночи. `systemd-analyze calendar`
  показывает только время, а само срабатывание проверил временным drop-in с `OnCalendar=*:*:00`.
  Пустая строка `OnCalendar=` в нём обязательна: без неё расписание добавляется к старому, а не
  заменяет его.
- Не сразу нашёл, где systemd помнит прошлый запуск для `Persistent=true`: в
  `~/.local/share/systemd/timers/stamp-nightcheck.timer`, файл появляется уже при включении
  таймера. Ручной `start` службы таймер запуском не считает: после него в `list-timers` в колонке
  `LAST` по-прежнему `-`.
