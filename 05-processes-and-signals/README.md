# Занятие 5. Процессы, сигналы и системные вызовы

Лекция-практикум о том, что происходит между запуском программы и её завершением. Разбирали,
чем программа отличается от процесса, как устроено адресное пространство и состояния, потом
`fork`, `exec` и `wait` с зомби и кодами возврата, сигналы и `sigaction`, дескрипторы,
`pipe` и `dup2`, в конце коротко потоки с гонкой данных и диагностику через `/proc`,
`strace` и `gdb`. Все примеры из архива Chapter 05 преподавателя. В ДЗ нужно было написать
свою маленькую оболочку с конвейером, `cd` и обработкой Ctrl+C.

## Что внутри

- [notes.md](notes.md): конспект лекции;
- [notes.pdf](notes.pdf): он же в PDF;
- [practice/](practice/): четыре практики и задание РПД №5 про скрипт зомби, условия в
  [practice/README.md](practice/README.md);
- [homework/](homework/): ДЗ, условие и критерии в [homework/README.md](homework/README.md),
  отчёт в [homework/REPORT.md](homework/REPORT.md), архив `proc-homework.tar.gz`.

## Как запустить

Всё проверял в контейнере с Ubuntu 24.04 (gcc 13.3.0, GNU Make 4.3, strace 6.8). Из папки
занятия:

```bash
docker run --rm -it --cap-add SYS_PTRACE -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential strace gdb psmisc procps shellcheck
```

`--cap-add SYS_PTRACE` нужен только для `strace -p` и `gdb -p`. Дальше, например, ДЗ:

```bash
cd homework
make && make test
./minishell
```

Для практик нужен ещё архив Chapter 05 с примерами (его в репозитории нет, это материалы
преподавателя), команды по шагам в `session.md` каждой практики. Подробнее про контейнер
написано в [CONTRIBUTING.md](../CONTRIBUTING.md).
