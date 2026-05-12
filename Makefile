CC := gcc

CFLAGS := -Wall -Wextra -g -O2

LDLIBS := -lncurses

SRC_DIR := src
INC_DIR := include
BIN_DIR := bin

CLIENT_SRC := $(wildcard $(SRC_DIR)/client/*.c)
SERVER_SRC := $(wildcard $(SRC_DIR)/server/*.c)

CLIENT_BIN := $(BIN_DIR)/client
SERVER_BIN := $(BIN_DIR)/server

all: $(CLIENT_BIN) $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(SERVER_BIN): $(SERVER_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean