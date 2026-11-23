"""Собирает конспект notes.md в notes.pdf.

    python tools/notes/build.py 04-build-toolchain/notes.md
    python tools/notes/build.py --all

Markdown превращается в HTML со стилем style.css, а PDF печатает Chrome
в режиме headless. Путь к Chrome можно задать переменной CHROME.
"""

import argparse
import html
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from markdown_it import MarkdownIt
from mdit_py_plugins.front_matter import front_matter_plugin
from pygments import highlight
from pygments.formatters import HtmlFormatter
from pygments.lexers import get_lexer_by_name
from pygments.util import ClassNotFound

TOOL_DIR = Path(__file__).resolve().parent
REPO_ROOT = TOOL_DIR.parent.parent

CHROME_CANDIDATES = [
    r"C:\Program Files\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
    os.path.expandvars(r"%LOCALAPPDATA%\Google\Chrome\Application\chrome.exe"),
    "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
    "google-chrome",
    "google-chrome-stable",
    "chromium",
    "chromium-browser",
]

# Цитата, которая начинается с жирного «Запомнить» или «Грабли», становится врезкой.
CALLOUTS = {"Запомнить": "remember", "Грабли": "pitfall"}


def resolve_executable(name):
    return shutil.which(name) or (name if Path(name).is_file() else None)


def find_chrome():
    custom = os.environ.get("CHROME")
    if custom:
        return resolve_executable(custom) or sys.exit(f"CHROME={custom} не найден")
    for candidate in CHROME_CANDIDATES:
        path = resolve_executable(candidate)
        if path:
            return path
    sys.exit("Chrome не найден: укажите путь в переменной CHROME")


def parse_front_matter(text):
    meta = {}
    for line in text.splitlines():
        if line.lstrip().startswith("#"):
            continue
        key, sep, value = line.partition(":")
        if sep:
            meta[key.strip()] = value.strip().strip("\"'")
    return meta


def highlight_code(code, lang, _attrs):
    try:
        lexer = get_lexer_by_name(lang or "text")
    except ClassNotFound:
        lexer = get_lexer_by_name("text")
    return highlight(code, lexer, HtmlFormatter(nowrap=True))


def mark_callouts(body):
    def replace(match):
        kind = CALLOUTS[match.group(1)]
        return f'<blockquote class="callout callout-{kind}">\n<p><strong>{match.group(1)}'

    pattern = r"<blockquote>\n<p><strong>(" + "|".join(CALLOUTS) + ")"
    return re.sub(pattern, replace, body)


def page_tag(lecture):
    """Метка для колонтитула в стиле man: 4 -> LEC04(7), 4.5 -> LEC04.5(7)."""
    if not lecture:
        return "NOTES(7)"
    number, dot, part = lecture.partition(".")
    return f"LEC{number.zfill(2)}{dot}{part}(7)"


def fill_template(meta, body):
    lecture = meta.get("lecture", "")
    replacements = {
        "{{title}}": html.escape(meta["title"]),
        "{{eyebrow}}": f"Лекция {html.escape(lecture)}" if lecture else "",
        "{{summary}}": html.escape(meta.get("summary", "")),
        "{{page_tag}}": page_tag(lecture),
        "{{tool_dir}}": TOOL_DIR.as_uri(),
        "{{body}}": body,
    }
    page = (TOOL_DIR / "template.html").read_text(encoding="utf-8")
    for key, value in replacements.items():
        page = page.replace(key, value)
    return page


def render_html(md_path):
    md = MarkdownIt("commonmark", {"highlight": highlight_code})
    md.enable("table").enable("strikethrough").use(front_matter_plugin)
    tokens = md.parse(md_path.read_text(encoding="utf-8"))
    meta = next((parse_front_matter(t.content) for t in tokens if t.type == "front_matter"), {})
    meta.setdefault("title", md_path.parent.name)
    lecture = meta.get("lecture", "")
    if lecture and not re.fullmatch(r"\d+(\.\d+)?", lecture):
        sys.exit(f"{md_path}: lecture должен быть числом, например 4 или 4.5, а не {lecture!r}")
    body = mark_callouts(md.renderer.render(tokens, md.options, {}))
    return fill_template(meta, body)


def build(md_path, chrome):
    with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as tmp:
        html_path = Path(tmp) / "notes.html"
        tmp_pdf = Path(tmp) / "notes.pdf"
        html_path.write_text(render_html(md_path), encoding="utf-8")
        args = [
            chrome,
            "--headless=new",
            "--no-pdf-header-footer",
            "--generate-pdf-document-outline",
            f"--user-data-dir={Path(tmp) / 'profile'}",
            f"--print-to-pdf={tmp_pdf}",
            html_path.as_uri(),
        ]
        if hasattr(os, "geteuid") and os.geteuid() == 0:
            args.insert(1, "--no-sandbox")  # под root Chrome без этого не стартует
        result = subprocess.run(args, capture_output=True, text=True, check=False)
        if result.returncode != 0:
            sys.exit(f"Chrome упал на {md_path}:\n{result.stderr[-2000:]}")
        if not tmp_pdf.is_file() or tmp_pdf.stat().st_size == 0:
            sys.exit(f"Chrome не создал PDF для {md_path}")
        shutil.move(tmp_pdf, md_path.with_suffix(".pdf"))
    print(f"{md_path.parent.name}/notes.pdf")


def main():
    parser = argparse.ArgumentParser(description="Собрать конспекты в PDF")
    parser.add_argument("files", nargs="*", type=Path, help="пути к notes.md")
    parser.add_argument("--all", action="store_true", help="собрать все */notes.md")
    args = parser.parse_args()

    if args.all:
        files = sorted(REPO_ROOT.glob("*/notes.md"))
        if not files:
            parser.error("в репозитории нет ни одного */notes.md")
    else:
        files = args.files
        if not files:
            parser.error("укажите notes.md или --all")
        for path in files:
            if not path.is_file():
                parser.error(f"нет файла {path}")

    chrome = find_chrome()
    for md_path in files:
        build(md_path.resolve(), chrome)


if __name__ == "__main__":
    main()
