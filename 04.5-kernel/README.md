# Занятие 4.5. Ядро Linux

Занятие о том, как смотреть на ядро и понимать, что видишь: где лежат ядро, конфигурация и
модули (`/boot`, `/lib/modules`, `/usr/src`), что ядро рассказывает о себе через `/proc`,
`/sys`, `lsmod`, `modinfo`, `/proc/cmdline`, `sysctl` и `dmesg`, и как выглядит правка ядра
(свой модуль, полная сборка, taint, Secure Boot). Правило занятия: смотреть можно всё, менять
нельзя ничего, правка ядра была только на слайдах. Отдельных практик на слайдах нет, задание
одно: ДЗ «Паспорт своей машины». Делал его на своём ноутбуке в WSL2 с Ubuntu 24.04.

## Что внутри

- [notes.md](notes.md): конспект лекции;
- [notes.pdf](notes.pdf): он же в PDF;
- [homework/](homework/): ДЗ, условие и критерии в [homework/README.md](homework/README.md),
  отчёт в [homework/REPORT.md](homework/REPORT.md), скрипт `passport.sh` и архив
  `kernel-homework.tar.gz`.

## Как запустить

Паспорт снимается на настоящей машине или в WSL, от обычного пользователя, без sudo:

```bash
cd homework
./passport.sh > passport.txt
```

В контейнере ядро то же, что у хоста, но модулей и доступа к `dmesg` там нет, поэтому
контейнер годится только для сборки архива и проверки скриптов:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y shellcheck
cd homework && shellcheck passport.sh dist.sh && ./dist.sh
```
