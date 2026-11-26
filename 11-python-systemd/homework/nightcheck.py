#!/usr/bin/env python3
"""nightcheck.py — ночной сторож: размер каталога и ответ сайта.

Проверяет, что каталог не вырос больше лимита и что каждый адрес отвечает
кодом 2xx или 3xx. Пишет отчёт в JSON с датой и временем проверки.

Запуск:
    python3 nightcheck.py --dir ~/www --max-mb 500 --url http://127.0.0.1:8000/
    python3 nightcheck.py --dir /var/log --max-mb 200 --url https://example.com \\
        --url http://127.0.0.1:8000/ --report /tmp/report.json

Код возврата: 0 — всё в порядке, 1 — что-то не так (подробности в stderr и
в отчёте), 2 — неверные аргументы.
"""

import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.request
from datetime import datetime
from pathlib import Path

MB = 1024 * 1024


def dir_size(path):
    """Суммарный размер файлов в каталоге, в байтах. Ссылки не считаются."""
    total = 0
    for root, _dirs, files in os.walk(path):
        for name in files:
            full = os.path.join(root, name)
            try:
                if not os.path.islink(full):
                    total += os.path.getsize(full)
            except OSError:
                continue  # файл удалили, пока мы шли по каталогу
    return total


def check_dir(path, max_mb):
    """Проверка каталога: словарь для отчёта, поле ok и причина, если не ok."""
    result = {"check": "dir", "path": str(path), "max_mb": max_mb}
    if not path.is_dir():
        result.update(ok=False, error="каталога нет")
        return result
    size_mb = dir_size(path) / MB
    result["size_mb"] = round(size_mb, 1)
    result["ok"] = size_mb <= max_mb
    if not result["ok"]:
        result["error"] = f"занято {size_mb:.1f} МБ при лимите {max_mb:g} МБ"
    return result


def check_url(url, timeout):
    """Проверка адреса: код ответа и время, или ошибка, если не ответил."""
    result = {"check": "url", "url": url}
    started = time.monotonic()
    try:
        with urllib.request.urlopen(url, timeout=timeout) as response:
            code = response.status
    except urllib.error.HTTPError as e:
        code = e.code  # сервер ответил, но ошибкой
    except (urllib.error.URLError, TimeoutError, ValueError) as e:
        reason = getattr(e, "reason", e)
        result.update(ok=False, error=f"не ответил: {reason}")
        return result
    result["status"] = code
    result["ms"] = round((time.monotonic() - started) * 1000)
    result["ok"] = 200 <= code < 400
    if not result["ok"]:
        result["error"] = f"ответил кодом {code}"
    return result


def write_report(path, report):
    """Записать отчёт целиком: сначала во временный файл, потом подменить."""
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + ".tmp")
    tmp.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                   encoding="utf-8")
    tmp.replace(path)


def describe(result):
    """Одна строка о проверке для вывода и журнала."""
    name = result.get("path") or result.get("url")
    if not result["ok"]:
        return f"ПЛОХО  {name}: {result['error']}"
    if result["check"] == "dir":
        return f"ok     {name}: {result['size_mb']} МБ из {result['max_mb']:g}"
    return f"ok     {name}: {result['status']} за {result['ms']} мс"


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Ночная проверка диска и сайта")
    parser.add_argument("--dir", type=Path, required=True,
                        help="каталог, размер которого проверяем")
    parser.add_argument("--max-mb", type=float, required=True,
                        help="сколько мегабайт ему можно занимать")
    parser.add_argument("--url", action="append", required=True,
                        help="адрес для проверки, можно несколько раз")
    parser.add_argument("--timeout", type=float, default=5,
                        help="сколько секунд ждать ответа (по умолчанию 5)")
    parser.add_argument("--report", type=Path, default=Path("report.json"),
                        help="куда писать отчёт (по умолчанию report.json)")
    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)
    checks = [check_dir(args.dir.expanduser(), args.max_mb)]
    checks += [check_url(url, args.timeout) for url in args.url]
    ok = all(c["ok"] for c in checks)

    report = {
        "date": datetime.now().astimezone().isoformat(timespec="seconds"),
        "ok": ok,
        "checks": checks,
    }
    try:
        write_report(args.report, report)
    except OSError as e:
        print(f"не могу записать отчёт {args.report}: {e.strerror}", file=sys.stderr)
        return 1

    # flush: stdout и stderr идут в журнал разными потоками, без него
    # строка об ошибке может обогнать строки, напечатанные раньше.
    for c in checks:
        print(describe(c), file=sys.stdout if c["ok"] else sys.stderr, flush=True)
    bad = sum(not c["ok"] for c in checks)
    print(f"итог: {'всё в порядке' if ok else f'проблем: {bad}'}, отчёт: {args.report}", flush=True)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
