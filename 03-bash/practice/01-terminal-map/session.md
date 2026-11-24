# Практика 1. Карта терминала

Работал в интерактивном Bash под пользователем `stepan` в контейнере с Ubuntu 24.04, имя
узла `lab`. Мышь не нужна была ни разу.

## 1. Каталог, пользователь, система

```text
stepan@lab:~$ pwd
/home/stepan
stepan@lab:~$ whoami
stepan
stepan@lab:~$ uname -s
Linux
stepan@lab:~$ echo $0
bash
stepan@lab:~$ ps -p $$
    PID TTY          TIME CMD
   4542 pts/0    00:00:00 bash
stepan@lab:~$ type cd
cd is a shell builtin
stepan@lab:~$ type -a ls
ls is aliased to `ls --color=auto'
ls is /usr/bin/ls
ls is /bin/ls
```

`ls` у меня сначала псевдоним из `~/.bashrc`, а уже потом файл. Два пути `/usr/bin/ls` и
`/bin/ls` потому, что в Ubuntu `/bin` это ссылка на `/usr/bin`.

## 2. Справка

```text
stepan@lab:~$ ls --help | head -n 8
Usage: ls [OPTION]... [FILE]...
List information about the FILEs (the current directory by default).
Sort entries alphabetically if none of -cftuvSUX nor --sort is specified.

Mandatory arguments to long options are mandatory for short options too.
  -a, --all                  do not ignore entries starting with .
  -A, --almost-all           do not list implied . and ..
      --author               with -l, print the author of each file
stepan@lab:~$ help cd | head -n 5
cd: cd [-L|[-P [-e]] [-@]] [dir]
    Change the shell working directory.

    Change the current directory to DIR.  The default DIR is the value of the
    HOME shell variable. If DIR is "-", it is converted to $OLDPWD.
stepan@lab:~$ man pwd | head -n 10
PWD(1)                           User Commands                          PWD(1)

NAME
       pwd - print name of current/working directory

SYNOPSIS
       pwd [OPTION]...

DESCRIPTION
       Print the full filename of the current working directory.
```

`man` здесь обрезан через `head`, чтобы вывод влез в файл. Интерактивно открывал `man pwd`
и выходил по `q`.

В чистом `ubuntu:24.04` `man` сначала не работал:

```text
# man pwd
This system has been minimized by removing packages and content that are
not required on a system that users do not log into.

To restore this content, including manpages, you can run the 'unminimize'
command. You will still need to ensure the 'man-db' package is installed.
```

Страницы возвращаются в два шага: `unminimize` восстанавливает документацию и убирает эту
заглушку, после чего `man` вообще пропадает (`command not found`), и его надо поставить
отдельно: `apt install man-db`. Вывод выше уже после этого.

## 3–4. echo, history, стрелка вверх и !!

```text
stepan@lab:~$ echo "Hello, Bash!"
Hello, Bash!
stepan@lab:~$ history
    1  pwd
    2  whoami
    3  uname -s
    4  echo $0
    5  ps -p $$
    6  type cd
    7  type -a ls
    8  ls --help | head -n 8
    9  help cd | head -n 5
   10  man pwd | head -n 10
   11  echo "Hello, Bash!"
   12  history
stepan@lab:~$ history
...
   12  history
stepan@lab:~$ !!
history
...
   12  history
```

Вторая `history` набрана стрелкой ↑, третья через `!!`. Bash перед выполнением `!!` печатает
то, во что он раскрылся. Номер 12 так и остался последним: в `~/.bashrc` Ubuntu стоит
`HISTCONTROL=ignoreboth`, и повторы подряд в историю не пишутся.

## 5. Очистка экрана

`clear` и `Ctrl+L` выглядят одинаково, но байты разные. Что печатает `clear`:

```text
stepan@lab:~$ clear | od -c
0000000 033   [   H 033   [   2   J 033   [   3   J
0000013
```

После `Ctrl+L` в записи сеанса было только `ESC[H ESC[2J`: курсор в угол и очистка экрана.
У `clear` есть ещё `ESC[3J`, он стирает и прокрутку.

Закрыл терминал через `Ctrl+D`, Bash напечатал `exit`.

## ls -lah по буквам

```text
  -a, --all                  do not ignore entries starting with .
  -h, --human-readable       with -l and -s, print sizes like 1K 234M 2G etc.
  -l                         use a long listing format
```

`pwd` это print working directory (p и wd). В `ls -lah`: `l` длинный формат, `a` показать
скрытые файлы с точкой, `h` размеры в K/M/G. По справке `-h` работает только вместе с `-l` или
`-s`: в коротком выводе размеров нет.

## Что понял

`type` быстрее всего отвечает на вопрос «что вообще запустится». Справку лучше держать
под рукой локально: в урезанном контейнере `man` может отсутствовать, а `--help` и `help`
есть всегда.
