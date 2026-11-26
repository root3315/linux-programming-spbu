# Программирование в Linux

Мои материалы по курсу «Программирование в Linux» (СПбГУ, кафедра искусственного интеллекта
в системах управления, 2026): конспекты лекций, решения практик и домашние задания.

## Занятия

| № | Тема | Конспект | Практика | Домашнее задание |
|---|---|---|---|---|
| 1 | [История Linux](01-history-of-linux/README.md) | [md](01-history-of-linux/notes.md), [pdf](01-history-of-linux/notes.pdf) | | |
| 2 | [Командная строка](02-command-line/README.md) | [md](02-command-line/notes.md), [pdf](02-command-line/notes.pdf) | | [сеанс в терминале](02-command-line/homework/README.md), [отчёт](02-command-line/homework/REPORT.md) |
| 3 | [Bash](03-bash/README.md) | [md](03-bash/notes.md), [pdf](03-bash/notes.pdf) | [7 практик](03-bash/practice/README.md) | [сводка по журналам](03-bash/homework/README.md), [отчёт](03-bash/homework/REPORT.md) |
| 4 | [Сборка программ](04-build-toolchain/README.md) | [md](04-build-toolchain/notes.md), [pdf](04-build-toolchain/notes.pdf) | [4 практики](04-build-toolchain/practice/README.md) | [модуль stats](04-build-toolchain/homework/README.md), [отчёт](04-build-toolchain/homework/REPORT.md) |
| 4.5 | [Ядро Linux](04.5-kernel/README.md) | [md](04.5-kernel/notes.md), [pdf](04.5-kernel/notes.pdf) | | [паспорт машины](04.5-kernel/homework/README.md), [отчёт](04.5-kernel/homework/REPORT.md) |
| 5 | [Процессы и сигналы](05-processes-and-signals/README.md) | [md](05-processes-and-signals/notes.md), [pdf](05-processes-and-signals/notes.pdf) | [4 практики](05-processes-and-signals/practice/README.md) | [мини-шелл](05-processes-and-signals/homework/README.md), [отчёт](05-processes-and-signals/homework/REPORT.md) |
| 6 | [Контейнеры](06-containers/README.md) | [md](06-containers/notes.md), [pdf](06-containers/notes.pdf) | [4 практики](06-containers/practice/README.md) | [образы для hello](06-containers/homework/README.md), [отчёт](06-containers/homework/REPORT.md) |
| 7 | [Автоматизация](07-automation/README.md) | [md](07-automation/notes.md), [pdf](07-automation/notes.pdf) | [4 практики](07-automation/practice/README.md) | [CI, пакет и профиль](07-automation/homework/README.md), [отчёт](07-automation/homework/REPORT.md) |
| 8 | [Проверка программ](08-code-health/README.md) | [md](08-code-health/notes.md), [pdf](08-code-health/notes.pdf) | [4 практики](08-code-health/practice/README.md) | [тесты и фаззинг](08-code-health/homework/README.md), [отчёт](08-code-health/homework/REPORT.md) |
| 9 | [Сети](09-networks/README.md) | [md](09-networks/notes.md), [pdf](09-networks/notes.pdf) | [4 практики](09-networks/practice/README.md) | [чат на двоих](09-networks/homework/README.md), [отчёт](09-networks/homework/REPORT.md) |
| 10 | [Генераторы кода](10-codegen-docs/README.md) | [md](10-codegen-docs/notes.md), [pdf](10-codegen-docs/notes.pdf) | [4 практики](10-codegen-docs/practice/README.md) | [калькулятор](10-codegen-docs/homework/README.md), [отчёт](10-codegen-docs/homework/REPORT.md) |
| 11 | [Python и systemd](11-python-systemd/README.md) | [md](11-python-systemd/notes.md), [pdf](11-python-systemd/notes.pdf) | [4 практики](11-python-systemd/practice/README.md) | [ночной сторож](11-python-systemd/homework/README.md), [отчёт](11-python-systemd/homework/REPORT.md) |

## Как читать

В каждой папке есть `README.md`: что было на занятии, где что лежит и как это запустить.
В практиках рядом с решением лежит `session.md` с командами и выводом. В `homework/` условие
с критериями, исходники, отчёт и архив для сдачи, в архиве тот же отчёт текстом.

Слайдов и архивов преподавателя здесь нет. Если решение опирается на пример из архива, ссылка
на него есть в условиях практики.

## Как запустить

Всё собирается и запускается в Ubuntu 24.04 (GCC 13, GNU Make 4.3, CMake 3.28). Команда для
контейнера и правила оформления записаны в [CONTRIBUTING.md](CONTRIBUTING.md), сборка
конспектов в PDF описана в [tools/notes](tools/notes/README.md). Проект из занятия 7 собирает и
тестирует [GitHub Actions](.github/workflows/ci.yml).
