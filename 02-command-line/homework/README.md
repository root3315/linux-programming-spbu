# ДЗ 2. Командная строка

## Условие

Всё делается в терминале, в домашнем каталоге:

1. Создать каталог `project` в домашней папке.
2. Создать в нём `hello.txt` и `world.txt`.
3. С помощью `find` и `grep` найти все файлы, где встречается слово «linux».
4. Запустить `sleep 100` в фоне, найти его PID и завершить процесс.
5. Через nano добавить в `hello.txt` строку `Hello, Linux!`.
6. Через vi скопировать содержимое `hello.txt` в `world.txt`. Подсказка со слайда: в vim
   открыть `hello.txt`, скопировать всё через `ggyG`, затем `:e world.txt` и вставить через `p`.

Сдаётся история команд (`history`) и вывод `ls -la project`. Оценивают и результат, и то,
какими командами он получен.

## Критерии

Базовый: выполнены все шесть пунктов, есть `history` и `ls -la project`.

Продвинутый (поставил себе сам, на слайде его нет): поиск сделан одной связкой `find` + `grep`,
PID найден отдельной командой и проверен через `ps`, после `kill` проверено, что процесса
больше нет, в отчёте по каждому шагу есть настоящий вывод и нажатые в редакторах клавиши.

## Что где лежит

| Файл | Что это |
|---|---|
| [REPORT.md](REPORT.md) | отчёт: команды, вывод, пояснения по шагам |
| [history.txt](history.txt) | вывод `history` из сессии |
| [ls-la-project.txt](ls-la-project.txt) | вывод `ls -la project` |
| [dist.sh](dist.sh) | делает `report.txt` из `REPORT.md` и собирает архив |
| `cli-homework.tar.gz` | архив для сдачи |

## Как собрать архив

```bash
./dist.sh
tar tzf cli-homework.tar.gz
```

В архиве каталог `cli-homework/` с файлами `history.txt`, `ls-la-project.txt` и `report.txt`
(тот же REPORT.md, но простым текстом).

Повторить саму сессию можно в контейнере с Ubuntu 24.04:

```bash
docker run --rm -it --hostname devbox -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y nano vim-tiny procps
useradd -m -s /bin/bash stepan && su - stepan
```

Дальше команды по порядку из [history.txt](history.txt). Файлы `history.txt` и
`ls-la-project.txt` появятся в `/home/stepan`; после `exit` скопировать их в `/work`.
