# Отчёт по домашнему заданию 3: Bash

Студент: Степан. Курс «Программирование в Linux», СПбГУ, 2026.

Всё делалось в контейнере с Ubuntu 24.04 (bash 5.2.21) от обычного пользователя `stepan`,
только внутри `~/bash-homework`. Вывод ниже скопирован из сеанса. В начале сеанса выключен
цвет у `ls` (`alias ls='ls --color=never'`), чтобы в выводе не было управляющих кодов.

## Задание

Сдать `bash-homework.txt` с командами и краткими выводами. Шаги:

1. создать `data` и три файла `*.log` с разными уровнями;
2. дописать по строке ERROR в два файла через `>>`;
3. найти все ERROR рекурсивно и сохранить без номеров строк;
4. построить сводку количества сообщений каждого уровня;
5. написать `report.sh`: принимает каталог, находит `*.log`, пишет `summary.txt`;
6. сделать сценарий исполняемым и проверить на каталоге с ошибками.

Базовый критерий: команды работают, пути не прибиты к одной машине. Продвинутый: проверки
аргументов, `set -u`, понятные сообщения об ошибках.

## Шаг 1. Каталог data и три журнала

Формат строки: время, уровень, сообщение. Третий файл положил в подкаталог `nginx`, чтобы
рекурсивный поиск на шаге 3 был не для галочки.

```text
stepan@lab:~$ mkdir -p ~/bash-homework && cd ~/bash-homework
stepan@lab:~/bash-homework$ mkdir -p data/nginx
stepan@lab:~/bash-homework$ cat > data/app.log <<'END'
> 2026-11-23T10:00:01 INFO app started
> 2026-11-23T10:00:05 INFO config loaded from /etc/app.conf
> 2026-11-23T10:02:17 WARN slow response 1840 ms
> 2026-11-23T10:05:42 INFO user stepan logged in
> END
stepan@lab:~/bash-homework$ cat > data/db.log <<'END'
> 2026-11-23T10:00:02 INFO db pool opened, size 10
> 2026-11-23T10:03:11 WARN connection pool 80% used
> 2026-11-23T10:03:12 ERROR deadlock detected on table orders
> END
stepan@lab:~/bash-homework$ cat > data/nginx/web.log <<'END'
> 2026-11-23T10:01:00 INFO GET /index.html 200
> 2026-11-23T10:01:03 INFO GET /api/items 200
> 2026-11-23T10:04:20 WARN GET /api/report 499 client closed request
> END
stepan@lab:~/bash-homework$ find data -type f -name '*.log' | sort
data/app.log
data/db.log
data/nginx/web.log
stepan@lab:~/bash-homework$ wc -l data/*.log data/nginx/*.log
  4 data/app.log
  3 data/db.log
  3 data/nginx/web.log
 10 total
```

Вывод: `cat > файл <<'END'` пишет блок как есть, кавычки у `'END'` запрещают подстановки
внутри.

## Шаг 2. Строка ERROR в два файла через >>

```text
stepan@lab:~/bash-homework$ printf '%s ERROR %s\n' "$(date +%Y-%m-%dT%H:%M:%S)" 'disk quota exceeded' >> data/app.log
stepan@lab:~/bash-homework$ printf '%s ERROR %s\n' "$(date +%Y-%m-%dT%H:%M:%S)" 'replica lag 30 s' >> data/db.log
stepan@lab:~/bash-homework$ tail -n 2 data/app.log data/db.log
==> data/app.log <==
2026-11-23T10:05:42 INFO user stepan logged in
2026-11-23T21:15:56 ERROR disk quota exceeded

==> data/db.log <==
2026-11-23T10:03:12 ERROR deadlock detected on table orders
2026-11-23T21:17:20 ERROR replica lag 30 s
```

Вывод: `>>` дописал строку в конец, старое содержимое на месте. С одной `>` файл был бы
очищен до запуска `printf`.

## Шаг 3. Все ERROR рекурсивно, без номеров строк

```text
stepan@lab:~/bash-homework$ grep -rw --color=never ERROR data
data/app.log:2026-11-23T21:15:56 ERROR disk quota exceeded
data/db.log:2026-11-23T10:03:12 ERROR deadlock detected on table orders
data/db.log:2026-11-23T21:17:20 ERROR replica lag 30 s
stepan@lab:~/bash-homework$ grep -rw ERROR data > errors.txt
stepan@lab:~/bash-homework$ cat errors.txt
data/app.log:2026-11-23T21:15:56 ERROR disk quota exceeded
data/db.log:2026-11-23T10:03:12 ERROR deadlock detected on table orders
data/db.log:2026-11-23T21:17:20 ERROR replica lag 30 s
stepan@lab:~/bash-homework$ wc -l < errors.txt
3
```

