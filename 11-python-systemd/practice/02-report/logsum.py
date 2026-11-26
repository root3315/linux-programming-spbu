#!/usr/bin/env python3
"""logsum.py — сколько в журнале строк каждого уровня и сколько это в процентах.

Запуск:
    python3 logsum.py app.log
    python3 logsum.py app.log --top 2
    python3 logsum.py app.log --save report.json
    python3 logsum.py app.log other.log        # два журнала и разница между ними

Код возврата: 0 — всё посчитано, 1 — какой-то файл не прочитался.
"""

import argparse
import json
import sys
from collections import Counter


def count_levels(path):
    """Вернуть Counter «уровень → сколько строк».

    Уровень — первое слово строки, пустые строки пропускаются. Байты, которые
    не читаются как UTF-8, заменяются на «?»: битая строка не роняет подсчёт.
    """
    levels = Counter()
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            words = line.split()
            if words:
                levels[words[0]] += 1
    return levels


def make_report(levels, top=0):
    """Список (уровень, строк, процент), от большего процента к меньшему."""
    total = sum(levels.values())
    if total == 0:
        return [], 0
    rows = [(name, count, count / total * 100) for name, count in levels.items()]
    # При равном проценте порядок по имени, чтобы вывод не зависел от файла.
    rows.sort(key=lambda row: (-row[2], row[0]))
    if top > 0:
        rows = rows[:top]
    return rows, total


def print_report(rows, total):
    if total == 0:
        print("журнал пустой")
        return
    width = max(len(name) for name, _, _ in rows)
    for name, count, percent in rows:
        print(f"{name:<{width}}  {count:>3}  {percent:5.1f}%")
    print(f"всего строк: {total}")


def save_report(path, levels):
    """Сохранить отчёт в JSON: счётчики, проценты и итог."""
    rows, total = make_report(levels)
    report = {
        "total": total,
        "levels": {name: {"count": count, "percent": round(percent, 1)}
                   for name, count, percent in rows},
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(report, f, ensure_ascii=False, indent=2)
        f.write("\n")


def print_difference(name_a, a, name_b, b):
    """Таблица: сколько строк каждого уровня в двух журналах и разница."""
    print(f"\nуровень  {name_a} -> {name_b}")
    for level in sorted(a.keys() | b.keys()):
        delta = b[level] - a[level]
        print(f"{level:<7}  {a[level]:>3} -> {b[level]:<3} ({delta:+d})")


def main(argv):
    parser = argparse.ArgumentParser(description="Уровни в журнале с процентами")
    parser.add_argument("files", nargs="+", help="один или два файла журнала")
    parser.add_argument("--top", type=int, default=0,
                        help="показать только N самых частых уровней")
    parser.add_argument("--save", metavar="FILE",
                        help="сохранить отчёт первого файла в JSON")
    args = parser.parse_args(argv)
    if len(args.files) > 2:
        parser.error("сравнить можно не больше двух файлов")

    counted = []
    for path in args.files:
        try:
            counted.append((path, count_levels(path)))
        except FileNotFoundError:
            print(f"нет такого файла: {path}", file=sys.stderr)
            return 1
        except (PermissionError, IsADirectoryError):
            print(f"не могу прочитать: {path}", file=sys.stderr)
            return 1

    for i, (path, levels) in enumerate(counted):
        if len(counted) > 1:
            print(("\n" if i else "") + f"== {path}")
        print_report(*make_report(levels, args.top))

    if len(counted) == 2:
        print_difference(counted[0][0], counted[0][1], counted[1][0], counted[1][1])

    if args.save:
        save_report(args.save, counted[0][1])
        print(f"отчёт сохранён: {args.save}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
