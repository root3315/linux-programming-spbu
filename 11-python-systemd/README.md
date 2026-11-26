# Занятие 11. Python и systemd

Лекция-практикум о том, как сделать, чтобы программа работала сама. Python берёт задачи, где
появляется логика: проценты, JSON, проверка сайта, понятные ошибки и код возврата. systemd
запускает такой скрипт: служба описывает, что запустить, таймер — когда, а вывод остаётся в журнале.
На практиках разбирал скрипты из архива преподавателя, добавил в `logsum.py` JSON и проценты, ставил
службу и таймер в свою сессию и писал своё расписание. Домашнее задание: «ночной сторож», который
раз в сутки проверяет каталог и сайт.

## Что внутри

- [notes.md](notes.md): конспект лекции;
- [notes.pdf](notes.pdf): он же в PDF;
- [practice/](practice/): четыре практики, условия в [practice/README.md](practice/README.md);
- [homework/](homework/README.md): ДЗ, отчёт и архив `nightcheck-homework.tar.gz`.

## Как запустить

Проверял на Ubuntu 24.04 (Python 3.12.3, systemd 255). systemd в моём WSL выключен, поэтому
юниты запускал в контейнере с systemd в роли PID 1, подробнее в
[practice/README.md](practice/README.md). Ставить ничего не надо, всё работает в своей сессии без
root. Если распаковать архив в папке занятия:

```bash
wget https://www.smartmelio.ru/chapter11.tar.gz
tar -xzvf chapter11.tar.gz
cd Chapter11/python
cp ../../practice/01-scripts/app.log .        # в архиве журнала нет
cp ../../practice/02-report/logsum.py .
python3 logsum.py app.log
```

Ночной сторож из ДЗ:

```bash
cd homework
./tests.sh
./install.sh
systemctl --user start nightcheck.service
journalctl --user -u nightcheck.service -n 20
./uninstall.sh
```
