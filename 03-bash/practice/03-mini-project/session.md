# Практика 3. Мини-проект и права

Работал только внутри `~/bash-lab/course`. В начале выключил цвет у `ls`
(`alias ls='ls --color=never'`), чтобы в записи не было управляющих кодов.

## Старт

```text
stepan@lab:~$ mkdir -p ~/bash-lab && cd ~/bash-lab
stepan@lab:~/bash-lab$ mkdir -p course/{src,docs,backup}
stepan@lab:~/bash-lab$ cd course
stepan@lab:~/bash-lab/course$ pwd
/home/stepan/bash-lab/course
stepan@lab:~/bash-lab/course$ touch src/main.c src/util.c docs/readme.txt docs/notes.log
stepan@lab:~/bash-lab/course$ ls -la
total 20
drwxrwxr-x 5 stepan stepan 4096 Nov 23 19:57 .
drwxrwxr-x 4 stepan stepan 4096 Nov 23 19:57 ..
drwxrwxr-x 2 stepan stepan 4096 Nov 23 19:57 backup
drwxrwxr-x 2 stepan stepan 4096 Nov 23 19:59 docs
drwxrwxr-x 2 stepan stepan 4096 Nov 23 19:59 src
```

На слайде приглашение `~/course`, но по тексту задания каталог `~/bash-lab/course`, его и
взял. Каталоги получились `rwxrwxr-x`: в Ubuntu у пользователя своя группа и umask 002.

## 1. Копия src и переименование

```text
stepan@lab:~/bash-lab/course$ cp -r src backup/src-v1
stepan@lab:~/bash-lab/course$ mv src/util.c src/utils.c
stepan@lab:~/bash-lab/course$ ls src backup/src-v1
backup/src-v1:
main.c	util.c

src:
main.c	utils.c
```

Порядок важен: сначала копия, потом переименование. В копии осталось старое имя `util.c`.

## 2. find

```text
stepan@lab:~/bash-lab/course$ find . -type f
./backup/src-v1/util.c
./backup/src-v1/main.c
./src/utils.c
./src/main.c
./docs/notes.log
./docs/readme.txt
stepan@lab:~/bash-lab/course$ find . -type f -name '*.c'
./backup/src-v1/util.c
./backup/src-v1/main.c
./src/utils.c
./src/main.c
```

`'*.c'` в кавычках: шаблон должен дойти до `find`, а не раскрыться Bash'ем в текущем
каталоге. `find` выводит файлы в порядке каталога на диске, без сортировки.

## 3. setup.sh и право x

```text
stepan@lab:~/bash-lab/course$ cat > docs/setup.sh <<'END'
> #!/usr/bin/env bash
> printf 'setup ok\n'
> END
stepan@lab:~/bash-lab/course$ ls -l docs/setup.sh
-rw-rw-r-- 1 stepan stepan 40 Nov 23 20:03 docs/setup.sh
stepan@lab:~/bash-lab/course$ test -x docs/setup.sh && echo "исполняемый" || echo "не исполняемый"
не исполняемый
stepan@lab:~/bash-lab/course$ chmod u+x docs/setup.sh
stepan@lab:~/bash-lab/course$ ls -l docs/setup.sh
-rwxrw-r-- 1 stepan stepan 40 Nov 23 20:03 docs/setup.sh
stepan@lab:~/bash-lab/course$ test -x docs/setup.sh && echo "исполняемый"
исполняемый
stepan@lab:~/bash-lab/course$ ./docs/setup.sh
setup ok
```

`u+x` добавил `x` только владельцу, режим стал 764.

## 4. Копия readme.txt и rm -i

```text
stepan@lab:~/bash-lab/course$ cp docs/readme.txt backup/
stepan@lab:~/bash-lab/course$ ls backup
readme.txt  src-v1
stepan@lab:~/bash-lab/course$ rm -i backup/readme.txt
rm: remove regular empty file 'backup/readme.txt'? y
stepan@lab:~/bash-lab/course$ ls backup
src-v1
```

## 5. Запрет записи в main.c

```text
stepan@lab:~/bash-lab/course$ chmod u-w src/main.c
stepan@lab:~/bash-lab/course$ ls -l src/main.c
-r--rw-r-- 1 stepan stepan 0 Nov 23 19:59 src/main.c
stepan@lab:~/bash-lab/course$ printf 'int main(void) { return 0; }\n' > src/main.c
bash: src/main.c: Permission denied
stepan@lab:~/bash-lab/course$ test -w src/main.c || echo "записи нет"
записи нет
stepan@lab:~/bash-lab/course$ chmod u+w src/main.c
stepan@lab:~/bash-lab/course$ ls -l src/main.c
-rw-rw-r-- 1 stepan stepan 0 Nov 23 19:59 src/main.c
stepan@lab:~/bash-lab/course$ printf 'int main(void) { return 0; }\n' > src/main.c && cat src/main.c
int main(void) { return 0; }
```

Режим `r--rw-r--` выглядит странно: группа `stepan` писать может, а я, владелец и член этой
группы, нет. Ядро берёт биты только одного класса: раз я владелец, смотрит на `r--` и на
группу уже не глядит.

## Итоговое дерево

```text
stepan@lab:~/bash-lab/course$ stat -c '%A %a %n' src/main.c docs/setup.sh
-rw-rw-r-- 664 src/main.c
-rwxrw-r-- 764 docs/setup.sh
stepan@lab:~/bash-lab/course$ find . -type f | sort
./backup/src-v1/main.c
./backup/src-v1/util.c
./docs/notes.log
./docs/readme.txt
./docs/setup.sh
./src/main.c
./src/utils.c
```

Совпадает с ожидаемым деревом со слайда.

## Что понял

`test -x` и `test -w` удобнее, чем глазами читать `ls -l`: их можно поставить в условие.
Права проверяются по одному классу, поэтому `u-w` работает даже при открытой на запись
группе.
