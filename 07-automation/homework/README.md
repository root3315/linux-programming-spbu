# ДЗ 7. Автоматическая сборка

## Условие

Взять проект `hello` из ДЗ 4 и довести его до сборки «без меня»:

1. Репозиторий проекта, ветка `main`.
2. Ветка `feature/version`: версия из `Makefile` попадает в `--version`.
3. `ci.sh` с шагами: сборка, тест, пакет.
4. `.github/workflows/ci.yml` с теми же шагами.
5. `.deb` с версией 1.1.0 и зависимостью от libc.
6. Профиль gprof для своей программы: горячая функция и что я с ней сделал.

Сдаётся репозиторий (архив с `.git`) и `report.txt`. Структура отчёта со слайда:
`git log --oneline --graph`, вывод `./ci.sh`, `dpkg-deb -c` и `-I`, gprof и горячая функция,
что было непонятно.

## Критерии

Базовый: `git log` читается как история работы, `ci.sh` зелёный, пакет собирается, профиль
приложен. Выполнен: история ДЗ в репозитории курса идёт отдельными коммитами (проект, ветка
`feature/version`, правка в `main`, слияние, оптимизация), граф в разделе 1 отчёта. `./ci.sh`
с нуля собирает, тестирует, пакует и проверяет пакет, код выхода 0 (раздел 2). На сломанном
тесте он возвращает 2 и пакет не собирает. `hello_1.1.0_amd64.deb` с `Depends: libc6
(>= 2.34)`, `dpkg-deb -c` и `-I` в разделе 3. Профиль до и после в разделе 4.

Продвинутый: тег `v1.1.0`, конфликт в истории разрешён и виден в графе, время программы
уменьшено и подтверждено замером. Выполнен: в `feature/version` и в `main` по-разному поменял
одну и ту же строку подсказки `--help`, при слиянии получил конфликт и оставил объединённый
вариант. Коммит слияния с двумя родителями виден в графе, тег `v1.1.0` стоит на последнем
коммите. `./hello --bench 50000` ускорился с 1.0 с до 0.001 с, замеры `time` и gprof до и после
в разделе 4.

Workflow лежит в корне репозитория курса, `.github/workflows/ci.yml`, и запускает тот же
`ci.sh`. Срабатывает на push и pull request, только если менялся `07-automation/homework/hello/`
или сам workflow. Собранный `.deb` сохраняется как артефакт `hello-deb`.

## Что где лежит

| Файл | Что это |
|---|---|
| [hello/main.c](hello/main.c) | точка входа: имя, `--help`, `--version`, `--bench N` |
| [hello/greet.c](hello/greet.c), [greet.h](hello/greet.h) | модуль приветствия из ДЗ 4 без изменений |
| [hello/stats.c](hello/stats.c), [stats.h](hello/stats.h) | статистика по имени, добавлен подсчёт разных байт |
| [hello/tests/test_hello.c](hello/tests/test_hello.c) | тест greet и stats, код 1 при ошибке |
| [hello/Makefile](hello/Makefile) | `VERSION`, цели `all`, `test`, `install`, `deb`, `profile`, `clean` |
| [hello/packaging/](hello/packaging/) | шаблон `control` и `build-deb.sh` |
| [hello/ci.sh](hello/ci.sh) | локальный CI: чистая сборка, тест, пакет, проверка пакета |
| [hello/.gitignore](hello/.gitignore) | артефакты сборки, пакета и профиля |
| [REPORT.md](REPORT.md) | отчёт |
| [dist.sh](dist.sh) | делает `report.txt`, bundle репозитория и архив |
| `automation-homework.tar.gz` | архив для сдачи |

## Как собрать

В контейнере с Ubuntu 24.04:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential git
```

```bash
cd hello
./ci.sh                      # сборка, тест, пакет, проверка пакета
./hello --version            # hello 1.1.0
./hello "Linux"
make profile                 # сборка с -pg в build-prof/ и плоский профиль
make profile BENCH=50000000  # нагрузка побольше, если профиль пустой
dpkg-deb -c dist/hello_1.1.0_amd64.deb
make clean
```

`dpkg-deb` есть в любой Ubuntu, `gprof` приезжает с binutils из `build-essential`. Пакет
ставить не нужно: `ci.sh` сам распаковывает его через `dpkg-deb -x` и запускает программу.

## Как сделать архив

```bash
./dist.sh ../..              # путь к корню репозитория курса
```

Скрипт переводит `REPORT.md` в простой текст `report.txt` (заголовки подчёркнуты `===` и
`---`, код с отступом в два пробела), снимает `git bundle create --all` с репозитория и пакует
в `automation-homework.tar.gz` каталог с `report.txt`, `hello-repo.bundle` и `hello/`.
Bundle — это весь `.git` одним файлом, со всеми ветками и тегами:

```bash
tar xzf automation-homework.tar.gz && cd automation-homework
git clone hello-repo.bundle repo
git -C repo log --oneline --graph --decorate -- 07-automation/homework/hello
```

Архив проверил в Ubuntu 24.04 (WSL) с пакетами выше: распаковал, `./ci.sh` зелёный
и в `hello/` из архива, и в клоне из bundle.
