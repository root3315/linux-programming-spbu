# Практика 1. Репозиторий проекта

Как на слайде: проект из Chapter 07 скопировал в `~/auto-lab` и завёл там репозиторий с нуля.
Работал под обычным пользователем в контейнере с Ubuntu 24.04, git 2.43.0.

## Что Git видит до первого коммита

```text
$ tar -xzf chapter07.tar.gz
$ mkdir -p ~/auto-lab && cd ~/auto-lab
$ cp -r ~/Chapter07/project/. .
$ git init -b main
Initialized empty Git repository in /home/stepan/auto-lab/.git/
$ git config user.name "Stepan"
$ git config user.email stepan@linux
$ git status --short
?? .github/
?? .gitignore
?? Makefile
?? ci.sh
?? include/
?? packaging/
?? profile/
?? src/
?? tests/
```

`??` на каталогах: Git пока не отслеживает ничего внутри, поэтому показывает каталог целиком.
Файлы по одному видно через `--untracked-files=all`.

## CRLF и права в архиве

Перед коммитом решил проверить, что `ci.sh` вообще запускается:

```text
$ file Makefile ci.sh .gitignore src/main.c
Makefile:   makefile script, Unicode text, UTF-8 text, with CRLF line terminators
ci.sh:      POSIX shell script, Unicode text, UTF-8 text executable, with CRLF line terminators
.gitignore: Unicode text, UTF-8 text, with CRLF line terminators
src/main.c: C source, Unicode text, UTF-8 text, with CRLF line terminators
$ ls -l ci.sh packaging/build-deb.sh
-rw-r--r-- 1 stepan stepan  737 Nov 23 20:28 ci.sh
-rw-r--r-- 1 stepan stepan 1102 Nov 23 20:28 packaging/build-deb.sh
$ sh ci.sh
ci.sh: 7: set: Illegal option -
$ echo $?
2
```

Две проблемы сразу. В архиве преподавателя все файлы с концами строк CRLF, и `sh` читает
`set -eu\r` как опцию `-` с мусором. И у скриптов нет бита выполнения, `./ci.sh` дал бы
`Permission denied`. Закоммить я это как есть, в истории навсегда остались бы CRLF. Починил
до первого коммита:

```text
$ find . -type f -not -path "./.git/*" -exec sed -i 's/\r$//' {} +
$ chmod +x ci.sh packaging/build-deb.sh
$ file Makefile ci.sh .gitignore src/main.c
Makefile:   makefile script, Unicode text, UTF-8 text
ci.sh:      POSIX shell script, Unicode text, UTF-8 text executable
.gitignore: Unicode text, UTF-8 text
src/main.c: C source, Unicode text, UTF-8 text
```

Make и `.gitignore` с CRLF, кстати, работали: `git check-ignore -v hello` нашёл правило
`.gitignore:2:hello`, Git сам отбрасывает `\r` в конце строки шаблона.

## Сборка не должна попасть в status

```text
$ make
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/main.c -o src/main.o
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/greet.c -o src/greet.o
gcc -o hello src/main.o src/greet.o
$ git status --short --untracked-files=all
?? .github/workflows/ci.yml
?? .gitignore
?? Makefile
?? ci.sh
?? include/greet.h
?? packaging/DEBIAN/control
?? packaging/build-deb.sh
?? profile/hot.c
?? src/greet.c
?? src/main.c
?? tests/test_greet.c
```

`hello` и `src/*.o` есть на диске, но в списке их нет.

## Два коммита

В первый всё, кроме модуля greet и теста. Тест пойдёт отдельной веткой.

