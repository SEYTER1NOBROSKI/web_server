CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g3 -O0
LDFLAGS = -lcurl -pthread -lcjson
SRCS = src/main.c src/init_server.c src/send.c src/http_methods.c src/config_loader.c src/logger.c
TARGET = server

all: $(TARGET) test_app

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

test_app: test/test.c
	$(CC) $(CFLAGS) -o test_app test/test.c $(LDFLAGS)

clean:
	rm -f $(TARGET) test_app server