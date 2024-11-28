CC = gcc
CFLAGS = -Wall

SRC = src/
INCLUDE = include/
BIN = bin/

.PHONY: downloader
downloader: $(SRC)/main.c $(SRC)/server.c $(SRC)/server.h $(SRC)/aux.h
	$(CC) $(CFLAGS) -o download $^

.PHONY: clean
clean:
	rm -rf download