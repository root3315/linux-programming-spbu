# Проект для лекции 08: тесты, покрытие, санитайзеры, фаззинг, линтеры.
# Каждая цель отвечает на один вопрос о коде.
# ДЗ: добавлена функция shorten_name (src/shorten.c) и её проверки.

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -g -Iinclude
SAN     = -fsanitize=address,undefined -fno-omit-frame-pointer

LIB_SRC = src/greeting.c src/shorten.c
HEADERS = include/greeting.h include/shorten.h

BIN   = app
OBJS  = src/main.o src/greeting.o src/shorten.o
TESTS = tests/test_greeting tests/test_shorten
FUZZ  = fuzz_greeting

FUZZ_RUNS ?= 2000
FUZZ_SEED ?= 1

.PHONY: all test coverage sanitize fuzz tidy lint-demo clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 1. Тесты: отвечают на вопрос «работает ли то, что мы придумали проверить».
tests/test_%: tests/test_%.c $(LIB_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(LIB_SRC)

test: $(TESTS)
	./tests/test_greeting
	./tests/test_shorten

# 2. Покрытие: какие строки тесты вообще не открывали.
# Объектные файлы называются как исходники, тогда gcov находит .gcno/.gcda сам.
coverage:
	rm -rf build/cov *.gcov
	mkdir -p build/cov
	$(CC) $(CFLAGS) --coverage -c src/greeting.c -o build/cov/greeting.o
	$(CC) $(CFLAGS) --coverage -c src/shorten.c -o build/cov/shorten.o
	$(CC) $(CFLAGS) --coverage tests/test_greeting.c build/cov/greeting.o build/cov/shorten.o \
		-o build/cov/test_greeting
	$(CC) $(CFLAGS) --coverage tests/test_shorten.c build/cov/greeting.o build/cov/shorten.o \
		-o build/cov/test_shorten
	./build/cov/test_greeting > /dev/null
	./build/cov/test_shorten > /dev/null
	gcov -o build/cov $(LIB_SRC) | grep -A1 "^File 'src/"
	@echo "--- непокрытые строки (#####) ---"
	@if grep -n '#####' greeting.c.gcov shorten.c.gcov; then exit 1; else echo "нет"; fi

# 3. Санитайзеры: ловят порчу памяти, даже если программа «не упала».
sanitize:
	mkdir -p build
	$(CC) $(CFLAGS) $(SAN) -o build/sanitize-greeting tests/test_greeting.c $(LIB_SRC)
	$(CC) $(CFLAGS) $(SAN) -o build/sanitize-shorten tests/test_shorten.c $(LIB_SRC)
	./build/sanitize-greeting > /dev/null
	./build/sanitize-shorten > /dev/null
	@echo "тесты под ASan и UBSan прошли"

# 4. Фаззинг: тысячи случайных входов для обеих функций.
$(FUZZ): fuzz/fuzz_greeting.c $(LIB_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SAN) -o $@ fuzz/fuzz_greeting.c $(LIB_SRC)

fuzz: $(FUZZ)
	./$(FUZZ) $(FUZZ_RUNS) $(FUZZ_SEED)

# 5. Линтеры: статический анализ, программу не запускаем.
# Ненулевой код при любом замечании, иначе make tidy зелёный всегда.
build/compile_commands.json: CMakeLists.txt
	cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null

tidy: build/compile_commands.json
	cppcheck --enable=warning,style --error-exitcode=1 --quiet -Iinclude src tests fuzz
	clang-tidy -p build --quiet --warnings-as-errors='*' \
		src/*.c tests/*.c fuzz/*.c
	@echo "cppcheck и clang-tidy замечаний не нашли"

# Учебный «грязный» файл с лекции: тут линтеры должны ругаться.
lint-demo: build/compile_commands.json
	-cppcheck --enable=warning,style --quiet lint/issues.c
	-clang-tidy -p build lint/issues.c

clean:
	rm -rf build $(BIN) $(TESTS) $(FUZZ) *.gcov src/*.o
