# Практика 1. Четыре этапа сборки вручную

Файл `stages/demo.c` из Chapter 04 не менял, своих файлов тут нет, только сессия. Работал в
контейнере с Ubuntu 24.04 (gcc 13.3.0) под обычным пользователем, в `~/build-lab`.
В `demo.c` два макроса (`VERSION "1.0"`, `SQUARE(x) ((x) * (x))`), глобальная переменная
`global_counter`, `static` указатель `app_name` и `main` с одним `printf`.

## Шаг 1. Препроцессор

Предсказание: в `demo.i` будет весь `stdio.h`, а в конце мой `main`, где вместо `VERSION`
стоит `"1.0"`, а вместо `SQUARE(7)` выражение со скобками.

```bash
cp -r /path/to/Chapter04/stages .
cd stages
gcc -E demo.c > demo.i
```

```text
$ wc -l demo.i
834 demo.i
$ tail -8 demo.i
static const char *app_name = "stages";

int main(void)
{
    global_counter++;
    printf("%s %s: %d\n", app_name, "1.0", ((7) * (7)));
    return 0;
}
```

Из 834 строк моих около двадцати, остальное принёс `#include <stdio.h>`. `SQUARE(7)` стал
`((7) * (7))`, а не `49`: препроцессор только подставляет текст, считать будет компилятор.

## Шаг 2. Компилятор

```text
$ gcc -S demo.c
$ cat demo.s
	.file	"demo.c"
	.text
	.globl	global_counter
	.bss
	.align 4
	.type	global_counter, @object
	.size	global_counter, 4
global_counter:
	.zero	4
	.section	.rodata
.LC0:
	.string	"stages"
...
	.section	.rodata
.LC1:
	.string	"1.0"
.LC2:
	.string	"%s %s: %d\n"
	.text
	.globl	main
	.type	main, @function
main:
...
	movl	global_counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, global_counter(%rip)
	movq	app_name(%rip), %rax
	movl	$49, %ecx
	leaq	.LC1(%rip), %rdx
...
	call	printf@PLT
...
```

Строки (`"stages"`, `"1.0"`, формат `printf`) лежат в `.rodata`. `global_counter` в `.bss`,
потому что инициализирован нулём. А `((7) * (7))` компилятор посчитал сам: в коде уже
готовое `$49`, хотя оптимизация была выключена.

## Шаг 3. Ассемблер

Предсказание: в `demo.o` будет определён `main` (T) и нужен снаружи `printf` (U).

```text
$ gcc -c demo.c
$ file demo.o
demo.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
$ nm demo.o
0000000000000000 d app_name
0000000000000000 B global_counter
0000000000000000 T main
                 U printf
$ nm -u demo.o
                 U printf
```

`B` у `global_counter` — глобальная переменная в `.bss`, а маленькая `d` у `app_name`
значит «локальная в секции данных» (она `static`). Адреса у всех нули: объектный файл ещё
никуда не размещён.

Проверил, что `.o` правда нельзя запустить:

```text
$ ./demo.o
bash: ./demo.o: Permission denied
$ cp demo.o try.o && chmod +x try.o && ./try.o
bash: ./try.o: cannot execute binary file: Exec format error
```

Даже с правом на выполнение ядро его не берёт: это `relocatable`, а не `executable`. В нём
нет точки входа `_start`, нет кода `printf`, адреса не расставлены.

## Шаг 4. Линковщик

```text
$ gcc demo.o -o demo
$ ./demo
stages 1.0: 49
$ nm demo | grep -E " (T|U) "
                 U __libc_start_main@GLIBC_2.34
0000000000001194 T _fini
0000000000001000 T _init
0000000000001060 T _start
0000000000001149 T main
                 U printf@GLIBC_2.2.5
$ ldd demo
	linux-vdso.so.1 (0x000079f09f0b4000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x000079f09ee91000)
	/lib64/ld-linux-x86-64.so.2 (0x000079f09f0b6000)
$ file demo
demo: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=29363bc588f581cab47df7f7f6bb8052c1abb33a, for GNU/Linux 3.2.0, not stripped
```

Что добавил линковщик: стартовый код (`_start`, `_init`, `_fini`), адреса для `main` и
переменных, и ссылку на `libc.so.6`. `printf` так и остался `U`, но теперь с версией
`@GLIBC_2.2.5`: его найдёт загрузчик `ld-linux-x86-64.so.2` при запуске.

## Шаг 5. Одной командой и сравнение размеров

```text
$ gcc demo.c -o demo2
$ ./demo2
stages 1.0: 49
$ ls -l demo.i demo.s demo.o demo demo2
-rwxrwxr-x 1 stepan stepan 16040 Nov 23 19:29 demo
-rw-rw-r-- 1 stepan stepan 21436 Nov 23 19:29 demo.i
-rw-rw-r-- 1 stepan stepan  1928 Nov 23 19:29 demo.o
-rw-rw-r-- 1 stepan stepan  1136 Nov 23 19:29 demo.s
-rwxrwxr-x 1 stepan stepan 16040 Nov 23 19:29 demo2
$ cmp demo demo2 && echo одинаковые
одинаковые
$ size demo.o demo
   text	   data	    bss	    dec	    hex	filename
    182	      8	      4	    194	     c2	demo.o
   1452	    608	      8	   2068	    814	demo
```

`demo` и `demo2` совпали байт в байт: `gcc demo.c -o demo2` делает те же четыре шага, только
промежуточные файлы прячет во временный каталог.

Ответ на вопрос со слайда, почему `.i` весит 21 КБ, а `.o` меньше 2 КБ: в `.i` текстом
лежат все объявления из `stdio.h` (сотни прототипов, типов, комментарии препроцессора с
номерами строк), а в `.o` попадает только машинный код моего `main` (182 байта `text`) и
таблица символов. Объявления, которые не использованы, в код не превращаются.

Для интереса собрал статически:

```text
$ gcc -static demo.c -o demo-static
$ ls -l demo-static; file demo-static; ldd demo-static
-rwxrwxr-x 1 stepan stepan 785352 Nov 23 19:29 demo-static
demo-static: ELF 64-bit LSB executable, x86-64, version 1 (GNU/Linux), statically linked, BuildID[sha1]=068c8313470535e8bd8330a5d27f20506f761aff, for GNU/Linux 3.2.0, not stripped
	not a dynamic executable
```

785 КБ вместо 16 КБ: кусок libc вклеен внутрь.

## Что понял

`.o` нельзя запустить, потому что в нём только мой код с дырками: чужие функции помечены
`U`, адреса не расставлены и нет стартового кода. Всё это добавляет линковщик на четвёртом
шаге, а для динамической сборки часть работы откладывается до запуска, её делает
`ld-linux`.
