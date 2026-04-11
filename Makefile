SRC_DIR := src
BIN_DIR := bin
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
BIN_FILES := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%, $(SRC_FILES))

CC := gcc
CFLAGS := -g -Wall -O2

all: $(BIN_FILES)

$(BIN_DIR)/%: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -f $(BIN_DIR)/*
