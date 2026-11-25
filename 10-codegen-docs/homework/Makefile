# встроенные правила make (calc.y -> y.tab.c и т.п.) не нужны, всё описано ниже
.SUFFIXES:

CC      = gcc
CFLAGS  = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -O2 -g
LDLIBS  = -lm
BISON   = bison -Wall -Wcounterexamples -Werror

# make SANITIZE=1 — те же сборка и тесты под ASan и UBSan
ifdef SANITIZE
CFLAGS  += -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address,undefined
endif

OBJS = calc.o calc.tab.o lex.yy.o vars.o funcs.o

all: calc

calc: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

# bison создаёт calc.tab.c и calc.tab.h за один запуск
calc.tab.c calc.tab.h &: calc.y
	$(BISON) -d calc.y

# flex создаёт lex.yy.c; ему нужен calc.tab.h (там NUM и NAME)
lex.yy.c: calc.l calc.tab.h
	flex calc.l

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

calc.o calc.tab.o lex.yy.o: calc.h vars.h funcs.h
lex.yy.o: calc.tab.h
vars.o: vars.h
funcs.o: funcs.h

test_vars: test_vars.c vars.c vars.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_vars.c vars.c

test_funcs: test_funcs.c funcs.c funcs.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_funcs.c funcs.c $(LDLIBS)

test: calc test_vars test_funcs
	./test_vars
	./test_funcs
	bash tests.sh ./calc

# сайт: docs/html/index.html
docs:
	doxygen Doxyfile

# архив для сдачи: исходники + report.txt из REPORT.md
dist:
	./dist.sh

clean:
	rm -f calc test_vars test_funcs *.o calc.tab.c calc.tab.h lex.yy.c codegen-homework.tar.gz
	rm -rf docs

.PHONY: all test docs dist clean
