# Практика 1. Свои тесты

Архив Chapter 08 распаковал в домашний каталог, проект скопировал в `~/health-lab`, как на
слайдах. Все практики делал там же, в контейнере с Ubuntu 24.04 (gcc 13.3.0).
Мой файл: [test_greeting.c](test_greeting.c), это `tests/test_greeting.c` из
`Chapter08/project` с двумя новыми тестами.

## Подготовка

```text
$ tar -xzvf chapter08.tar.gz
Chapter08/
Chapter08/README.md
Chapter08/project/
...
$ cp -r Chapter08/project ~/health-lab
$ cd ~/health-lab && ls -A
.gitignore
CMakeLists.txt
Makefile
fuzz
include
lint
src
tests
```

Как и в Chapter 04, файлы в архиве с концами строк CRLF. gcc и make это переживают, но
`diff` с моими правками показал бы каждую строку. Перевёл в LF сразу:

```text
$ file src/greeting.c Makefile
src/greeting.c: C source, Unicode text, UTF-8 text, with CRLF line terminators
Makefile:       makefile script, Unicode text, UTF-8 text, with CRLF line terminators
$ sed -i "s/\r$//" Makefile CMakeLists.txt .gitignore */*.c */*.h
$ file src/greeting.c Makefile
src/greeting.c: C source, Unicode text, UTF-8 text
Makefile:       makefile script, Unicode text, UTF-8 text
```

## 1. Исходные тесты

```text
$ make test; echo "код: $?"
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -o tests/test_greeting tests/test_greeting.c src/greeting.c
./tests/test_greeting
ok:   код возврата 0
ok:   обычное имя
ok:   NULL допустим
ok:   NULL заменяется на world
ok:   пустое имя допустимо
ok:   пустое имя заменяется
ok:   out == NULL отклоняется
ok:   cap == 0 отклоняется

все проверки прошли (но это не значит, что код без ошибок)
код: 0
```

Восемь зелёных, но длинного имени среди них нет.

## 2. Кириллица

Дописал после проверки `cap == 0`:

```c
memset(buffer, 0, sizeof buffer);
check(make_greeting(buffer, sizeof buffer, "СПбГУ") == 0
      && strcmp(buffer, "Hello, СПбГУ!") == 0, "кириллица");
```

```text
$ make test; echo "код: $?"
...
ok:   cap == 0 отклоняется
ok:   кириллица

все проверки прошли (но это не значит, что код без ошибок)
код: 0
```

Проходит: `strcat` копирует байты и про UTF-8 не знает, а «СПбГУ» — это 10 байт, места
хватает.

## 3. Длинное имя

Имя из 64 букв `A`. Вместе с `"Hello, "` и `"!"` это 72 символа, в буфер на 64 байта не
влезет, жду `-2`:

```c
/* 64 буквы: вместе с "Hello, " и "!" в буфер на 64 байта не влезет. */
char long_name[65];
memset(long_name, 'A', 64);
long_name[64] = '\0';
check(make_greeting(buffer, sizeof buffer, long_name) == -2,
      "длинное имя возвращает -2");
```

```text
$ make test; echo "код: $?"
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -o tests/test_greeting tests/test_greeting.c src/greeting.c
./tests/test_greeting
ok:   код возврата 0
ok:   обычное имя
ok:   NULL допустим
ok:   NULL заменяется на world
ok:   пустое имя допустимо
ok:   пустое имя заменяется
ok:   out == NULL отклоняется
ok:   cap == 0 отклоняется
ok:   кириллица
FAIL: длинное имя возвращает -2

1 проверок не прошло
make: *** [Makefile:28: test] Error 1
код: 2
```

Ответ на вопрос со слайда: тест не проходит, потому что в `make_greeting` вообще нет
ветки с `return -2`, хотя в `greeting.h` этот код описан. Функция вернула 0 и записала
строку за край буфера. Программа при этом не упала: лишние байты легли в соседний
`long_name`. Тест красный, но про порчу памяти он ничего не сказал.

## 4. Ломаю функцию

Убрал `"!"` из последнего `strcat` (копию исходника сохранил в `/tmp`):

```text
$ cp src/greeting.c /tmp/greeting.c.ok
$ sed -i "s/strcat(out, \"!\");/strcat(out, \"\");/" src/greeting.c
$ grep -n "strcat(out, \"\")" src/greeting.c
30:    strcat(out, "");
$ make test; echo "код: $?"
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -o tests/test_greeting tests/test_greeting.c src/greeting.c
./tests/test_greeting
ok:   код возврата 0
FAIL: обычное имя
ok:   NULL допустим
FAIL: NULL заменяется на world
ok:   пустое имя допустимо
FAIL: пустое имя заменяется
ok:   out == NULL отклоняется
ok:   cap == 0 отклоняется
FAIL: кириллица
FAIL: длинное имя возвращает -2

5 проверок не прошло
make: *** [Makefile:28: test] Error 1
код: 2
```

Покраснели все тесты, которые сравнивают строку. Те, что смотрят только код возврата,
остались зелёными: «код возврата 0» про восклицательный знак ничего не знает.

## 5. Чиню обратно

```text
$ cp /tmp/greeting.c.ok src/greeting.c
$ make test; echo "код: $?"
...
ok:   кириллица
FAIL: длинное имя возвращает -2

1 проверок не прошло
make: *** [Makefile:28: test] Error 1
код: 2
$ ./tests/test_greeting > /dev/null; echo "код теста: $?"
код теста: 1
```

Остался один красный, тот самый длинный. Сам тест возвращает 1, а `make` при упавшей
команде выходит с кодом 2. Исправлять функцию буду в практике 4.

## Что понял

Тест полезен, только если краснеет при поломке. Восемь исходных тестов не видят ошибку,
потому что я (и автор) не подумали про длинное имя. А тест, который проверяет только код
возврата, пропускает испорченную строку.