Вывод: номера строк `grep` добавляет только с `-n`, здесь его нет. Имя файла в начале строки
оставил, без него непонятно, откуда ошибка (убирается через `-h`). `-w` ищет целое слово,
чтобы не поймать, например, `ERRORS` в тексте сообщения.

## Шаг 4. Сводка по уровням

```text
stepan@lab:~/bash-homework$ find data -type f -name '*.log' -exec cat {} + | cut -d' ' -f2 | sort | uniq -c | sort -k1,1nr -k2
      6 INFO
      3 ERROR
      3 WARN
```

Вывод: уровень во втором поле, `cut` его вырезает, `sort | uniq -c` считает. Второй ключ
`-k2` нужен, потому что у ERROR и WARN поровну: на практике с одним `sort -nr` порядок
равных строк получался обратным алфавиту.

## Шаг 5. report.sh

Сценарий в файле [report.sh](report.sh). Что он делает:

- `./report.sh DIRECTORY [OUTPUT]`, по умолчанию сводка в `summary.txt` в текущем каталоге;
- ищет `*.log` через `find` с подкаталогами, имена с пробелами не ломаются (`-print0` и
  `mapfile -d ''`);
- для каждого файла считает строки ERROR, WARN, INFO и прочие, внизу строка TOTAL;
- пишет сначала во временный файл и переименовывает его в `summary.txt`, так что при ошибке
  старая сводка не затирается;
- начинается с `set -euo pipefail`, все ошибки пишет в stderr с именем сценария и выходит с
  ненулевым кодом: 2 при неверном числе аргументов, 1 при остальных ошибках.

Путей к конкретной машине в нём нет: каталог и файл сводки приходят аргументами,
интерпретатор ищется через `#!/usr/bin/env bash`.

```text
stepan@lab:~/bash-homework$ bash -n report.sh && echo "синтаксис ок"
синтаксис ок
stepan@lab:~/bash-homework$ shellcheck report.sh && echo "shellcheck: замечаний нет"
shellcheck: замечаний нет
```

Сценарий писал на своей машине и перенёс в `~/bash-homework` через `cat > report.sh`, а
не `cp`, чтобы файл получил обычные права (подробности в разделе «Что было непонятно»).

## Шаг 6. Исполняемый сценарий и проверка на каталоге с ошибками

```text
stepan@lab:~/bash-homework$ ls -l report.sh
-rw-rw-r-- 1 stepan stepan 3101 Nov 23 21:21 report.sh
stepan@lab:~/bash-homework$ ./report.sh data
bash: ./report.sh: Permission denied
stepan@lab:~/bash-homework$ chmod +x report.sh
stepan@lab:~/bash-homework$ ls -l report.sh
-rwxrwxr-x 1 stepan stepan 3101 Nov 23 21:21 report.sh
stepan@lab:~/bash-homework$ ./report.sh data
Сводка записана в summary.txt (файлов: 3)
stepan@lab:~/bash-homework$ cat summary.txt
Сводка по журналам в каталоге data
Файлов *.log: 3

FILE                            ERROR   WARN   INFO  OTHER
app.log                             1      1      3      0
db.log                              2      1      1      0
nginx/web.log                       0      1      2      0
TOTAL                               3      3      6      0
```

Вывод: итог совпал со сводкой из шага 4 (ERROR 3, WARN 3, INFO 6). ERROR нашлись в двух
файлах, куда я их дописывал, плюс одна была в `db.log` с самого начала.

## Проверки для продвинутого критерия

Неверные аргументы и неподходящие каталоги:

```text
stepan@lab:~/bash-homework$ ./report.sh; echo "код: $?"
Usage: report.sh DIRECTORY [OUTPUT]
Ищет *.log в DIRECTORY (с подкаталогами), сводку пишет в OUTPUT (summary.txt).
код: 2
stepan@lab:~/bash-homework$ ./report.sh data out.txt extra; echo "код: $?"
Usage: report.sh DIRECTORY [OUTPUT]
Ищет *.log в DIRECTORY (с подкаталогами), сводку пишет в OUTPUT (summary.txt).
код: 2
stepan@lab:~/bash-homework$ ./report.sh no-such-dir; echo "код: $?"
report.sh: ошибка: каталог не найден: no-such-dir
код: 1
stepan@lab:~/bash-homework$ ./report.sh data/app.log; echo "код: $?"
report.sh: ошибка: это не каталог: data/app.log
код: 1
stepan@lab:~/bash-homework$ mkdir -p empty && ./report.sh empty; echo "код: $?"
report.sh: ошибка: в каталоге empty нет файлов *.log
код: 1
stepan@lab:~/bash-homework$ ./report.sh data no-such-dir/summary.txt; echo "код: $?"
report.sh: ошибка: нельзя записать сводку в каталог: no-such-dir
код: 1
stepan@lab:~/bash-homework$ ./report.sh data data; echo "код: $?"
report.sh: ошибка: OUTPUT это каталог, нужен файл: data
код: 1
stepan@lab:~/bash-homework$ mkdir -p locked && chmod 000 locked && ./report.sh locked; echo "код: $?"; chmod 755 locked
report.sh: ошибка: нет прав на чтение каталога: locked
код: 1
```

