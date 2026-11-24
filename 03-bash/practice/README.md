# Практики занятия 3

На слайдах четыре практики по ходу лекции, финальный сценарий `count_txt.sh` и две практики
в частях про SSH и tmux. Решал все семь. В каждой папке `session.md`: команды, настоящий
вывод и пара фраз о том, что понял.

Всё запускалось в контейнере с Ubuntu 24.04 от обычного пользователя. Из папки `03-bash`:

```bash
docker run --rm -it --hostname lab -v "${PWD}:/work" -w /work ubuntu:24.04 bash
useradd -m -s /bin/bash stepan
```

`--hostname lab` нужен только для того, чтобы приглашение было как в записях,
`stepan@lab`. Короткие команды для повтора (у практик 4 и 5) запускаются прямо в `/work`,
то есть в папке `03-bash`. Для ручного повтора сеансов дальше `su - stepan`. Чего не хватает
в чистом образе, указано у каждой практики (`apt install` делается от root, до `su`).

## Практика 1. Карта терминала

Без мыши узнать каталог, пользователя и систему (`pwd`, `whoami`, `uname -s`), посмотреть
справку тремя способами (`ls --help`, `help cd`, `man pwd`), выполнить `echo` и `history`,
повторить команду стрелкой и через `!!`, очистить экран `clear` и `Ctrl+L`, выйти через
`exit` или `Ctrl+D`. В конце объяснить буквы в `pwd` и `ls -lah`.

Решение: [01-terminal-map/session.md](01-terminal-map/session.md). Для `man` в чистом
образе нужно `unminimize`, потом `apt install man-db`.

## Практика 2. Навигация и раскрытие имён

Походить по путям (`cd`, `cd -`, `pwd`), проверить шаблоны `*`, `?`, `{a,b}`, подстановки
`$HOME`, `$(...)`, `$((...))`, разницу кавычек, форматы `printf`, `\n` в `echo` и `printf`,
`--` для имён с дефисом. Разобрать по словам команду
`printf '%-10s %5s\n' "$USER" "$(date +%s)"` до нажатия Enter.

Решение: [02-paths-expansion/session.md](02-paths-expansion/session.md). Ставить ничего не
нужно.

## Практика 3. Мини-проект и права

В `~/bash-lab/course` создать `src`, `docs`, `backup` и файлы, скопировать `src` в
`backup/src-v1`, переименовать `util.c`, вывести файлы через `find`, сделать `docs/setup.sh`
исполняемым для владельца, удалить копию через `rm -i`, запретить и вернуть запись в
`main.c`.

Решение: [03-mini-project/session.md](03-mini-project/session.md). Ставить ничего не нужно.

## Практика 4. Анализ журнала

Собрать по звеньям конвейер над `app.log`: `grep ERROR`, `cut -d" " -f1`, `sort`,
`uniq -c`, `sort -nr`, `wc -l`, и посмотреть, как меняется результат. Заодно проверить
`tee`, `2>/dev/null`, `2>&1` и `pipefail`.

Решение: [04-log-analysis/session.md](04-log-analysis/session.md), журнал
[04-log-analysis/app.log](04-log-analysis/app.log). Повторить:

```bash
cd practice/04-log-analysis
cut -d' ' -f1 app.log | sort | uniq -c | sort -nr
```

## Практика 5. Сценарий count_txt.sh

Дописать заготовку со слайда: сценарий принимает каталог, печатает имена `*.txt` и их число,
без аргументов выводит `Usage` и выходит с ошибкой.

Решение: [05-count-txt/count_txt.sh](05-count-txt/count_txt.sh), запуск и проверки в
[05-count-txt/session.md](05-count-txt/session.md).

```bash
mkdir -p /tmp/txt-demo && touch /tmp/txt-demo/a.txt "/tmp/txt-demo/b c.txt"
bash practice/05-count-txt/count_txt.sh /tmp/txt-demo
```

## Практика 6. SSH

Подключиться, выполнить `whoami` и `hostname`, создать файл одной командой, скопировать файл
на сервер и обратно. Сервер я поднял в том же контейнере на порту 2222, подробности в
[06-ssh/session.md](06-ssh/session.md). Нужно
`apt install openssh-server rsync iproute2 curl python3`.

## Практика 7. tmux: восстановите сессию

Создать сессию `bash-lab` с окнами `work` и `logs`, переименовать окно, разделить панель,
в одной запустить `sleep 1000`, в другой `date`, отсоединиться, вернуться, отправить
команду через `send-keys`, удалить сессию.

Решение: [07-tmux/session.md](07-tmux/session.md), конфиг [07-tmux/tmux.conf](07-tmux/tmux.conf)
(кладётся в `~/.tmux.conf`). Нужно `apt install tmux`.
