# Makefile для hello: сборка, тест, установка, пакет и профиль.
# Эти же цели вызывают ci.sh и GitHub Actions.
#
#   make             сборка (-O2 -g)
#   make test        модульный тест
#   make deb         пакет dist/hello_<VERSION>_<arch>.deb
#   make profile     сборка с -pg, прогон --bench и плоский профиль gprof

CC      = gcc
CSTD    = -std=c11
WARN    = -Wall -Wextra -Wpedantic -Werror
CFLAGS  = $(CSTD) $(WARN) -O2 -g
PREFIX  = /usr/local

# Версия живёт только здесь: отсюда её берут --version, пакет и ci.sh.
VERSION  = 1.1.0
CPPFLAGS = -DVERSION=\"$(VERSION)\"

TARGET  = hello
SRCS    = main.c greet.c stats.c
OBJS    = $(SRCS:.c=.o)
TEST    = tests/test_hello

# -fno-inline: на -O2 gcc встраивает count_distinct в stats_collect,
# и в профиле её не видно отдельной строкой.
PROF_DIR    = build-prof
PROF_CFLAGS = $(CSTD) $(WARN) -O2 -g -pg -fno-inline
BENCH       = 50000

.PHONY: all test install deb profile clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# VERSION зашита в main.o: сменили её в Makefile — main.o надо пересобрать.
main.o: greet.h stats.h Makefile
greet.o: greet.h
stats.o: stats.h

$(TEST): tests/test_hello.c greet.o stats.o greet.h stats.h
	$(CC) $(CFLAGS) -I. -o $@ tests/test_hello.c greet.o stats.o

test: $(TARGET) $(TEST)
	./$(TEST)
	@v=$$(./$(TARGET) --version); \
	if [ "$$v" = "hello $(VERSION)" ]; then echo "ok:   --version: $$v"; \
	else echo "FAIL: --version: '$$v', ждал 'hello $(VERSION)'"; exit 1; fi

# DESTDIR — корень «будущей системы»: так пакет собирается без root.
install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

deb: $(TARGET)
	rm -rf build/deb
	$(MAKE) --no-print-directory install DESTDIR=build/deb PREFIX=/usr
	sh packaging/build-deb.sh build/deb $(VERSION)

# gmon.out появляется в текущем каталоге процесса, поэтому запуск из $(PROF_DIR).
profile:
	rm -rf $(PROF_DIR)
	mkdir -p $(PROF_DIR)
	$(CC) $(CPPFLAGS) $(PROF_CFLAGS) -o $(PROF_DIR)/$(TARGET) $(SRCS)
	cd $(PROF_DIR) && ./$(TARGET) --bench $(BENCH)
	cd $(PROF_DIR) && gprof -b -p $(TARGET) gmon.out > flat.txt
	head -n 10 $(PROF_DIR)/flat.txt

clean:
	rm -f $(TARGET) $(OBJS) $(TEST)
	rm -rf build dist $(PROF_DIR)
