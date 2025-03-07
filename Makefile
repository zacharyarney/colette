# Allow overriding of compiler and installation paths
CC ?= cc
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

# Detect operating system
UNAME_S := $(shell uname -s)

# Allow for overriding default C standard
CSTD ?= c99

# Enforce c99 as minimum standard
ifeq ($(CSTD),c89)
override CSTD = c99
$(warning C89/C90 is not supported, using C99 instead)
endif

ifeq ($(CSTD),c90)
override CSTD = c99
$(warning C89/C90 is not supported, using C99 instead)
endif

# Compiler flags - allow overriding but provide defaults
WARNING_FLAGS ?= -Wall -Werror -Wextra
STD_FLAGS ?= -std=$(CSTD) -pedantic -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
INCLUDE_FLAGS ?= -I./src
CFLAGS ?= $(WARNING_FLAGS) $(STD_FLAGS) $(INCLUDE_FLAGS)

# Linker flags
LDFLAGS ?=

# Sanitizer options
ASAN_OPTIONS ?= detect_leaks=1:print_stats=1:halt_on_error=0:exitcode=0
LSAN_OPTIONS ?= detect_leaks=1:print_suppressions=0:max_leaks=0

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
debug: CFLAGS += -DDBUG -g -O0
debug: directories $(EXECUTABLE)

# Memcheck flags - should work with both gcc and clang
SANITIZE_FLAGS = -fsanitize=address
# Allows program to continue after finding issues
SANITIZE_FLAGS += -fsanitize-recover=address
# Still halt on undefined behavior
SANITIZE_FLAGS += -fno-sanitize-recover=undefined
# Better stack traces
SANITIZE_FLAGS += -fno-omit-frame-pointer
SANITIZE_FLAGS += -fno-optimize-sibling-calls

# Memcheck target - should use llvm clang for full LSAN support
memcheck: CFLAGS += $(SANITIZE_FLAGS) -DDBUG -g -O0 -fno-inline
memcheck: LDFLAGS += $(SANITIZE_FLAGS)
memcheck: directories $(EXECUTABLE)
	ASAN_OPTIONS="$(ASAN_OPTIONS)" LSAN_OPTIONS="$(LSAN_OPTIONS)" $(SCRIPT_DIR)/run_memcheck.sh

# Test target
test: debug
	$(SCRIPT_DIR)/run_tests.sh

# Release target
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

# Install target that respects DESTDIR
install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(EXECUTABLE) $(DESTDIR)$(BINDIR)/

# Rebuild targets
rebuild: clean all
rebuild-debug: clean debug
rebuild-memcheck: clean memcheck
rebuild-test: clean test
rebuild-release: clean release
