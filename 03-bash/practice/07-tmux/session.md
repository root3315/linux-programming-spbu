# Практика 7. tmux: восстановите сессию

Шесть шагов со слайда: сессия `bash-lab`, два окна, панели, отсоединение, `send-keys`,
удаление. Критерий: различать закрытие терминала, отсоединение и удаление сессии.

Сеанс записывался программой `script`, поэтому всё, что можно, я делал командами tmux из
строки, а не клавишами внутри сессии: при подключении tmux перерисовывает весь экран, и в
записи остаётся каша из управляющих кодов. Клавиши проверил один раз, на отсоединении.

## Окна по умолчанию и конфиг

```text
stepan@lab:~$ tmux -V
tmux 3.4
stepan@lab:~$ tmux new -d -s probe && tmux list-windows -t probe && tmux kill-session -t probe
0: bash* (1 panes) [80x24] [layout b25d,80x24,0,0,0] @0 (active)
stepan@lab:~$ tmux list-keys -T prefix r
bind-key -T prefix r refresh-client
```

Без конфига первое окно имеет номер 0, а слайд обращается к `bash-lab:1.0` как к первому
окну. И `префикс, r` по умолчанию только перерисовывает экран, конфиг он не перечитывает.
Поэтому сначала положил [tmux.conf](tmux.conf) со слайда в `~/.tmux.conf` и добавил строку
`bind r source-file ~/.tmux.conf`. Префикс в нём `Ctrl+A`.

## 1–3. Сессия, окна, панели

```text
stepan@lab:~$ tmux new -d -s bash-lab -n work
stepan@lab:~$ tmux new-window -t bash-lab -n logs
stepan@lab:~$ tmux rename-window -t bash-lab:1 code
stepan@lab:~$ tmux list-windows -t bash-lab
1: code- (1 panes) [80x24] [layout b25d,80x24,0,0,0] @0
2: logs* (1 panes) [80x24] [layout b25e,80x24,0,0,1] @1 (active)
stepan@lab:~$ tmux split-window -h -t bash-lab:1
stepan@lab:~$ tmux send-keys -t bash-lab:1.0 'sleep 1000' Enter
stepan@lab:~$ tmux send-keys -t bash-lab:1.1 'date' Enter
stepan@lab:~$ tmux list-panes -t bash-lab:1 -F '#{session_name}:#{window_index}.#{pane_index} #{pane_current_command}'
bash-lab:1.0 sleep
bash-lab:1.1 bash
stepan@lab:~$ tmux show -g prefix
prefix C-a
```

`-d` создаёт сессию сразу отсоединённой. Окна стали 1 и 2, как на слайде. Звёздочка у
`logs` значит текущее окно, минус у `code` предыдущее.

## 4. Отсоединение и возврат

Подключился `tmux attach -t bash-lab`, посмотрел на панели, нажал `Ctrl+A`, отпустил, `d`.
Перерисовку экрана из записи убрал, осталась строка состояния и сообщение при выходе:

```text
stepan@lab:~$ tmux attach -t bash-lab
...
[bash-lab]1:code- 2:logs*                                  "lab" 20:59 23-Nov-26
...
[detached (from session bash-lab)]
stepan@lab:~$ tmux ls
bash-lab: 2 windows (created Mon Nov 23 20:56:06 2026)
```

## 5. send-keys в панель, где идёт sleep

Строки после `capture-pane` это содержимое панели, а не мои команды.

```text
stepan@lab:~$ tmux send-keys -t bash-lab:1.0 'date' Enter
stepan@lab:~$ tmux capture-pane -p -t bash-lab:1.0 | grep -v '^$'
stepan@lab:~$ sleep 1000
date
stepan@lab:~$ tmux send-keys -t bash-lab:1.0 C-c
stepan@lab:~$ tmux capture-pane -p -t bash-lab:1.0 | grep -v '^$'
stepan@lab:~$ sleep 1000
date
^C
stepan@lab:~$
stepan@lab:~$ tmux capture-pane -p -t bash-lab:1.1 | grep -v '^$'
stepan@lab:~$ date
Mon Nov 23 20:57:58 UTC 2026
stepan@lab:~$
```

Тут я ошибся в предсказании. Ждал дату в панели 1.0, а там идёт `sleep 1000`: `send-keys`
просто напечатал `date` в терминал занятой программы, и ничего не выполнилось. После `C-c`
`sleep` умер, но и набранное заранее `date` пропало, Bash его не выполнил. Дата есть только
в панели 1.1, куда я отправил её на шаге 3. `capture-pane -p` удобен как способ прочитать
панель, не подключаясь к ней.

## Закрытие терминала против отсоединения

Хотел проверить, что будет с сессией при закрытом терминале: подключить клиент в фоне и
послать ему `SIGHUP`, как при закрытии окна терминала или обрыве SSH. Первая попытка:

```text
stepan@lab:~$ script -qc 'tmux attach -t bash-lab' /dev/null > /dev/null &
[1] 5049
stepan@lab:~$ jobs
[1]+  Stopped                 script -qc 'tmux attach -t bash-lab' /dev/null > /dev/null
stepan@lab:~$ kill %1

[1]+  Stopped                 script -qc 'tmux attach -t bash-lab' /dev/null > /dev/null
stepan@lab:~$ jobs
[1]+  Terminated              script -qc 'tmux attach -t bash-lab' /dev/null > /dev/null
```

Фоновое задание сразу встало в `Stopped`: оно попыталось читать с моего терминала и
получило `SIGTTIN`. Дал ему вход из FIFO, тогда терминал ему не нужен:

```text
stepan@lab:~$ mkfifo /tmp/keys; exec 3<>/tmp/keys
stepan@lab:~$ script -qfc 'tmux attach -t bash-lab' /dev/null < /tmp/keys > /dev/null 2>&1 &
[1] 5051
stepan@lab:~$ tmux list-clients; pgrep -af "tmux attach"
/dev/pts/4: bash-lab [80x24 xterm] (attached,focused,UTF-8)
5051 script -qfc tmux attach -t bash-lab /dev/null
5052 tmux attach -t bash-lab
stepan@lab:~$ pkill -HUP -f 'tmux attach'; sleep 1; jobs
[1]+  Hangup                  script -qfc 'tmux attach -t bash-lab' /dev/null < /tmp/keys > /dev/null 2>&1
stepan@lab:~$ tmux list-clients; tmux ls
bash-lab: 2 windows (created Mon Nov 23 20:56:06 2026)
```

Клиента больше нет, а сессия с двумя окнами на месте.

## 6. Удаление

```text
stepan@lab:~$ tmux kill-session -t bash-lab
stepan@lab:~$ tmux ls; echo "код: $?"
no server running on /tmp/tmux-1001/default
код: 1
```

## Что понял

| Действие | Клиент | Сессия и программы в ней |
|---|---|---|
| закрыть терминал (SIGHUP клиенту) | умер | живут |
| отсоединиться (`префикс, d`) | вышел сам | живут |
| `tmux kill-session` | отключён | убиты |

Что подключённый клиент при `kill-session` отключается, знаю по документации, сам не
проверял: в тот момент клиентов не было. После последней сессии завершается и сам сервер
tmux, поэтому `tmux ls` пишет `no server running`.
