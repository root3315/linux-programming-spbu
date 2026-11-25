# Практика 1. Осмотреть сеть

Контейнер с Ubuntu 24.04, пользователь `stepan`. `iproute2` и `netcat-openbsd` поставил
сам, в голом образе `ip` и `ss` нет.

## Адреса и маршрут

```text
stepan@box:~$ ip -brief addr
lo               UNKNOWN        127.0.0.1/8 ::1/128
eth0@if36        UP             172.17.0.4/16
stepan@box:~$ ip route
default via 172.17.0.1 dev eth0
172.17.0.0/16 dev eth0 proto kernel scope link src 172.17.0.4
```

Мой адрес `172.17.0.4/16`, маска 16 бит значит, что общие первые два числа: соседи это
`172.17.x.x`. Шлюз `172.17.0.1`, через него уходит всё, что не в этой сети. `lo` это
`127.0.0.1`, петля. `ping` в образе нет, проверять петлю им не стал.

## Кто слушает

В свежем контейнере `ss -ltn` печатает только шапку: слушать некому. Запустил простой
сервер, привязав его к петле:

```text
stepan@box:~$ python3 -m http.server 8000 --bind 127.0.0.1 &
stepan@box:~$ ss -ltn
State  Recv-Q Send-Q Local Address:Port Peer Address:Port Process
LISTEN 0      5          127.0.0.1:8000      0.0.0.0:*
stepan@box:~$ ss -ltnp
State  Recv-Q Send-Q Local Address:Port Peer Address:PortProcess
LISTEN 0      5          127.0.0.1:8000      0.0.0.0:*    users:(("python3",pid=252,fd=3))
```

Порт 8000 слушает `python3`, процесс 252, дескриптор 3. Без `-p` имя программы не
показывается. Без `--bind` `http.server` слушает `0.0.0.0`, то есть виден всей сети: в
практике 3 он так и запустился, это видно в выводе `ss` там.

## Порты и имена

```text
stepan@box:~$ grep -E '^(ssh|http|https)\b' /etc/services
ssh		22/tcp				# SSH Remote Login Protocol
http		80/tcp		www		# WorldWideWeb HTTP
https		443/tcp				# http protocol over TLS/SSL
https		443/udp				# HTTP/3
http-alt	8080/tcp	webcache	# WWW caching service
stepan@box:~$ cat /etc/hosts
127.0.0.1	localhost
::1	localhost ip6-localhost ip6-loopback
fe00::	ip6-localnet
ff00::	ip6-mcastprefix
ff02::1	ip6-allnodes
ff02::2	ip6-allrouters
172.17.0.4	8748f96e0765
```

Последняя строка `/etc/hosts` привязывает имя контейнера к его адресу без DNS.

## Что понял

Адрес и шлюз находятся двумя командами, а кто слушает порт, говорит `ss -ltnp`. Один адрес и
много портов: на разных портах легко жить десятку программ. Порт 8080, который я потом
занимаю эхо-сервером, в `/etc/services` записан как `http-alt`.