Имена с пробелами и запуск из другого каталога:

```text
stepan@lab:~/bash-homework$ mkdir -p "logs with spaces" && cp data/db.log "logs with spaces/my db.log"
stepan@lab:~/bash-homework$ ./report.sh "logs with spaces" spaces-summary.txt && cat spaces-summary.txt
Сводка записана в spaces-summary.txt (файлов: 1)
Сводка по журналам в каталоге logs with spaces
Файлов *.log: 1

FILE                            ERROR   WARN   INFO  OTHER
my db.log                           2      1      1      0
TOTAL                               2      1      1      0
stepan@lab:~/bash-homework$ cd data && ../report.sh . ../summary-from-data.txt && cd ..
Сводка записана в ../summary-from-data.txt (файлов: 3)
stepan@lab:~/bash-homework$ diff <(tail -n +2 summary.txt) <(tail -n +2 summary-from-data.txt) && echo "сводки совпадают"
сводки совпадают
```

Каталог с `=` в имени и путь со слэшем на конце. Сначала посмотрел, как на такое имя
реагирует сам awk:

```text
stepan@lab:~/bash-homework$ mkdir -p 'x=y' && cp data/db.log 'x=y/a.log'
stepan@lab:~/bash-homework$ awk '{ print $2 }' 'x=y/a.log'
^C

stepan@lab:~/bash-homework$ awk '{ print $2 }' < 'x=y/a.log' | sort | uniq -c
      2 ERROR
      1 INFO
      1 WARN
stepan@lab:~/bash-homework$ ./report.sh 'x=y' xy.txt; echo "код: $?"; cat xy.txt
Сводка записана в xy.txt (файлов: 1)
код: 0
Сводка по журналам в каталоге x=y
Файлов *.log: 1

FILE                            ERROR   WARN   INFO  OTHER
a.log                               2      1      1      0
TOTAL                               2      1      1      0
stepan@lab:~/bash-homework$ ./report.sh data/; echo "код: $?"
Сводка записана в summary.txt (файлов: 3)
код: 0
```

Первый `awk` повис, пришлось нажать Ctrl+C (почему, см. ниже). Через `<` всё считается, и
`report.sh` подаёт файлы в awk именно так.

| Требование | Где выполнено |
|---|---|
| проверка числа аргументов | `check_args`, `Usage` и код 2 |
| каталог существует, это каталог, его можно читать | `check_args`, код 1 и сообщение |
| в каталоге есть `*.log` | `main`, сообщение «нет файлов *.log» |
| сводку можно записать, OUTPUT не каталог | `check_args`, код 1 и сообщение |
| `set -u` | `set -euo pipefail` в начале сценария |
| понятные сообщения | все в stderr в виде `report.sh: ошибка: ...` |
| `shellcheck` | замечаний нет |

## Что было непонятно

- Почему скопированный в контейнер сценарий уже исполняемый. Файлы с диска Windows внутри
  контейнера видны с правами 777, а `cp` переносит биты `x`. Проверка «сначала Permission
  denied, потом `chmod +x`» на такой копии ничего не доказывала. Перешёл на `cat src > dst`,
  а в `dist.sh` права в архиве выставляю явно.
- Почему `awk '{ print $2 }' 'x=y/a.log'` повис. Аргумент вида `имя=значение` awk считает
  присваиванием переменной, а не файлом, и ждёт ввод с терминала. Команда просто повисла без
  единого сообщения. В `report.sh` файл подаётся в awk через `<`, тогда имя awk не видит.
- Откуда `tmp: unbound variable` после успешной работы. Сначала временный файл был `local`
  внутри `main`, а `trap ... EXIT` срабатывает уже после выхода из функции. Под `set -u`
  переменной к этому моменту нет, и сценарий завершался с кодом 1, хотя сводку записал.
  Проверил на маленьком сценарии и сделал переменную глобальной.
- Почему подчёркивание заголовков в `bash-homework.txt` выходило вдвое длиннее. В Ubuntu
  `awk` это mawk, его `length("Отчёт")` вернул 10, то есть байты, а не буквы. Генерацию
  текста в `dist.sh` переписал на Bash: `${#title}` в локали `C.UTF-8` считает символы.

## Итог

Все шесть шагов сделаны, `report.sh` проходит `bash -n` и `shellcheck` без замечаний, на
каталоге с ошибками сводка сходится с ручным подсчётом. Для продвинутого критерия есть
проверки аргументов и каталогов, `set -euo pipefail`, сообщения в stderr с кодами 1 и 2.
