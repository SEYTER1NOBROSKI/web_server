CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g3 -O0
LDFLAGS = -lcurl -pthread -lcjson
SRCS = src/main.c src/init_server.c src/send.c src/http_methods.c src/config_loader.c
TARGET = server

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)