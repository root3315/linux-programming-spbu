# Практика 6. SSH

Задание со слайда: подключиться, выполнить `whoami` и `hostname`, создать файл одной командой,
скопировать файл на сервер и обратно. Отдельного сервера у меня не было, поэтому поднял
`sshd` на порту 2222 в том же контейнере (`apt install openssh-server`, запуск от root:
`mkdir -p /run/sshd; /usr/sbin/sshd -p 2222`). Клиент и сервер на одной машине, адрес
`127.0.0.1`, но обмен идёт через настоящий SSH с ключами и `known_hosts`. Дальше всё от
пользователя `stepan`.

## Ключ и config

```text
stepan@lab:~$ ssh-keygen -t ed25519 -C "stepan@lab" -N '' -f ~/.ssh/id_ed25519
Generating public/private ed25519 key pair.
Created directory '/home/stepan/.ssh'.
Your identification has been saved in /home/stepan/.ssh/id_ed25519
Your public key has been saved in /home/stepan/.ssh/id_ed25519.pub
The key fingerprint is:
SHA256:ThPyrFe1NFrUnH0RMqcwDZO2giM/R0l0ADXOQVp4sBw stepan@lab
...
stepan@lab:~$ ls -la --color=never ~/.ssh
total 16
drwx------ 2 stepan stepan 4096 Nov 23 20:34 .
drwxr-xr-x 5 stepan stepan 4096 Nov 23 20:34 ..
-rw------- 1 stepan stepan  399 Nov 23 20:34 id_ed25519
-rw-r--r-- 1 stepan stepan   92 Nov 23 20:34 id_ed25519.pub
stepan@lab:~$ cat ~/.ssh/id_ed25519.pub >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys
stepan@lab:~$ cat > ~/.ssh/config <<'END'
> Host lab
>     HostName 127.0.0.1
>     User stepan
>     Port 2222
>     IdentityFile ~/.ssh/id_ed25519
>     ServerAliveInterval 60
> END
stepan@lab:~$ chmod 600 ~/.ssh/config
stepan@lab:~$ ssh -G lab | grep -E '^(hostname|user|port|identityfile|serveraliveinterval|stricthostkeychecking) '
user stepan
hostname 127.0.0.1
port 2222
stricthostkeychecking ask
serveraliveinterval 60
identityfile ~/.ssh/id_ed25519
```

`-N ''` задаёт пустую парольную фразу, это только для учебного стенда. `ssh-keygen` сам
выставил 700 на `~/.ssh` и 600 на закрытый ключ. Ключ в `authorized_keys` я дописал руками:
`ssh-copy-id` делает то же самое, но просит пароль, а у `stepan` в контейнере его нет.

## Первый вход и отпечаток

Перед первым входом посмотрел отпечаток ключа сервера, чтобы было с чем сверять:

```text
stepan@lab:~$ ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub
256 SHA256:g8tixyaBv/CHwA6iXtDvKCVze5JCT6Op2kcichzzBP4 root@lab (ED25519)
stepan@lab:~$ ssh lab 'whoami; hostname'
The authenticity of host '[127.0.0.1]:2222 ([127.0.0.1]:2222)' can't be established.
ED25519 key fingerprint is SHA256:g8tixyaBv/CHwA6iXtDvKCVze5JCT6Op2kcichzzBP4.
This key is not known by any other names.
Are you sure you want to continue connecting (yes/no/[fingerprint])? yes
Warning: Permanently added '[127.0.0.1]:2222' (ED25519) to the list of known hosts.
stepan
lab
stepan@lab:~$ ssh-keygen -F '[127.0.0.1]:2222' | head -n 2
# Host [127.0.0.1]:2222 found: line 1 
|1|nhLY5dyzftdwcmg7/C29Qh6L7j0=|YxhCPC9azxqR9rHGJkPjskJeMx0= ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIF5v6bMNZpnJfkJsuwNfkLTMGzwzuP0Bs+z0hmHWzdaY
stepan@lab:~$ ssh lab 'whoami; hostname'
stepan
lab
```

Отпечатки совпали, только после этого ответил `yes`. Сервер нестандартного порта записан в
`known_hosts` как `[127.0.0.1]:2222`, и искать его надо именно так. Имя хоста в файле
захешировано (`|1|...`), поэтому `ssh-keygen -F` удобнее, чем `grep`.

## Кавычки в удалённой команде

```text
stepan@lab:~$ ssh lab 'echo "remote: $SSH_CONNECTION"'
remote: 127.0.0.1 39470 127.0.0.1 2222
stepan@lab:~$ ssh lab "echo \"local: $SSH_CONNECTION\""
local: 
```

В одинарных кавычках `$SSH_CONNECTION` раскрыл сервер, там переменная есть. В двойных её
подставил мой Bash до отправки, а у меня локально она пустая.

## Файл одной командой, scp туда и обратно

