# minishell: одна единица трансляции, C11, предупреждения как ошибки.
#
#   make         обычная сборка
#   make asan    сборка с AddressSanitizer и UBSan: minishell-asan
#   make test    прогон test.sh на обеих сборках
#   make dist    report.txt из REPORT.md и архив proc-homework.tar.gz
#   make clean   удалить всё собранное

CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g
SANFLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all

NAME    = proc-homework
SOURCES = minishell.c Makefile test.sh md2txt.sh

.PHONY: all asan test dist clean

all: minishell

minishell: minishell.c
	$(CC) $(CFLAGS) -o $@ $<

minishell-asan: minishell.c
	$(CC) $(CFLAGS) -O1 $(SANFLAGS) -o $@ $<

asan: minishell-asan

test: minishell minishell-asan
	./test.sh ./minishell
	./test.sh ./minishell-asan

# Архив собирается во временном каталоге, чтобы внутри была папка proc-homework/.
# report.txt появляется только там: в рабочем каталоге его нет, и clean в
# распакованном архиве его не сотрёт.
dist: REPORT.md $(SOURCES)
	rm -rf dist-tmp && mkdir -p dist-tmp/$(NAME)
	cp $(SOURCES) dist-tmp/$(NAME)/
	./md2txt.sh <REPORT.md >dist-tmp/$(NAME)/report.txt
	tar --sort=name --owner=0 --group=0 -czf $(NAME).tar.gz -C dist-tmp $(NAME)
	rm -rf dist-tmp
	tar tzf $(NAME).tar.gz

clean:
	rm -f minishell minishell-asan
	rm -rf dist-tmp
