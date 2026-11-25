# Занятие 10. Генераторы кода и документация

Лекция-практикум о том, как не писать скучный код руками. flex по набору правил делает лексер
(режет строку на токены), bison по грамматике делает парсер, doxygen собирает html-сайт из
комментариев в исходниках. На практиках работал с калькулятором из архива преподавателя: добавлял
комментарии `#`, остаток от деления `%` и степень `^`, сравнивал его с ручным разбором и собирал
документацию. Домашнее задание: переменные, `sqrt` и документация ко всем функциям.

## Что внутри

- [notes.md](notes.md): конспект лекции;
- [notes.pdf](notes.pdf): он же в PDF;
- [practice/](practice/): четыре практики, условия в [practice/README.md](practice/README.md);
- [homework/](homework/README.md): ДЗ, отчёт и архив `codegen-homework.tar.gz`.

## Как запустить

Проверял на Ubuntu 24.04 (flex 2.6.4, bison 3.8.2, doxygen 1.9.8, gcc 13.3.0):

```bash
sudo apt update
sudo apt install flex bison doxygen build-essential
wget https://www.smartmelio.ru/chapter10.tar.gz
tar -xzvf chapter10.tar.gz
cd Chapter10
```

Архив с примерами преподавателя в репозитории нет. Мои правки лежат в папках практик: их
файлами заменяются одноимённые файлы из `Chapter10/calc` и `Chapter10/handwritten`, подробности
в [practice/README.md](practice/README.md). Если архив распакован в папке занятия, собрать и
попробовать калькулятор можно так, из `Chapter10`:

```bash
cp ../practice/03-compare/calc.l ../practice/03-compare/calc.y calc/
cp ../practice/04-doxygen/mini_calc.c handwritten/
cd calc && make && printf '%s\n' '2 + 3 * 4' '10 % 3' '-2 ^ 2' | ./calc
```