```text
stepan@lab:~$ ssh lab 'mkdir -p ~/incoming && printf "created on server\n" > ~/incoming/remote.log'
stepan@lab:~$ printf 'report from client\n' > report.txt
stepan@lab:~$ scp report.txt lab:~/incoming/
report.txt                                    100%   19    46.7KB/s   00:00
stepan@lab:~$ scp lab:~/incoming/remote.log .
remote.log                                    100%   18    22.0KB/s   00:00
stepan@lab:~$ cat remote.log
created on server
stepan@lab:~$ ssh lab 'ls -l --color=never ~/incoming'
total 8
-rw-rw-r-- 1 stepan stepan 18 Nov 23 20:41 remote.log
-rw-rw-r-- 1 stepan stepan 19 Nov 23 20:42 report.txt
```

У `scp` строка прогресса перерисовывается на месте, здесь оставлено её последнее состояние.

## rsync

```text
stepan@lab:~$ mkdir -p project && printf 'v1\n' > project/a.txt && printf 'v1\n' > project/b.txt
stepan@lab:~$ rsync -av project/ lab:~/project-copy/
sending incremental file list
created directory /home/stepan/project-copy
./
a.txt
b.txt

sent 192 bytes  received 105 bytes  198.00 bytes/sec
total size is 6  speedup is 0.02
stepan@lab:~$ printf 'v2\n' > project/b.txt
stepan@lab:~$ rsync -av project/ lab:~/project-copy/
sending incremental file list
b.txt

sent 151 bytes  received 41 bytes  384.00 bytes/sec
total size is 6  speedup is 0.03
```

Второй раз ушёл только изменившийся `b.txt`. `rsync` должен стоять на обеих сторонах.

## Права на ключ и BatchMode

```text
stepan@lab:~$ ssh -o BatchMode=yes lab true; echo "BatchMode: $?"
BatchMode: 0
stepan@lab:~$ chmod 644 ~/.ssh/id_ed25519
stepan@lab:~$ ssh -o BatchMode=yes lab true; echo "код: $?"
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@         WARNING: UNPROTECTED PRIVATE KEY FILE!          @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Permissions 0644 for '/home/stepan/.ssh/id_ed25519' are too open.
It is required that your private key files are NOT accessible by others.
This private key will be ignored.
Load key "/home/stepan/.ssh/id_ed25519": bad permissions
stepan@127.0.0.1: Permission denied (publickey,password).
код: 255
stepan@lab:~$ chmod 600 ~/.ssh/id_ed25519
```

`BatchMode=yes` запрещает спрашивать пароль, так что код 0 значит: вход по ключу работает.
С правами 644 ключ проигнорирован, ssh вернул 255.

## Туннель -L и ssh -v

На «сервере» запустил веб-сервер только на `127.0.0.1:8000` и пробросил к себе порт 8080.
Потом всё погасил и посмотрел подробный вывод `ssh -v`:

```text
stepan@lab:~$ mkdir -p ~/site && printf 'hello from server\n' > ~/site/index.html
stepan@lab:~$ (cd ~/site && python3 -m http.server 8000 --bind 127.0.0.1 >/dev/null 2>&1 &)
stepan@lab:~$ ssh -N -f -L 8080:localhost:8000 lab
stepan@lab:~$ ss -ltn | grep -E ':(2222|8000|8080) '
LISTEN 0      128        127.0.0.1:8080      0.0.0.0:*          
LISTEN 0      5          127.0.0.1:8000      0.0.0.0:*          
LISTEN 0      128          0.0.0.0:2222      0.0.0.0:*          
LISTEN 0      128            [::1]:8080         [::]:*          
LISTEN 0      128             [::]:2222         [::]:*          
stepan@lab:~$ curl -s http://localhost:8080/index.html
hello from server
stepan@lab:~$ pkill -f 'ssh -N -f -L'; pkill -f http.server
stepan@lab:~$ Terminated
ssh -v lab true 2>&1 | grep -E 'Reading configuration|Connecting to|Server host key|Server accepts key|Authenticated to'
debug1: Reading configuration data /home/stepan/.ssh/config
debug1: Reading configuration data /etc/ssh/ssh_config
debug1: Connecting to 127.0.0.1 [127.0.0.1] port 2222.
debug1: Server host key: ssh-ed25519 SHA256:g8tixyaBv/CHwA6iXtDvKCVze5JCT6Op2kcichzzBP4
debug1: Server accepts key: /home/stepan/.ssh/id_ed25519 ED25519 SHA256:ThPyrFe1NFrUnH0RMqcwDZO2giM/R0l0ADXOQVp4sBw explicit
Authenticated to 127.0.0.1 ([127.0.0.1]:2222) using "publickey".
```

Порт 8080 слушает сам `ssh` (`-f` увёл его в фон после входа), запрос по туннелю дошёл до
8000. На одной машине это выглядит игрушечно, но схема та же, что с настоящим сервером.

`Terminated` про убитый `http.server` Bash напечатал уже после приглашения, поэтому
следующая команда в записи стоит на строке без него. По строкам `ssh -v` видно план
проверки со слайда: какой конфиг прочитан, куда подключились, какой ключ у сервера (тот же
отпечаток, что я сверял при первом входе), принят ли мой ключ.

## Что понял

`ssh -G` и `ssh -v` отвечают почти на все вопросы «почему не подключается» без гаданий.
Права на закрытый ключ ssh проверяет сам: при 644 ключ просто не используется, и вход по
ключу ломается.
