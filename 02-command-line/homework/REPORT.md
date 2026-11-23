# Отчёт по ДЗ 2. Командная строка

Делал в контейнере Ubuntu 24.04, пользователь `stepan`, оболочка bash, имя машины `devbox`.
Всё в одной интерактивной сессии, поэтому `history` в конце показывает её целиком.

## Шаг 0. Где я

```text
stepan@devbox:~$ whoami
stepan
stepan@devbox:~$ pwd
/home/stepan
stepan@devbox:~$ echo $SHELL
/bin/bash
```

## Шаг 1. Каталог project

```text
stepan@devbox:~$ mkdir ~/project
stepan@devbox:~$ cd ~/project
stepan@devbox:~/project$ pwd
/home/stepan/project
```

## Шаг 2. hello.txt и world.txt

Сразу записал в файлы по строке, чтобы на шаге 3 было что искать. Слово «linux» есть только
в `hello.txt`.

```text
stepan@devbox:~/project$ echo "hello linux" > hello.txt
stepan@devbox:~/project$ echo "world" > world.txt
stepan@devbox:~/project$ ls -la
total 16
drwxrwxr-x 2 stepan stepan 4096 Nov 23 19:05 .
drwxr-x--- 3 stepan stepan 4096 Nov 23 19:04 ..
-rw-rw-r-- 1 stepan stepan   12 Nov 23 19:05 hello.txt
-rw-rw-r-- 1 stepan stepan    6 Nov 23 19:05 world.txt
stepan@devbox:~/project$ cat hello.txt world.txt
hello linux
world
```

## Шаг 3. Поиск слова linux

`find` сам по себе только перечисляет файлы, `grep -rl` ищет по содержимому и печатает имена
файлов. Последняя команда — связка: `find` отбирает обычные файлы, а `grep -l` проверяет
каждый из них.

```text
stepan@devbox:~/project$ find ~/project -type f -name "*.txt"
/home/stepan/project/world.txt
/home/stepan/project/hello.txt
stepan@devbox:~/project$ grep -rl "linux" ~/project
/home/stepan/project/hello.txt
stepan@devbox:~/project$ find ~/project -type f -exec grep -l "linux" {} +
/home/stepan/project/hello.txt
```

## Шаг 4. sleep 100 в фоне, PID, kill

`&` запустил `sleep` в фоне, bash сразу показал номер задания `[1]` и PID 37. Проверил PID
ещё двумя способами (`jobs -l`, `pgrep -a`) и посмотрел состояние через `ps`: `S` значит,
что процесс спит. Завершил обычным `kill`, то есть SIGTERM, `-9` не понадобился.

```text
stepan@devbox:~/project$ sleep 100 &
[1] 37
stepan@devbox:~/project$ jobs -l
[1]+    37 Running                 sleep 100 &
stepan@devbox:~/project$ pgrep -a sleep
37 sleep 100
stepan@devbox:~/project$ ps -o pid,stat,cmd -p 37
    PID STAT CMD
     37 S    sleep 100
stepan@devbox:~/project$ kill 37
stepan@devbox:~/project$ jobs
[1]+  Terminated              sleep 100
stepan@devbox:~/project$ pgrep -a sleep || echo no sleep
no sleep
```

## Шаг 5. Строка в hello.txt через nano

```bash
nano hello.txt
```

Нажимал: Alt+/ (в конец файла, на пустую строку после `hello linux`), набрал `Hello, Linux!`,
Ctrl+O, Enter (подтвердить имя файла), Ctrl+X. Результат и повторный поиск:

```text
stepan@devbox:~/project$ cat hello.txt
hello linux
Hello, Linux!
stepan@devbox:~/project$ grep -n linux hello.txt
1:hello linux
stepan@devbox:~/project$ grep -in linux hello.txt
1:hello linux
2:Hello, Linux!
```

## Шаг 6. Копирование hello.txt в world.txt через vi

```bash
vi hello.txt
```

Нажимал: `gg` (в начало), `yG` (скопировать до конца файла, вместе это `ggyG`), `:e world.txt`
и Enter (открыть второй файл, скопированное при этом не теряется), `G` (на последнюю строку),
`p` (вставить после неё), `:wq` и Enter. Результат:

```text
stepan@devbox:~/project$ cat world.txt
world
hello linux
Hello, Linux!
```

## Итог: ls -la project и history

```text
stepan@devbox:~/project$ cd ~
stepan@devbox:~$ ls -la project
total 16
drwxrwxr-x 2 stepan stepan 4096 Nov 23 19:11 .
drwxr-x--- 4 stepan stepan 4096 Nov 23 19:09 ..
-rw-rw-r-- 1 stepan stepan   26 Nov 23 19:09 hello.txt
-rw-rw-r-- 1 stepan stepan   32 Nov 23 19:11 world.txt
stepan@devbox:~$ ls -la project > ls-la-project.txt
stepan@devbox:~$ history
...
stepan@devbox:~$ history > history.txt
```

`hello.txt` вырос с 12 до 26 байт (`Hello, Linux!` и перевод строки — 14 байт), `world.txt` —
с 6 до 32 (6 + 26). Полная история лежит в [history.txt](history.txt): 31 команда, последняя —
сама `history > history.txt`. В [ls-la-project.txt](ls-la-project.txt) у `..` время 19:13, а не
19:09: оболочка создаёт файл для `>` до запуска `ls`, и время домашнего каталога обновилось.

## Что было непонятно

- Шаг 3 я делал до nano, поэтому нашлась только `hello linux`. После nano проверил:
  `grep -n linux hello.txt` строку `Hello, Linux!` не находит, grep различает регистр.
  Без учёта регистра ищет `grep -in`.
- `p` в vi вставляет после текущей строки. У меня в `world.txt` уже была строка `world`,
  поэтому копия встала под ней. Потом проверил на пустом файле: там первой строкой остаётся
  пустая. `P` вставляет перед текущей строкой, но тогда пустая строка остаётся в конце.
- `man ls` в контейнере не открылся: образ Ubuntu минимизирован, man-страниц нет, предлагают
  `unminimize`. Обходился `ls --help` и `help cd`.
- `vi` в Ubuntu 24.04 — это `vim.tiny`, полного vim нет. `ggyG`, `:e` и `p` в нём работают.
