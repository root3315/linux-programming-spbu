#!/bin/sh
# Паспорт ядра: шесть пунктов ДЗ 4.5. Только чтение, без sudo, ничего не меняет.
# Запуск: ./passport.sh [модуль для modinfo] > passport.txt
set -u

module=${1:-br_netfilter}
release=$(uname -r)
moddir=/lib/modules/$release

section() {
    printf '\n=== %s ===\n' "$1"
}

run() {
    printf '$ %s\n' "$*"
    sh -c "$*" 2>&1
}

section "1. Версия ядра"
run "uname -r"
run "uname -a"
run "cat /proc/version"

section "2. Конфигурация"
config=/boot/config-$release
if [ -r "$config" ]; then
    show="grep"
else
    run "ls -l $config"
    echo "# /boot/config-* нет, беру конфигурацию из самого ядра: /proc/config.gz"
    config=/proc/config.gz
    show="zgrep"
fi
run "$show -c '^CONFIG_' $config"
run "$show -c '=y\$' $config"
run "$show -c '=m\$' $config"
run "$show -c 'is not set\$' $config"
run "$show -E '^CONFIG_EXT4_FS=|^CONFIG_BTRFS_FS=|^# CONFIG_WLAN_VENDOR_INTEL ' $config"

section "3. Модули"
run "lsmod | tail -n +2 | wc -l"
run "find $moddir -name '*.ko*' | wc -l"

section "4. modinfo $module"
run "modinfo -F filename $module"
run "modinfo -F license $module"
run "modinfo -F depends $module"

section "5. Параметры загрузки"
run "cat /proc/cmdline"
echo "# по одному параметру в строке:"
tr ' ' '\n' < /proc/cmdline | grep -v '^$'

section "6. Последние строки журнала ядра"
run "cat /proc/sys/kernel/dmesg_restrict"
if dmesg > /dev/null 2>&1; then
    run "dmesg | tail -5"
else
    run "dmesg | tail -5"
    echo "# dmesg не отдал журнал, пробую journalctl -k"
    run "journalctl -k -b --no-pager -q | tail -5"
fi
