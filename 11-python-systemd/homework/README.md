# ДЗ 11. Ночной сторож

## Условие

Сделать «ночного сторожа»: раз в сутки он проверяет диск и сайт, пишет отчёт, а если что-то не
так, завершается с ошибкой.

1. Скрипт `nightcheck.py`: размер каталога и проверка URL.
2. Отчёт в `report.json` с датой.
3. Код возврата 1, если что-то не так.
4. Юниты `nightcheck.service` и `nightcheck.timer`.
5. Расписание: каждый день в 3:00, с `Persistent=true`.
6. `report.txt`: как проверить, что всё работает.

Структура отчёта: что делает скрипт, текст юнитов, `systemctl status`, `journalctl --user -u …`,
что было непонятно.

## Критерии

Базовый: скрипт запускается вручную и через таймер, отчёт появляется, журнал показывает запуск.
Сделано: ручной запуск, `systemctl --user start` и срабатывание таймера есть в
[отчёте](REPORT.md), `report.json` пишется в `~/.local/state/nightcheck/`.

Продвинутый: при ошибке служба помечается `failed`, в журнале понятная причина, есть
uninstall-скрипт и инструкция в README. Сделано: остановил сайт, служба ушла в `failed`, в журнале
`не ответил: [Errno 111] Connection refused`; `uninstall.sh` и инструкция ниже.

Сверх условия: адресов можно дать несколько, настройки лежат отдельно от юнита, есть `tests.sh`
на 12 проверок без systemd.

## Что где лежит

| Файл | Что это |
| --- | --- |
| `nightcheck.py` | проверки каталога и адресов, отчёт в JSON, код возврата |
| `nightcheck.service` | oneshot-служба, `@PROJECT@` подставляет `install.sh` |
| `nightcheck.timer` | каждый день в 3:00, `Persistent=true` |
| `nightcheck.env.example` | что проверять: каталог, лимит в МБ, адрес |
| `install.sh` | ставит юниты в `~/.config/systemd/user`, проверяет их и включает таймер |
| `uninstall.sh` | выключает таймер и удаляет юниты, с `--purge` ещё настройки и отчёт |
| `tests.sh` | проверки скрипта: свой сайт на `http.server`, разные исходы и коды |
| `dist.sh` | делает `report.txt` из `REPORT.md` и собирает архив |
| `REPORT.md` | отчёт по структуре со слайда |
| `nightcheck-homework.tar.gz` | архив для сдачи: файлы выше и `report.txt` |

## Как поставить

Нужны Python 3 и systemd в пользовательской сессии (Ubuntu 24.04: Python 3.12, systemd 255).
Root не нужен.

```bash
./tests.sh                      # проверить скрипт без systemd
./install.sh                    # юниты, настройки, таймер включён
nano ~/.config/nightcheck.env   # свой каталог, лимит и адрес
systemctl --user start nightcheck.service
```

Настройки:

```bash
NIGHTCHECK_DIR=/home/stepan/www     # каталог, размер которого смотрим
NIGHTCHECK_MAX_MB=500               # сколько ему можно занимать
NIGHTCHECK_URL=http://127.0.0.1:8000/
```

`install.sh` берёт путь к `nightcheck.py` из того места, где лежит сам, поэтому каталог проекта после
установки не переносить. Перенёс — запустить `install.sh` ещё раз, настройки он не затрёт.

Скрипт можно запускать и руками:

```bash
./nightcheck.py --dir ~/www --max-mb 500 --url http://127.0.0.1:8000/ --report /tmp/report.json
```

Код 0 — всё в порядке, 1 — каталог больше лимита или его нет, адрес ответил 4xx/5xx или не ответил,
отчёт не записался, 2 — неверные аргументы.

## Как проверить, что всё работает

```bash
systemctl --user list-timers nightcheck.timer        # NEXT: завтра в 03:00
systemctl --user status nightcheck.service           # status=0/SUCCESS или failed
journalctl --user -u nightcheck.service -n 20        # строки ok/ПЛОХО и итог
cat ~/.local/state/nightcheck/report.json            # дата и результат последней проверки
systemctl --user list-units --failed                 # здесь сторож, если ночью было плохо
```

Чтобы увидеть срабатывание таймера, не дожидаясь 3:00, можно на время поставить расписание раз в
минуту и потом убрать (так проверял в отчёте):

```bash
mkdir -p ~/.config/systemd/user/nightcheck.timer.d
printf '[Timer]\nOnCalendar=\nOnCalendar=*:*:00\nAccuracySec=1s\n' \
    > ~/.config/systemd/user/nightcheck.timer.d/test.conf
systemctl --user daemon-reload
# через минуту: journalctl --user -u nightcheck.service
rm -r ~/.config/systemd/user/nightcheck.timer.d && systemctl --user daemon-reload
```

## Как убрать

```bash
./uninstall.sh            # таймер выключен, юниты удалены, настройки и отчёт остаются
./uninstall.sh --purge    # удалить и их
```

## Как сделать архив

```bash
./dist.sh
```

`dist.sh` превращает `REPORT.md` в `report.txt` (без markdown, таблицы колонками), кладёт файлы и
отчёт в каталог `nightcheck-homework/` и пакует его в `nightcheck-homework.tar.gz`.
