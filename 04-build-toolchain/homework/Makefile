# Makefile для hello: три единицы трансляции и два заголовка.
# В рецептах обязателен символ табуляции, а не пробелы.
#
#   make           обычная сборка (-O2 -g)
#   make release   чистая сборка без отладочной информации
#   make check     сборка с ASan/UBSan в build-san/ и прогон программы
#   make run       запуск с аргументом

CC      = gcc
CSTD    = -std=c11
WARN    = -Wall -Wextra -Wpedantic -Werror
CFLAGS  = $(CSTD) $(WARN) -O2 -g
LDFLAGS =
TARGET  = hello
SRCS    = main.c greet.c stats.c
OBJS    = $(SRCS:.c=.o)

RELEASE_CFLAGS = $(CSTD) $(WARN) -O2 -DNDEBUG

# -fno-sanitize-recover=all: UBSan по умолчанию только пишет предупреждение
# и продолжает работу, а нам нужен ненулевой код выхода.
SAN_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
SAN_DIR   = build-san
SAN_OBJS  = $(addprefix $(SAN_DIR)/,$(OBJS))

.PHONY: all clean run release check

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Заголовки тоже зависимости: правим greet.h — пересобираются те, кто его подключает.
main.o: greet.h stats.h
greet.o: greet.h
stats.o: stats.h

run: $(TARGET)
	./$(TARGET) "Linux"

# Сначала clean: иначе make решит, что объекты уже свежие, и флаги не сменятся.
release: clean
	$(MAKE) CFLAGS="$(RELEASE_CFLAGS)" $(TARGET)

# Санитайзерная сборка лежит отдельно, чтобы не смешивать её объекты с обычными.
$(SAN_DIR):
	mkdir -p $@

$(SAN_DIR)/%.o: %.c | $(SAN_DIR)
	$(CC) $(CSTD) $(WARN) -O1 -g $(SAN_FLAGS) -c $< -o $@

$(SAN_DIR)/main.o: greet.h stats.h
$(SAN_DIR)/greet.o: greet.h
$(SAN_DIR)/stats.o: stats.h

$(SAN_DIR)/$(TARGET): $(SAN_OBJS)
	$(CC) $(SAN_FLAGS) -o $@ $^

# Любая находка санитайзера даёт ненулевой код, make на нём останавливается
# и сам возвращает ошибку — это и нужно для CI.
check: $(SAN_DIR)/$(TARGET)
	./$(SAN_DIR)/$(TARGET)
	./$(SAN_DIR)/$(TARGET) "Linux"
	./$(SAN_DIR)/$(TARGET) ""
	./$(SAN_DIR)/$(TARGET) "Привет 2024"
	@echo "check: ошибок нет"

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf $(SAN_DIR)
