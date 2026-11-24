# Практика 4. Анализ журнала

Журнал `app.log` со слайда, собирал конвейер по одному звену и смотрел, что меняется.
Каталог `~/bash-lab/logs`. У `grep` выключал цвет, чтобы в записи не было кодов.

## Журнал

```text
stepan@lab:~$ mkdir -p ~/bash-lab/logs && cd ~/bash-lab/logs
stepan@lab:~/bash-lab/logs$ printf '%s\n' 'INFO start' 'WARN slow' 'ERROR timeout' 'ERROR database' 'INFO stop' > app.log
stepan@lab:~/bash-lab/logs$ cat app.log
INFO start
WARN slow
ERROR timeout
ERROR database
INFO stop
stepan@lab:~/bash-lab/logs$ wc -l app.log
5 app.log
stepan@lab:~/bash-lab/logs$ head -n 2 app.log
INFO start
WARN slow
```

## Конвейер по звеньям

```text
stepan@lab:~/bash-lab/logs$ grep --color=never ERROR app.log
ERROR timeout
ERROR database
stepan@lab:~/bash-lab/logs$ grep ERROR app.log | wc -l
2
stepan@lab:~/bash-lab/logs$ grep -c ERROR app.log
2
stepan@lab:~/bash-lab/logs$ cut -d" " -f1 app.log
INFO
WARN
ERROR
ERROR
INFO
stepan@lab:~/bash-lab/logs$ cut -d" " -f1 app.log | uniq -c
      1 INFO
      1 WARN
      2 ERROR
      1 INFO
stepan@lab:~/bash-lab/logs$ cut -d" " -f1 app.log | sort | uniq -c
      2 ERROR
      2 INFO
      1 WARN
stepan@lab:~/bash-lab/logs$ cut -d" " -f1 app.log | sort | uniq -c | sort -nr
      2 INFO
      2 ERROR
      1 WARN
stepan@lab:~/bash-lab/logs$ grep --color=never -in 'error' app.log
3:ERROR timeout
4:ERROR database
```

Без `sort` перед `uniq -c` INFO посчитался два раза по одному: `uniq` склеивает только
соседние строки. При равных числах `sort -nr` поставил INFO выше ERROR: ключ одинаковый, и
`sort` сравнивает строки целиком, тоже в обратном порядке. Если нужен стабильный вывод,
второй ключ надо задать явно: `sort -k1,1nr -k2`. Так я и сделал в домашнем задании.

## tee и stderr

```text
stepan@lab:~/bash-lab/logs$ grep ERROR app.log | tee errors.log
ERROR timeout
ERROR database
stepan@lab:~/bash-lab/logs$ cat errors.log
ERROR timeout
ERROR database
stepan@lab:~/bash-lab/logs$ ls app.log no-such-file
ls: cannot access 'no-such-file': No such file or directory
app.log
stepan@lab:~/bash-lab/logs$ ls app.log no-such-file 2>/dev/null
app.log
stepan@lab:~/bash-lab/logs$ ls app.log no-such-file | wc -l
ls: cannot access 'no-such-file': No such file or directory
1
stepan@lab:~/bash-lab/logs$ ls app.log no-such-file 2>&1 | wc -l
2
stepan@lab:~/bash-lab/logs$ ls app.log no-such-file > out.txt 2> err.txt; cat out.txt; cat err.txt
app.log
ls: cannot access 'no-such-file': No such file or directory
```

`wc -l` насчитал 1: ошибка прошла мимо конвейера прямо на экран. С `2>&1` строк стало 2.

## Коды возврата в конвейере

```text
stepan@lab:~/bash-lab/logs$ grep -q ERROR app.log && echo "есть ошибки"
есть ошибки
stepan@lab:~/bash-lab/logs$ grep -q FATAL app.log || echo "FATAL нет, код $?"
FATAL нет, код 1
stepan@lab:~/bash-lab/logs$ false | true; echo "код конвейера: $?"
код конвейера: 0
stepan@lab:~/bash-lab/logs$ set -o pipefail; false | true; echo "с pipefail: $?"; set +o pipefail
с pipefail: 1
```

## Что понял

Конвейер удобно собирать по одному звену и смотреть промежуточный вывод, так сразу видно,
где ломается подсчёт. Код конвейера по умолчанию берётся от последней команды, упавшую
середину видно только с `pipefail`.
