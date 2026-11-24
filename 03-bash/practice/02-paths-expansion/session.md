# Практика 2. Навигация и раскрытие имён

Всё в `~/bash-lab/paths`. Перед каждой командой сначала писал себе, что ожидаю, потом
запускал. Где ожидание не совпало, отмечено ниже. У `ls` выключал цвет (`--color=never`),
чтобы в записи не было управляющих кодов.

## Пути

```text
stepan@lab:~$ mkdir -p ~/bash-lab/paths/{src,docs} && cd ~/bash-lab
stepan@lab:~/bash-lab$ cd paths
stepan@lab:~/bash-lab/paths$ pwd
/home/stepan/bash-lab/paths
stepan@lab:~/bash-lab/paths$ cd -
/home/stepan/bash-lab
stepan@lab:~/bash-lab$ cd paths
stepan@lab:~/bash-lab/paths$ touch alpha.txt beta.txt gamma.log .secret src/main.c docs/readme.txt docs/intro.txt
stepan@lab:~/bash-lab/paths$ ls --color=never
alpha.txt  beta.txt  docs  gamma.log  src
stepan@lab:~/bash-lab/paths$ ls -A --color=never
.secret  alpha.txt  beta.txt  docs  gamma.log  src
```

## Шаблоны и подстановки

```text
stepan@lab:~/bash-lab/paths$ printf '%s\n' *.txt
alpha.txt
beta.txt
stepan@lab:~/bash-lab/paths$ printf '%s\n' ?lpha.txt
alpha.txt
stepan@lab:~/bash-lab/paths$ printf '%s\n' {red,green,blue}.log
red.log
green.log
blue.log
stepan@lab:~/bash-lab/paths$ printf '%s\n' .*
.secret
```

`{red,green,blue}.log` раскрылся в три имени, хотя таких файлов нет: фигурные скобки не
смотрят на диск.

На `.*` я ждал `.`, `..` и `.secret`, как на слайде. Получил только `.secret`. Разобрался:

```text
stepan@lab:~/bash-lab/paths$ shopt globskipdots
globskipdots   	on
stepan@lab:~/bash-lab/paths$ shopt -u globskipdots
stepan@lab:~/bash-lab/paths$ printf '%s\n' .*
.
..
.secret
stepan@lab:~/bash-lab/paths$ shopt -s globskipdots
stepan@lab:~/bash-lab/paths$ sh -c 'printf "%s\n" .*'
.
..
.secret
stepan@lab:~/bash-lab/paths$ readlink -f /bin/sh
/usr/bin/dash
```

В bash 5.2 опция `globskipdots` включена по умолчанию, а в `sh` (это dash) её нет. Слайд
прав для любого сценария на `#!/bin/sh` и для старого Bash.

```text
stepan@lab:~/bash-lab/paths$ printf '%s\n' *.pdf
*.pdf
stepan@lab:~/bash-lab/paths$ printf '%s\n' "$HOME" "$(pwd)" "$((2+3))"
/home/stepan
/home/stepan/bash-lab/paths
5
```

`*.pdf` не совпал ни с чем и остался как есть.

## Кавычки

```text
stepan@lab:~/bash-lab/paths$ name='Linux User'
stepan@lab:~/bash-lab/paths$ printf '<%s>\n' "$name"
<Linux User>
stepan@lab:~/bash-lab/paths$ printf '<%s>\n' $name
<Linux>
<User>
stepan@lab:~/bash-lab/paths$ printf '<%s>\n' '$name'
<$name>
```

## printf и \n

```text
stepan@lab:~/bash-lab/paths$ printf '%-8s %5s %.2f\n' NAME 42 3.14159
NAME        42 3.14
stepan@lab:~/bash-lab/paths$ printf '%05d %x %o %%\n' 42 255 8
00042 ff 10 %
stepan@lab:~/bash-lab/paths$ printf '[%3s]\n' toolong
[toolong]
stepan@lab:~/bash-lab/paths$ printf 'a\nb\n'
a
b
stepan@lab:~/bash-lab/paths$ echo "a\nb"
a\nb
stepan@lab:~/bash-lab/paths$ echo -e "a\nb"
a
b
stepan@lab:~/bash-lab/paths$ printf '%s' $'a\nb' | od -c
0000000   a  \n   b
0000003
stepan@lab:~/bash-lab/paths$ printf '%s' 'a\nb' | od -c
0000000   a   \   n   b
0000004
```

`od -c` показывает разницу: `$'a\nb'` это три байта (Bash сам сделал перевод строки), `'a\nb'`
четыре, и `%s` их не трогает. Ширина `%3s` минимальная, `toolong` не обрезался.

## Опции, экранирование, перенос строки

```text
stepan@lab:~/bash-lab/paths$ touch -- -notes.txt
stepan@lab:~/bash-lab/paths$ ls -notes.txt
ls: invalid option -- 'e'
Try 'ls --help' for more information.
stepan@lab:~/bash-lab/paths$ ls -- -notes.txt
-notes.txt
stepan@lab:~/bash-lab/paths$ printf '%s\n' "\$PATH"
$PATH
stepan@lab:~/bash-lab/paths$ cost=250; printf 'total: %d RUB\n' $((cost * 2))
total: 500 RUB
stepan@lab:~/bash-lab/paths$ for d in src docs; do \
>   printf '%s: %s\n' "$d" "$(ls "$d" | wc -l)"; \
> done
src: 1
docs: 2
```

`ls -notes.txt` разобрал `-n`, `-o`, `-t` как опции и споткнулся на `-e`.

## Разбор команды по словам

Задание со слайда: разобрать `printf '%-10s %5s\n' "$USER" "$(date +%s)"` до Enter.
Мой разбор: программа `printf`; первый аргумент в одинарных кавычках, это формат без
подстановок; `%-10s` строка шириной 10 влево, `%5s` шириной 5 вправо; второй аргумент
значение `$USER`, третий вывод `date +%s` (секунды с 1970 года). Перенаправлений нет.
Проверил через `set -x`, он показывает строку уже после всех подстановок:

```text
stepan@lab:~/bash-lab/paths$ set -x
stepan@lab:~/bash-lab/paths$ printf '%-10s %5s\n' "$USER" "$(date +%s)"
++ date +%s
+ printf '%-10s %5s\n' stepan 1795463603
stepan     1795463603
stepan@lab:~/bash-lab/paths$ set +x
+ set +x
```

`++` это подоболочка `$(date +%s)`, она выполнилась первой. Число из 10 цифр шире, чем `%5s`,
поле просто растянулось.

## Что понял

Программа никогда не видит `*`, `$name` и `$(...)`: всё раскрывает Bash, и `set -x` честно
показывает, что ушло на самом деле. Поведение шаблонов зависит от оболочки и её опций, так
что проверять надо в той оболочке, где сценарий будет работать.
