#!/bin/sh
# Выключает таймер и удаляет юниты, которые поставил install.sh.
#   ./uninstall.sh          настройки и последний отчёт остаются
#   ./uninstall.sh --purge  удалить и их
set -eu

UNITS="$HOME/.config/systemd/user"

systemctl --user disable --now nightcheck.timer 2>/dev/null || true
systemctl --user stop nightcheck.service 2>/dev/null || true
rm -f "$UNITS/nightcheck.service" "$UNITS/nightcheck.timer"
systemctl --user daemon-reload
systemctl --user reset-failed nightcheck.service nightcheck.timer 2>/dev/null || true
echo "таймер выключен, юниты удалены"

if [ "${1:-}" = "--purge" ]; then
    rm -f "$HOME/.config/nightcheck.env"
    rm -rf "${XDG_STATE_HOME:-$HOME/.local/state}/nightcheck"
    echo "настройки и отчёт удалены"
fi
