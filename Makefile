# Compiler
CC = gcc

# Compiler flags
WARNING_FLAGS = -Wall -Werror -Wextra
STD_FLAGS = -std=c11 -pedantic
INCLUDE_FLAGS = -I./src
CFLAGS = $(WARNING_FLAGS) $(STD_FLAGS) $(INCLUDE_FLAGS)

# Linker flags
LDFLAGS =

# Directory structure
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Find all source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
EXECUTABLE = $(BIN_DIR)/colette

# Build targets
.PHONY: all debug release clean rebuild directories install

# Main target
all: release

# Debug target
debug: DEBUG = 1
debug: CFLAGS += -fsanitize=address -DDBUG -g -O0
debug: LDFLAGS += -fsanitize=address
debug: directories $(EXECUTABLE)

# Debug target
release: DEBUG = 0
release: CFLAGS += -O2
release: directories $(EXECUTABLE)

# Create necessary directories
directories:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

# Link object files into executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)

rebuild: clean all
rebuild-debug: clean debug
rebuild-release: clean release

install: all
	mkdir -p ~/bin
	cp $(EXECUTABLE) ~/bin/
