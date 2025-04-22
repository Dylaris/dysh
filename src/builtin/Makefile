CC = gcc 
CFLAGS = -Wall -Wextra

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
