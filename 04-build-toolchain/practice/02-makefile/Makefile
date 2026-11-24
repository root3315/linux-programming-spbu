# Makefile из Chapter 04/hello, дополненный целями run и release.
# В рецептах обязателен символ табуляции, а не пробелы.

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -g
TARGET  = hello
OBJS    = main.o greet.o

RELEASE        = hello-release
RELEASE_CFLAGS = -std=c11 -Wall -Wextra -O3 -DNDEBUG

.PHONY: all clean run release

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Заголовок тоже зависимость: правим greet.h, пересобираются оба объекта.
main.o: greet.h
greet.o: greet.h

run: $(TARGET)
	./$(TARGET) "Linux"

# Релиз собираю отдельным файлом прямо из исходников, без .o:
# иначе объекты с -O2 -g и с -O3 перепутаются.
release: $(RELEASE)

$(RELEASE): main.c greet.c greet.h
	$(CC) $(RELEASE_CFLAGS) -o $@ main.c greet.c

clean:
	rm -f $(OBJS) $(TARGET) $(RELEASE)
