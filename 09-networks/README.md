# Занятие 9. Сети: как программы разговаривают

Лекция-практикум о том, как две программы передают друг другу данные: адрес и порт, сокет как
дескриптор, TCP (звонок) и UDP (открытка), HTTP как обычный текст и пять проверок, когда «сеть не
работает». Всё на `127.0.0.1`. На практиках запускал эхо-сервер и клиент из архива преподавателя,
запрашивал страницу своим `http_get` и искал поломки. В ДЗ сделал «чат на двоих»: TCP-сервер, который
отвечает `привет, имя! ты сказал: ...`, считает сообщения и не падает на плохих строках.

## Что внутри

- [notes.md](notes.md): конспект лекции;
- [notes.pdf](notes.pdf): он же в PDF;
- [practice/](practice/): четыре практики, условия в [practice/README.md](practice/README.md);
- [homework/](homework/): ДЗ, условие и критерии в [homework/README.md](homework/README.md),
  отчёт в [homework/REPORT.md](homework/REPORT.md), архив `net-homework.tar.gz`.

## Как запустить

Всё проверял в контейнере с Ubuntu 24.04 (gcc 13.3.0, iproute2, netcat-openbsd, Python 3.12). Из папки
занятия:

```bash
docker run --rm -it -v "${PWD}:/work" -w /work ubuntu:24.04 bash
apt update && apt install -y build-essential iproute2 netcat-openbsd curl python3 strace
```

ДЗ:

```bash
cd homework
make
./chat_server 9100 &
./chat_client 127.0.0.1 9100 Степан "привет" "как дела"
```

Для практик нужен архив Chapter 09 с примерами (его в репозитории нет, это материалы
преподавателя), как его подложить, написано в [practice/README.md](practice/README.md). Подробнее про
контейнер в [CONTRIBUTING.md](../CONTRIBUTING.md).
