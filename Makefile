CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lcurl
SRCS = src/main.c src/init_server.c src/send.c src/get.c
TARGET = server

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)