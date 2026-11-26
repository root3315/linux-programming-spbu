#!/usr/bin/env python3
"""logsum.py — считает, сколько в журнале строк каждого уровня.

Это та же задача, что в лекции 03 решалась конвейером
    cut -d' ' -f1 app.log | sort | uniq -c | sort -nr
только записанная на Python: читаемо и легко расширяется.

Запуск:
    python3 logsum.py app.log
    python3 logsum.py app.log --top 3
    python3 logsum.py app.log --json
"""

import argparse
import json
import sys
from collections import Counter


def count_levels(path):
    """Вернуть словарь «уровень → сколько раз встретился».

    Уровень — первое слово строки. Пустые строки пропускаем.
    Если файл не открывается, FileNotFoundError полетит наверх:
    решение, что с ним делать, принимает вызывающий код.
    """
    levels = Counter()
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            levels[line.split()[0]] += 1
    return levels


def print_table(rows, total):
    width = max(len(name) for name, _ in rows)
    for name, count in rows:
        print(f"{name:<{width}}  {count}")
    print(f"\nвсего строк: {total}")


def print_json(rows, total):
    report = {"levels": dict(rows), "total": total}
    print(json.dumps(report, ensure_ascii=False, indent=2))


def main(argv):
    """Разобрать аргументы, посчитать, напечатать таблицу или JSON."""
    parser = argparse.ArgumentParser(description="Счётчик уровней в журнале")
    parser.add_argument("file", help="файл журнала")
    parser.add_argument("--top", type=int, default=0,
                        help="показать только N самых частых уровней")
    parser.add_argument("--json", action="store_true",
                        help="напечатать результат в JSON вместо таблицы")
    args = parser.parse_args(argv)

    try:
        levels = count_levels(args.file)
    except FileNotFoundError:
        print(f"нет такого файла: {args.file}", file=sys.stderr)
        return 1
    except PermissionError:
        print(f"нет прав на чтение: {args.file}", file=sys.stderr)
        return 1

    if not levels:
        print("журнал пустой")
        return 0

    # Сортируем по количеству: от большего к меньшему.
    rows = levels.most_common(args.top if args.top > 0 else None)
    total = sum(levels.values())

    if args.json:
        print_json(rows, total)
    else:
        print_table(rows, total)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
