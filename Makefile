CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
SRCS = src/main.c src/init_server.c src/send.c
TARGET = server

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)