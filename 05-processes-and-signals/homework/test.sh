#!/usr/bin/env bash
# Проверки minishell без терминала: команды идут в stdin, вывод сравниваю с bash.
#   ./test.sh [./minishell]
# Ctrl+C имитирую так: запускаю minishell фоном в своей группе процессов
# и шлю SIGINT всей группе (kill -INT -PGID), как это делает терминал.
# ls и ps | grep здесь нарочно: сравниваю с тем, что выдаёт bash на ls | wc -l.
# shellcheck disable=SC2009,SC2012
set -u

SHELL_BIN=$(realpath "${1:-./minishell}")
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
# Если оболочка умрёт, запись в её FIFO даст SIGPIPE. Пусть лучше echo вернёт
# ошибку, а скрипт дойдёт до итога и честно напишет, сколько провалено.
trap '' PIPE
fails=0

check() { # имя, ожидаемое, полученное
    if [[ $2 == "$3" ]]; then
        printf 'ok    %s\n' "$1"
    else
        printf 'FAIL  %s: ждал [%s], получил [%s]\n' "$1" "$2" "$3"
        fails=$((fails + 1))
    fi
}

zombies_of() { # зомби-дети процесса $1
    ps -o stat= --ppid "$1" | grep -c '^Z'
}

all_zombies() { # зомби во всей системе: посторонние тоже бывают, сравниваю до/после
    ps -eo stat= | grep -c '^Z'
}
zombies_before=$(all_zombies)

check "ls | wc -l" "$(ls /etc | wc -l)" "$(echo 'ls /etc | wc -l' | "$SHELL_BIN")"
check "ls | sort | head -3" "$(ls /etc | sort | head -3)" \
    "$(echo 'ls /etc | sort | head -3' | "$SHELL_BIN")"

printf 'ls /etc > %s/out.txt\n' "$tmp" | "$SHELL_BIN"
check "> файл" "$(ls /etc)" "$(cat "$tmp/out.txt")"

check "cd и pwd" "/tmp" "$(printf 'cd /tmp\npwd\n' | "$SHELL_BIN")"
check "cd без аргумента" "$HOME" "$(printf 'cd\npwd\n' | "$SHELL_BIN")"

echo 'exit 3' | "$SHELL_BIN"
check "exit 3" "3" "$?"
echo 'nosuchcmd' | "$SHELL_BIN" 2>/dev/null
check "нет команды: код 127" "127" "$?"
echo 'exit abc' | "$SHELL_BIN" 2>/dev/null
check "exit abc: код 2" "2" "$?"
check "exit 1 2: не выходит" "still here" \
    "$(printf 'exit 1 2\necho still here\n' | "$SHELL_BIN" 2>/dev/null)"

# Неподдерживаемое оболочка должна отвергать с кодом 2 и ничего не создавать.
mkdir "$tmp/empty"
for line in 'ls>out' 'ls 2>err' 'ls >> out' 'cat < in' 'cd / | cat' 'cd > out'; do
    (cd "$tmp/empty" && echo "$line" | "$SHELL_BIN" 2>/dev/null)
    check "отказ: $line" "2" "$?"
done
check "после отказов файлов нет" "" "$(ls -A "$tmp/empty")"

# Команды читаются из stdin по байту: остаток ввода достаётся запущенной команде.
check "cat читает остаток stdin" "hello" "$(printf 'cat\nhello\n' | "$SHELL_BIN")"

# Зомби: пока идёт `true | sleep 2`, у оболочки не должно быть детей в Z.
# set -m нужен по двум причинам: фоновая задача получает свою группу процессов,
# и bash не ставит ей SIGINT в SIG_IGN. Без него проверка SIGINT проходит даже
# у оболочки вообще без обработчика.
mkfifo "$tmp/in"
set -m
"$SHELL_BIN" <"$tmp/in" >"$tmp/log" 2>&1 &
set +m
msh=$!
exec 3>"$tmp/in"
echo 'true | sleep 2' >&3
sleep 1
check "зомби во время конвейера" "0" "$(zombies_of "$msh")"

# SIGINT всей группе во время sleep 30: sleep умирает, оболочка живёт.
echo 'sleep 30' >&3
sleep 2
kill -INT -- "-$msh"
sleep 0.5
check "оболочка жива после SIGINT" "yes" "$(kill -0 "$msh" 2>/dev/null && echo yes)"
echo 'echo alive' >&3
sleep 0.5
check "после SIGINT команды выполняются" "alive" "$(tail -n 1 "$tmp/log")"

exec 3>&-   # EOF на stdin: оболочка выходит
wait "$msh"
check "выход по EOF с кодом последней команды" "0" "$?"
check "зомби после выхода" "0" "$(($(all_zombies) - zombies_before))"

if ((fails)); then
    echo "провалено: $fails"
    exit 1
fi
echo "все проверки прошли"
