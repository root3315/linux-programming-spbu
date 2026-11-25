# Makefile для hello: три единицы трансляции и два заголовка.
# Используется и локально, и внутри образа.
#
#   make           обычная сборка
#   make static    статическая сборка (для образа на scratch)
#   make clean

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
LDFLAGS =
TARGET  = hello
SRCS    = main.c greet.c stats.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all static clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

main.o: greet.h stats.h
greet.o: greet.h
stats.o: stats.h

# -s выкидывает таблицу символов: в образе отладчика всё равно нет.
static: clean
	$(MAKE) LDFLAGS="-static -s" $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
