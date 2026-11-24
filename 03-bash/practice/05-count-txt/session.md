# Практика 5. Сценарий count_txt.sh

Финальное задание из части про сценарии: дописать заготовку, чтобы сценарий печатал имена
`*.txt` в каталоге и их число. Решение в [count_txt.sh](count_txt.sh): в цикле печать имени
и `((count += 1))`, остальное из заготовки.

Слайд предлагает набрать файл в nano или vim. Я писал его в своём репозитории, который в
контейнере лежит в `~/linux-programming-spbu`, и переносил через `cat > файл`, а не `cp`.
Репозиторий на диске Windows, оттуда всё приходит с правами 777, и `cp` сохранил бы лишний
`x`. Через `cat` файл создаётся заново с обычными правами, и шаг с `chmod +x` честный.

Каталог `course/docs` остался от практики 3, в нём уже были `readme.txt`, `notes.log` и
`setup.sh`. Добавил ещё два `.txt`, один с пробелом в имени.

## Проверка и запуск

```text
stepan@lab:~$ mkdir -p ~/bash-lab/scripts && cd ~/bash-lab
stepan@lab:~/bash-lab$ cat ~/linux-programming-spbu/03-bash/practice/05-count-txt/count_txt.sh > scripts/count_txt.sh
stepan@lab:~/bash-lab$ touch course/docs/intro.txt "course/docs/my notes.txt"
stepan@lab:~/bash-lab$ ls course/docs
 intro.txt  'my notes.txt'   notes.log   readme.txt   setup.sh
stepan@lab:~/bash-lab$ mkdir -p empty
stepan@lab:~/bash-lab$ bash -n scripts/count_txt.sh && echo "синтаксис ок"
синтаксис ок
stepan@lab:~/bash-lab$ shellcheck scripts/count_txt.sh && echo "shellcheck чисто"
shellcheck чисто
stepan@lab:~/bash-lab$ ls -l --color=never scripts/count_txt.sh
-rw-rw-r-- 1 stepan stepan 426 Nov 23 20:26 scripts/count_txt.sh
stepan@lab:~/bash-lab$ ./scripts/count_txt.sh course/docs
bash: ./scripts/count_txt.sh: Permission denied
stepan@lab:~/bash-lab$ chmod +x scripts/count_txt.sh
stepan@lab:~/bash-lab$ ./scripts/count_txt.sh ~/bash-lab/course/docs
Found: /home/stepan/bash-lab/course/docs/intro.txt
Found: /home/stepan/bash-lab/course/docs/my notes.txt
Found: /home/stepan/bash-lab/course/docs/readme.txt
Found: 3
```

Файл с пробелом в имени посчитался один раз: `"$dir"/*.txt` в кавычках только в части
переменной, шаблон раскрывается, а имена дальше не дробятся.

## Без аргументов и на пустом каталоге

```text
stepan@lab:~/bash-lab$ ./scripts/count_txt.sh; echo "код: $?"
Usage: ./scripts/count_txt.sh DIRECTORY
код: 1
stepan@lab:~/bash-lab$ ./scripts/count_txt.sh no-such-dir; echo "код: $?"
Usage: ./scripts/count_txt.sh DIRECTORY
код: 1
stepan@lab:~/bash-lab$ ./scripts/count_txt.sh empty; echo "код: $?"
Found: 0
код: 0
stepan@lab:~/bash-lab$ bash -x scripts/count_txt.sh empty
+ set -euo pipefail
+ [[ 1 -ne 1 ]]
+ [[ ! -d empty ]]
+ dir=empty
+ count=0
+ for file in "$dir"/*.txt
+ [[ -f empty/*.txt ]]
+ continue
+ printf 'Found: %d\n' 0
Found: 0
```

В трассировке видно, зачем нужна строка `[[ -f "$file" ]] || continue`: в пустом каталоге
шаблон не раскрылся, и в `file` попала строка `empty/*.txt`.

## Почему не count++

На слайде в решении `((count += 1))`. Проверил отдельно, что было бы с привычным
`((count++))` под `set -e`:

```text
stepan@lab:~/bash-lab$ bash -c 'set -e; n=0; ((n++)); echo "сюда не дошли"'; echo "код: $?"
код: 1
```

Постфиксный `++` возвращает старое значение 0, а `((0))` даёт код 1. Сценарий остановился
бы на первом же файле без единого сообщения. `((count += 1))` возвращает уже 1, с ним всё
работает.

## Критерии со слайда

| Критерий | Выполнен |
|---|---|
| корректный shebang | да, `#!/usr/bin/env bash` |
| проверка аргумента | да, число и что это каталог |
| цикл по файлам | да |
| подсчёт и вывод | да |
| сценарий исполняем | да, после `chmod +x` |
| нет ошибки при запуске без аргументов | да, `Usage` в stderr и код 1 |

## Что понял

`bash -x` на граничном случае (пустой каталог) объясняет больше, чем чтение кода. И под
`set -e` надо помнить, что арифметика с результатом 0 считается ошибкой.
