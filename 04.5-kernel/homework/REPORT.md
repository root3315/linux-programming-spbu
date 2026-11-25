# Отчёт по ДЗ 4.5. Паспорт своей машины

Учебная машина: WSL2 на Windows 11, Ubuntu 24.04.5, пользователь `stepan` без sudo. Всё
снято скриптом [passport.sh](passport.sh), полный вывод в [passport.txt](passport.txt).

## 1. Версия и сборка

```text
$ uname -r
6.18.58.1-microsoft-standard-WSL2
$ uname -a
Linux devbox 6.18.58.1-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Tue Oct 27 00:24:11 UTC 2026 x86_64 x86_64 x86_64 GNU/Linux
```

Версия сообщества 6.18.58, `.1` и `-microsoft-standard-WSL2` добавила Microsoft. `SMP`:
поддержка нескольких процессоров, `PREEMPT_DYNAMIC`: режим вытеснения выбирается при загрузке.
Собрано 27 октября, x86_64, имя машины `devbox`.

## 2. Три опции конфигурации

```text
$ zgrep -E '^CONFIG_EXT4_FS=|^CONFIG_BTRFS_FS=|^# CONFIG_WLAN_VENDOR_INTEL ' /proc/config.gz
# CONFIG_WLAN_VENDOR_INTEL is not set
CONFIG_EXT4_FS=y
CONFIG_BTRFS_FS=m
```

`/boot` в WSL пустой: ядро грузит сам WSL, без GRUB. Зато ядро собрано с
`CONFIG_IKCONFIG_PROC=y` и отдаёт свой конфиг сжатым в `/proc/config.gz`, отсюда `zgrep`.
ext4 вкомпилирован (на нём корень), btrfs собран модулем, драйверов Wi-Fi от Intel нет вовсе:
виртуалке Wi-Fi не нужен. Всего строк `CONFIG_` 2723, из них `=y` 1825, `=m` 771.

## 3. Модули

```text
$ lsmod | tail -n +2 | wc -l
29
$ find /lib/modules/6.18.58.1-microsoft-standard-WSL2 -name '*.ko*' | wc -l
748
```

Загружено 29 из 748 (`tail -n +2` отрезает заголовок). Загруженные в основном netfilter и мост
(`xt_*`, `nf_*`, `bridge`, `br_netfilter`), их подтянул Docker.

## 4. modinfo br_netfilter

```text
$ modinfo -F filename br_netfilter
/lib/modules/6.18.58.1-microsoft-standard-WSL2/kernel/net/bridge/br_netfilter.ko
$ modinfo -F license br_netfilter
GPL
$ modinfo -F depends br_netfilter
bridge
```

Модуль пропускает трафик моста через правила файрвола, это нужно Docker. Файл `.ko` без
сжатия, лицензия GPL. Зависит от `bridge`, поэтому у `bridge` в `lsmod` Used by `1 br_netfilter`.

## 5. /proc/cmdline

```text
$ cat /proc/cmdline
initrd=\initrd.img WSL_ROOT_INIT=1 panic=-1 nr_cpus=8 hv_utils.timesync_implicit=1 printk.devkmsg=on page_reporting.page_reporting_order=5 hv_storvsc.storvsc_max_hw_queues=4 console=hvc0 debug pty.legacy_count=0 WSL_ENABLE_CRASH_DUMP=1
```

Строку составляет WSL, а не GRUB, поэтому нет `root=`, `ro`, `quiet`, `splash`.

| Параметр | Что значит |
|---|---|
| `initrd=\initrd.img` | образ initrd; обратный слэш в стиле EFI-загрузчика, файл лежит в каталоге WSL на стороне Windows |
| `WSL_ROOT_INIT=1`, `WSL_ENABLE_CRASH_DUMP=1` | ядру неизвестны, оно отдаёт их init (пишет об этом при загрузке); флаги WSL, второй по имени: дамп при падении |
| `panic=-1` | при kernel panic перезагрузиться сразу |
| `nr_cpus=8` | не больше 8 процессоров, `nproc` у меня 8 |
| `hv_utils.timesync_implicit=1` | параметр `hv_utils` (Hyper-V): подтягивать часы к Windows, если отстали (например, после сна) |
| `printk.devkmsg=on` | писать в журнал ядра через `/dev/kmsg` без ограничения частоты (по умолчанию `ratelimit`) |
| `page_reporting.page_reporting_order=5` | отдавать гипервизору свободную память блоками не меньше 2^5 страниц (128 КБ) |
| `hv_storvsc.storvsc_max_hw_queues=4` | драйвер виртуальных дисков Hyper-V: до 4 очередей |
| `console=hvc0` | консоль ядра: виртуальная консоль Hyper-V |
| `debug` | подробные сообщения ядра на консоль при загрузке |
| `pty.legacy_count=0` | не создавать старые BSD-терминалы `/dev/ptyXX` |

## 6. Последние пять строк dmesg

```text
$ dmesg | tail -5
[24153.011979] vethfba8ccf: renamed from eth0
[24153.042289] docker0: port 2(veth4dbec11) entered disabled state
[24153.042530] veth4dbec11 (unregistering): left allmulticast mode
[24153.042535] veth4dbec11 (unregistering): left promiscuous mode
[24153.042537] docker0: port 2(veth4dbec11) entered disabled state
```

`dmesg_restrict` равен 0, поэтому `dmesg` читается без root. В скобках секунды с загрузки,
около 6,7 часа. Это остановился Docker-контейнер: его `eth0` (один конец пары veth) вернулся
в хост под именем `vethfba8ccf`, порт 2 моста `docker0` выключился, а второй конец пары
удаляется и выходит из режимов, в которые его ставил мост. Ядро у WSL общее, поэтому события
Docker видны и в моём `dmesg`.

## Почему ядро нельзя править на живой системе

1. Ядро работает в одном адресном пространстве без защиты памяти внутри себя, и ошибка в
   модуле даёт kernel panic всей машины. У меня `panic=-1`: WSL сразу перезагрузится, и разом
   пропадут все дистрибутивы и контейнеры Docker вместе с несохранённой работой.
2. Части ядра держат друг друга. У меня `bridge` используется `br_netfilter`, а на `bridge`
   построен `docker0`. `rmmod bridge` откажет, модуль занят, а выгрузка силой или подмена на
   ходу оставит без сети все контейнеры сразу. Перезапустить ядро по частям, как службу, нельзя.
3. Ошибку может быть нечем исправить. Модуль под другую сборку не загрузится
   (`invalid module format`), неподписанный не пустит Secure Boot, а после ошибки в `/boot`
   или в `root=` машина не загрузится, и чинить придётся с флешки.

## Что было непонятно

- По слайду ждал `/boot/config-*`, а в WSL `/boot` пустой. На слайде `/proc/config.gz` назван
  ловушкой, а у меня работает только он.
- В Docker-контейнере `uname -r` и `/proc/cmdline` те же, но `/lib/modules` нет, а `dmesg` даёт
  `Operation not permitted` даже от root: нет `CAP_SYSLOG`, с `--cap-add SYSLOG` заработало.
- `journalctl -k` от обычного пользователя выдаёт `-- No entries --`: системный журнал видят
  только группы `adm` и `systemd-journal`.
- Хвост `dmesg` каждый раз разный. В одном прогоне это были строки `systemd-journald` (journald
  пишет в журнал ядра через `/dev/kmsg`, а `printk.devkmsg=on` снимает лимит на такие строки),
  в другом остановка контейнера, как выше.
