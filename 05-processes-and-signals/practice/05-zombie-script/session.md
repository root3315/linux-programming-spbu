# Задание РПД №5. Скрипт, который выводит все зомби-процессы

В Chapter 05 решение уже есть: `zombie-finder.sh` перебирает `/proc/<pid>/stat` и берёт
букву состояния после последней `)`. Я написал свой вариант, [zombies.sh](zombies.sh),
через `ps`: в нём меньше разбора руками, и он печатает ещё имя родителя, потому что
чинить надо именно родителя. Проверял оба на зомби с пробелом в имени, ради этого всё и
затевалось. Концы строк в `zombie-finder.sh` я поправил ещё на практике 2
(`sed -i 's/\r$//'`).

## Без зомби

```text
stepan@devbox:~$ cd ~/proc-lab
stepan@devbox:~/proc-lab$ sh zombies.sh; echo "код=$?"
PID      PPID     COMMAND          PARENT
(зомби нет)
код=0
```

## Зомби с пробелом в имени

Скопировал `zombie` под именем `my zombie`, имя процесса берётся из имени файла.

```text
stepan@devbox:~/proc-lab$ cp zombie "my zombie"
stepan@devbox:~/proc-lab$ ./"my zombie" 90 &
[1] 364
stepan@devbox:~/proc-lab$ [родитель 364] ребёнок 365 завершился, ждём 90 с
смотрите: ps -o pid,ppid,stat,comm -p 365
[ребёнок 365] работаю и выхожу
awk '$3 == "Z" {print $1, $2}' /proc/[0-9]*/stat
stepan@devbox:~/proc-lab$ grep -h 'my zombie' /proc/[0-9]*/stat | cut -c1-40
364 (my zombie) S 350 364 349 34816 367 
365 (my zombie) Z 364 364 349 34816 367 
```

Наивный `awk` с третьим полем не нашёл ничего. В `stat` имя стоит в скобках и содержит
пробел, поэтому для `awk` поле 2 — `(my`, поле 3 — `zombie)`, а `Z` уезжает в четвёртое.
Ровно от этого защищается `zombie-finder.sh`, когда режет строку по последней `) `.

```text
stepan@devbox:~/proc-lab$ ./zombie-finder.sh
PID      PPID     COMMAND
365      364      my zombie
stepan@devbox:~/proc-lab$ sh zombies.sh; echo "код=$?"
PID      PPID     COMMAND          PARENT
365      364      my zombie        my zombie
код=1
```

Оба нашли. У `ps` формат `comm` стоит последним, `read` кладёт весь остаток строки в
последнюю переменную, и пробел в имени не мешает.

## Второй зомби и проверка shellcheck

```text
stepan@devbox:~/proc-lab$ ./zombie 60 &
[2] 384
stepan@devbox:~/proc-lab$ [родитель 384] ребёнок 385 завершился, ждём 60 с
смотрите: ps -o pid,ppid,stat,comm -p 385
[ребёнок 385] работаю и выхожу
[родитель] завершаюсь; зомби заберёт init
sh zombies.sh
PID      PPID     COMMAND          PARENT
385      384      zombie           zombie
[1]-  Done                    ./"my zombie" 90
stepan@devbox:~/proc-lab$ shellcheck zombies.sh && echo ok
ok
```

Второго зомби увидеть вместе с первым не успел: пока я запускал `./zombie 60`, у
`my zombie` кончились его 90 секунд, он вышел (строка «завершаюсь») и зомби забрал PID 1.
Через минуту то же случилось со вторым:

```text
stepan@devbox:~/proc-lab$ [родитель] завершаюсь; зомби заберёт init
sh zombies.sh; echo "код=$?"
PID      PPID     COMMAND          PARENT
(зомби нет)
[2]+  Done                    ./zombie 60
код=0
```

## Что понял

Разбирать `/proc/<pid>/stat` по пробелам нельзя: имя процесса задаёт тот, кто его
запускает, и в нём может быть что угодно. Либо резать по последней скобке, либо брать
готовый `ps`, у которого `comm` можно поставить в конец строки. Код выхода 1 при найденных
зомби оставил, чтобы скрипт можно было вставить в проверку вроде `make test`.
