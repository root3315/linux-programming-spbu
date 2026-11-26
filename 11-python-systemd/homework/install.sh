#!/bin/sh
# Ставит nightcheck.service и nightcheck.timer в свою сессию и включает таймер.
#   ./install.sh
# Root не нужен: юниты ложатся в ~/.config/systemd/user.
set -eu

PROJECT=$(cd "$(dirname "$0")" && pwd)
UNITS="$HOME/.config/systemd/user"
CONFIG="$HOME/.config/nightcheck.env"

mkdir -p "$UNITS"
for unit in nightcheck.service nightcheck.timer; do
    sed "s|@PROJECT@|$PROJECT|g" "$PROJECT/$unit" > "$UNITS/$unit"
    echo "установлен: $UNITS/$unit"
done

if [ -e "$CONFIG" ]; then
    echo "настройки уже есть: $CONFIG"
else
    cp "$PROJECT/nightcheck.env.example" "$CONFIG"
    echo "настройки: $CONFIG (поправьте каталог и адрес)"
fi

systemd-analyze --user verify "$UNITS/nightcheck.service" "$UNITS/nightcheck.timer"
systemctl --user daemon-reload
systemctl --user enable --now nightcheck.timer
systemctl --user list-timers nightcheck.timer --no-pager
