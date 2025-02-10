# Detect operating system
UNAME_S := $(shell uname -s)

# Check for Homebrew LLVM on macOS, fallback to system clang
ifeq ($(UNAME_S),Darwin)
    HOMEBREW_LLVM := $(shell test -x /opt/homebrew/opt/llvm/bin/clang && echo "yes")
    ifeq ($(HOMEBREW_LLVM),yes)
        CC = /opt/homebrew/opt/llvm/bin/clang
    else
        CC = clang
        $(warning On macOS, Homebrew LLVM is recommended for leak detection. Install with: brew install llvm)
    endif
else
    CC = clang
endif

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
SCRIPT_DIR = tests

# Find all source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
EXECUTABLE = $(BIN_DIR)/colette

# Build targets
.PHONY: all memcheck debug test release clean rebuild directories install

# Main target
all: release

# Debug target
debug: DEBUG = 1
debug: CFLAGS += -DDBUG -g -O0
debug: directories $(EXECUTABLE)

# Memcheck flags based on compiler
ifeq ($(HOMEBREW_LLVM),yes)
    MEMCHECK_SANITIZE = -fsanitize=address,leak
else
    MEMCHECK_SANITIZE = -fsanitize=address
endif

# Memcheck target
# Allows program to continue after finding issues
MEMCHECK_SANITIZE += -fsanitize-recover=address,leak
# Still halt on undefined behavior
MEMCHECK_SANITIZE += -fno-sanitize-recover=undefined
# Better stack traces
MEMCHECK_SANITIZE += -fno-omit-frame-pointer
MEMCHECK_SANITIZE += -fno-optimize-sibling-calls
memcheck: DEBUG = 1
memcheck: CFLAGS += $(MEMCHECK_SANITIZE) -DDBUG -g -O0 -fno-inline
memcheck: LDFLAGS += $(MEMCHECK_SANITIZE)
memcheck: directories $(EXECUTABLE)
	$(SCRIPT_DIR)/run_memcheck.sh

# Test target
test: debug
	$(SCRIPT_DIR)/run_tests.sh

# Release target
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
rebuild-memcheck: clean memcheck
rebuild-test: clean test
rebuild-release: clean release

install: all
	echo $(shell gcc --version | grep ^gcc | sed 's/^.* //g')
	mkdir -p ~/bin
	cp $(EXECUTABLE) ~/bin/
