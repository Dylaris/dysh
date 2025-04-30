CC = gcc 
CFLAGS = -Wall -Wextra -std=gnu11 -I../

BIN_DIR = ../bin
SRC = $(wildcard *.c)
TARGET = $(patsubst %.c, $(BIN_DIR)/%, $(SRC))

.PHONY: all clean

all: $(TARGET)

$(BIN_DIR)/%: %.c
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BIN_DIR)
