#!/bin/sh
# zombies.sh — задание №5 из РПД: вывести все зомби-процессы.
# Мой вариант через ps. В Chapter 05 есть zombie-finder.sh, он читает /proc
# напрямую; здесь то же самое, но для каждого зомби печатается ещё и имя
# родителя: убрать зомби может только он, вызвав wait.
#   ./zombies.sh        код выхода 0, если зомби нет, и 1, если есть

printf '%-8s %-8s %-16s %s\n' PID PPID COMMAND PARENT
found=0

# comm стоит последним: имя с пробелами целиком попадёт в $comm.
# Цикл читает из here-doc, а не из ps | while: в конвейере цикл идёт
# в подоболочке, и счётчик found после него снова был бы 0.
while read -r pid ppid stat comm; do
    case $stat in
        Z*) ;;
        *) continue ;;
    esac
    parent=$(ps -o comm= -p "$ppid")
    printf '%-8s %-8s %-16s %s\n' "$pid" "$ppid" "$comm" "$parent"
    found=$((found + 1))
done <<EOF_PS
$(ps -eo pid=,ppid=,stat=,comm=)
EOF_PS

if [ "$found" -eq 0 ]; then
    echo "(зомби нет)"
    exit 0
fi
exit 1
