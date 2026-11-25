# Занятие 8. Как проверить программу

Лекция-практикум о пяти способах найти ошибку в коде: тесты, покрытие (gcov),
санитайзеры (ASan и UBSan), фаззинг и линтеры (cppcheck, clang-tidy). Всё на одном
маленьком проекте из архива Chapter 08: функция приветствия, в которой `strcat` пишет
за край буфера, а восемь тестов этого не видят. На практиках дописывал тесты, закрывал
белые пятна покрытия, читал отчёт санитайзера, ловил ошибку фаззером и чинил её через
`snprintf`. В ДЗ добавил в проект свою функцию `shorten_name` (сократить имя до 10 букв
с многоточием, буквы считаются по UTF-8) и прогнал её через все пять проверок.

## Что внутри

- [notes.md](notes.md): конспект лекции;
- [notes.pdf](notes.pdf): он же в PDF;
- [practice/](practice/): четыре практики, условия в [practice/README.md](practice/README.md);
- [homework/](homework/): ДЗ, условие и критерии в [homework/README.md](homework/README.md),
  отчёт в [homework/REPORT.md](homework/REPORT.md), архив `health-homework.tar.gz`.

## Как запустить

Всё проверял в контейнере с Ubuntu 24.04 (gcc 13.3.0, gcov 13.3.0, cppcheck 2.13.0,
clang-tidy 18.1.3, CMake 3.28.3). Из папки занятия:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential cmake cppcheck clang-tidy python3 file
```

Дальше, например, ДЗ:

```bash
cd homework
make test && make coverage && make sanitize && make fuzz && make tidy
make && ./app Александрина
```

Для практик нужен ещё архив Chapter 08 с проектом (его в репозитории нет, это материалы
преподавателя), как его подложить, написано в [practice/README.md](practice/README.md).
Подробнее про контейнер в [CONTRIBUTING.md](../CONTRIBUTING.md).
