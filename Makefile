CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O2
LDFLAGS =

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

COMMON_OBJ = $(OBJ_DIR)/protocol.o $(OBJ_DIR)/nn.o $(OBJ_DIR)/crc32.o
TARGETS = $(BIN_DIR)/main $(BIN_DIR)/sender $(BIN_DIR)/receiver

.PHONY: all clean

all: $(TARGETS)

$(BIN_DIR)/main: $(OBJ_DIR)/main.o $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/sender: $(OBJ_DIR)/sender.o $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/receiver: $(OBJ_DIR)/receiver.o $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
