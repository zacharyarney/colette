# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -Wextra -I./src

# Directory structure
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Find all source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
EXECUTABLE = $(BIN_DIR)/colette

# Main target
all: directories $(EXECUTABLE)

# Create necessary directories
directories:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

# Link object files into executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)

rebuild: clean all

install: all
	mkdir -p ~/bin
	cp $(EXECUTABLE) ~/bin/

.PHONY: all clean rebuild directories install
