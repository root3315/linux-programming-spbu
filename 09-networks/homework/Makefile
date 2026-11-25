CC     ?= gcc
CFLAGS  = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -O2

all: chat_server chat_client

chat_server: chat_server.c chat.h
	$(CC) $(CFLAGS) -o $@ chat_server.c

chat_client: chat_client.c chat.h
	$(CC) $(CFLAGS) -o $@ chat_client.c

# архив для сдачи: исходники + report.txt из REPORT.md
dist:
	./dist.sh

clean:
	rm -f chat_server chat_client net-homework.tar.gz

.PHONY: all dist clean