```text
$ git add .gitignore Makefile src/main.c ci.sh packaging .github profile
$ git status --short
A  .github/workflows/ci.yml
A  .gitignore
A  Makefile
A  ci.sh
A  packaging/DEBIAN/control
A  packaging/build-deb.sh
A  profile/hot.c
A  src/main.c
?? include/
?? src/greet.c
?? tests/
$ git commit -m "скелет проекта"
[main (root-commit) 828632a] скелет проекта
 8 files changed, 193 insertions(+)
 create mode 100644 .github/workflows/ci.yml
 create mode 100644 .gitignore
 create mode 100644 Makefile
 create mode 100755 ci.sh
 create mode 100644 packaging/DEBIAN/control
 create mode 100755 packaging/build-deb.sh
 create mode 100644 profile/hot.c
 create mode 100644 src/main.c
$ git add include/greet.h src/greet.c
$ git commit -m "модуль greet"
[main ddbf796] модуль greet
 2 files changed, 37 insertions(+)
 create mode 100644 include/greet.h
 create mode 100644 src/greet.c
$ git status --short
?? tests/
```

`100755` у `ci.sh` и `build-deb.sh`: Git хранит бит выполнения, `chmod +x` до коммита был не зря.

## Ветка feature/test

```text
$ git switch -c feature/test
Switched to a new branch 'feature/test'
$ git add tests/test_greet.c
$ git commit -m "тест для greet"
[feature/test 269bee9] тест для greet
 1 file changed, 38 insertions(+)
 create mode 100644 tests/test_greet.c
$ make test
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -o tests/test_greet tests/test_greet.c src/greet.o
./tests/test_greet
ok:   обычное имя
ok:   NULL заменяется на world
ok:   пустая строка заменяется
ok:   счётчик вызовов

все проверки прошли
$ git switch main
Switched to branch 'main'
$ git merge --no-ff -m "слить feature/test" feature/test
Merge made by the 'ort' strategy.
 tests/test_greet.c | 38 ++++++++++++++++++++++++++++++++++++++
 1 file changed, 38 insertions(+)
 create mode 100644 tests/test_greet.c
$ git branch -d feature/test
Deleted branch feature/test (was 269bee9).
$ git log --oneline --graph
*   cd693df слить feature/test
|\  
| * 269bee9 тест для greet
|/  
* ddbf796 модуль greet
* 828632a скелет проекта
```

В `main` за это время ничего не менялось, и обычный `git merge` сделал бы fast-forward: `main`
просто переехал бы на `269bee9`, и в графе ветки бы не было. Критерий просит видеть слияние,
поэтому `--no-ff`: он всегда делает коммит слияния с двумя родителями.

## Конфликт

Одну и ту же строку `snprintf` поменял в двух ветках по-разному:

```text
$ git switch -c feature/greet
Switched to a new branch 'feature/greet'
$ sed -i 's/"Hello, %s!"/"Hi, %s!"/' src/greet.c
$ git diff
diff --git a/src/greet.c b/src/greet.c
index 93bd39d..190542c 100644
--- a/src/greet.c
+++ b/src/greet.c
@@ -13,7 +13,7 @@ const char *greet(const char *name)
         name = "world";
 
     calls++;
-    snprintf(buffer, sizeof buffer, "Hello, %s!", name);
+    snprintf(buffer, sizeof buffer, "Hi, %s!", name);
     return buffer;
 }
 
$ git commit -am "короткое приветствие Hi"
[feature/greet e3005e7] короткое приветствие Hi
 1 file changed, 1 insertion(+), 1 deletion(-)
$ git switch main
Switched to branch 'main'
$ sed -i 's/"Hello, %s!"/"Hello there, %s!"/' src/greet.c
$ git commit -am "приветствие Hello there"
[main cefaba2] приветствие Hello there
 1 file changed, 1 insertion(+), 1 deletion(-)
$ git merge feature/greet
Auto-merging src/greet.c
CONFLICT (content): Merge conflict in src/greet.c
Automatic merge failed; fix conflicts and then commit the result.
$ echo $?
1
$ git status --short
UU src/greet.c
$ sed -n '14,22p' src/greet.c

    calls++;
<<<<<<< HEAD
    snprintf(buffer, sizeof buffer, "Hello there, %s!", name);
=======
    snprintf(buffer, sizeof buffer, "Hi, %s!", name);
>>>>>>> feature/greet
    return buffer;
}
```

`UU` — изменено с обеих сторон, не слито. Проверил, что будет, если забыть про маркеры:

```text
$ git diff --check
src/greet.c:16: leftover conflict marker
src/greet.c:18: leftover conflict marker
src/greet.c:20: leftover conflict marker
$ echo $?
2
$ make
gcc -std=c11 -Wall -Wextra -O2 -g -Iinclude -c src/greet.c -o src/greet.o
src/greet.c: In function ‘greet’:
src/greet.c:16:1: error: version control conflict marker in file
   16 | <<<<<<< HEAD
      | ^~~~~~~
src/greet.c:18:1: error: version control conflict marker in file
...
```

gcc узнаёт маркеры и пишет об этом прямо. Решение выбрал третье: ни `Hi`, ни `Hello there`,
а исходное `Hello, %s!`. На него рассчитан тест, оба варианта из веток его ломают. Оставил
в файле одну строку без маркеров:

```text
$ git diff --check
$ echo $?
0
$ make test | tail -1
все проверки прошли
$ git add src/greet.c
$ git status --short
M  src/greet.c
$ git commit --no-edit
[main 56f372d] Merge branch 'feature/greet'
$ git log --oneline --graph
*   56f372d Merge branch 'feature/greet'
|\  
| * e3005e7 короткое приветствие Hi
* | cefaba2 приветствие Hello there
|/  
*   cd693df слить feature/test
|\  
| * 269bee9 тест для greet
|/  
* ddbf796 модуль greet
* 828632a скелет проекта
```

## Сверх задания: git bisect

Сделал ещё четыре коммита, в одном специально сломал формат (убрал пробел после запятой), и
отдал поиск `git bisect run`. Он запускает команду на каждом шаге и по коду возврата решает:
0 хороший, 1–127 (кроме 125) плохой.

```text
$ git log --oneline -5
db9d3a6 обновил README
418e23e комментарий в main
9b5d7b1 поправил формат строки
3011dd3 README
56f372d Merge branch 'feature/greet'
$ git bisect start HEAD 56f372d
$ git bisect run make test
Bisecting: 1 revision left to test after this (roughly 1 step)
[9b5d7b1444bdfc077b006b035c8719c1e4a37619] поправил формат строки
running 'make' 'test'
...
FAIL: обычное имя
FAIL: NULL заменяется на world
FAIL: пустая строка заменяется
ok:   счётчик вызовов

3 проверок не прошло
make: *** [Makefile:26: test] Error 1
Bisecting: 0 revisions left to test after this (roughly 0 steps)
[3011dd3e5f94f87593e6f3d6cec2227f5c335a08] README
running 'make' 'test'
...
все проверки прошли
9b5d7b1444bdfc077b006b035c8719c1e4a37619 is the first bad commit
commit 9b5d7b1444bdfc077b006b035c8719c1e4a37619
Author: Stepan <stepan@linux>
Date:   Mon Nov 23 20:32:03 2026 +0000

    поправил формат строки

 src/greet.c | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)
bisect found first bad commit
$ git bisect reset
$ git revert --no-edit 9b5d7b1
[main 82a6342] Revert "поправил формат строки"
 Date: Mon Nov 23 20:32:44 2026 +0000
 1 file changed, 1 insertion(+), 1 deletion(-)
```

Четыре коммита, две проверки. Но `make test` пропустил вторую поломку. В коммите
«комментарий в main» я вставил строку `/* ... */` внутрь уже открытого комментария в шапке
`main.c`, и `*/` закрыл его раньше времени. Цель `test` собирает только `greet.o` и тест,
`main.c` она не трогает, так что bisect считал этот коммит хорошим. Нашёл это только в
практике 2, когда `./ci.sh` с `make clean && make` упал на `main.c`. Для bisect лучше
`git bisect run ./ci.sh`, он собирает всё.

## Что понял

Коммит берёт индекс, а не каталог: `git add` с конкретными путями позволил разложить одну
пачку файлов на два осмысленных коммита. Конфликт ничего не ломает, Git просто
останавливается, а дальше `git diff --check` и сборка ловят забытые маркеры. И проверка для
bisect должна быть полной, иначе он уверенно найдёт не тот коммит или не найдёт ничего.
