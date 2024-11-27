CC = gcc
CFLAGS = -Wall

SRC = src/
INCLUDE = include/
BIN = bin/

.PHONY: downloader
downloader: $(SRC)/main.c
	$(CC) $(CFLAGS) -o protocol $^

.PHONY: clean
clean:
	rm -rf download