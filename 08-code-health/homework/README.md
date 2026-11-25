# ДЗ 8. Своя функция под пятью проверками

## Условие

Добавить в проект из Chapter 08 свою функцию (на слайде пример: «сократить имя до 10 букв с
многоточием») и проверить её всеми пятью способами:

1. Функция и заголовок, понятное описание в комментарии.
2. Минимум три теста: обычный случай, пустой, слишком длинный.
3. `make coverage`: 100% строк своей функции.
4. `make fuzz`: фаззер должен доходить и до неё.
5. `make tidy`: ноль предупреждений.
6. `report.txt`: что нашли инструменты и что исправлено.

Структура отчёта со слайда: что делает функция; `make test` — вывод; `make coverage` —
процент и белые пятна; отчёт санитайзера до исправления; выводы.

## Критерии

Базовый: все четыре команды зелёные, ошибок не осталось, тесты осмысленные.
Выполнен: `make test`, `make coverage`, `make fuzz`, `make tidy` (и заодно `make sanitize`)
выходят с кодом 0. У `shorten.c` 100% строк, у `greeting.c` тоже. 14 тестов на
`shorten_name`: обычное, пустое, `NULL`, длинное, кириллица, эмодзи, битый UTF-8, буфер
впритык. Вывод в [REPORT.md](REPORT.md), разделы 2–3.

Продвинутый: специально внести ошибку, найти её фаззером, исправить и показать «до и
после». Выполнен: ошибка «многоточие не учтено в размере» проходит тесты, покрытие 100% и
`make sanitize`, а фаззер ловит её на 4-й попытке (ASan, heap-buffer-overflow). Плюс одну
настоящую ошибку с битым UTF-8 фаззер нашёл сам. Всё в разделе 4 отчёта.

## Функция

`shorten_name(out, cap, name)` режет имя до 10 символов вместе с многоточием: длинное имя
становится «первые 9 букв + `…`». Буквы считаются по UTF-8, а не по байтам, поэтому
«Александрина» превращается в «Александр…», а не в обрубок посреди буквы. Подробности и коды
возврата в [include/shorten.h](include/shorten.h). Программа `app` теперь сначала сокращает
имя, потом здоровается:

```text
$ ./app Александрина
Hello, Александр…!
```

## Что где лежит

Это рабочая копия проекта из `Chapter08/project`, так её проще собрать целиком.

| Файл | Чьё | Что это |
|---|---|---|
| [include/shorten.h](include/shorten.h) | моё | объявление и описание `shorten_name` |
| [src/shorten.c](src/shorten.c) | моё | реализация |
| [tests/test_shorten.c](tests/test_shorten.c) | моё | 14 тестов `shorten_name` |
| [.clang-tidy](.clang-tidy) | моё | набор проверок для `clang-tidy` и почему одно правило выключено |
| [dist.sh](dist.sh) | моё | делает `report.txt` из `REPORT.md` и архив |
| [REPORT.md](REPORT.md) | моё | отчёт |
| [Makefile](Makefile) | правка | все цели знают про `shorten.c`, `tidy` проверяет весь проект и падает при замечаниях, учебный файл переехал в `lint-demo` |
| [CMakeLists.txt](CMakeLists.txt) | правка | библиотека `health`, тесты и фаззер, чтобы `clang-tidy` видел все файлы по одному разу |
| [fuzz/fuzz_greeting.c](fuzz/fuzz_greeting.c) | правка | фаззит обе функции, имена из UTF-8 и битых байтов, буфер случайного размера в куче |
| [src/greeting.c](src/greeting.c) | правка | `snprintf` вместо `strcpy`/`strcat` (из практики 4) |
| [src/main.c](src/main.c) | правка | сокращает имя перед приветствием |
| [tests/test_greeting.c](tests/test_greeting.c) | правка | тесты из практик 1–2 |
| [.gitignore](.gitignore) | правка | плюс `tests/test_shorten` |
| [include/greeting.h](include/greeting.h), [lint/issues.c](lint/issues.c) | преподаватель | без изменений (только концы строк переведены в LF) |
| `health-homework.tar.gz` | | архив для сдачи |

## Как собрать

В контейнере с Ubuntu 24.04:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential cmake cppcheck clang-tidy
```

```bash
make && ./app Александрина
make test           # тесты greeting и shorten
make coverage       # gcov, падает, если в greeting.c или shorten.c есть #####
make sanitize       # тесты под ASan + UBSan
make fuzz           # 2000 попыток, зерно 1; можно make fuzz FUZZ_RUNS=100000 FUZZ_SEED=7
make tidy           # cppcheck + clang-tidy по src, tests, fuzz; код 1 при любом замечании
make lint-demo      # учебный lint/issues.c с лекции, тут линтеры должны ругаться
make clean
```

## Как сделать архив

```bash
./dist.sh
```

Скрипт переводит `REPORT.md` в простой текст `report.txt` (заголовки подчёркнуты `===` и
`---`, код с отступом в два пробела) и пакует каталог `health-homework/` с проектом и
`report.txt`. Архив проверил в чистом `ubuntu:24.04`: распаковал, поставил пакеты выше, все
цели из списка отработали.
