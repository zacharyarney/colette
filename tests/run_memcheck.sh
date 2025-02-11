#!/bin/bash

# Get absolute paths (using same pattern as your run_tests.sh)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_DATA="$SCRIPT_DIR/testdata"
COLETTE="$PROJECT_ROOT/bin/colette"

# ASAN/LSAN configuration for memory checking
export ASAN_OPTIONS="detect_leaks=1:print_stats=1:halt_on_error=0:exitcode=0"
export LSAN_OPTIONS="detect_leaks=1:print_suppressions=0:max_leaks=0"

# Set up test directory structure
echo "Setting up test directory..."
mkdir -p "$TEST_DATA/memcheck/valid"
mkdir -p "$TEST_DATA/memcheck/nested"

# Create test files
echo "chapter1.md" > "$TEST_DATA/memcheck/valid/.index"
echo "Chapter 1" > "$TEST_DATA/memcheck/valid/chapter1.md"

echo "chapter1" > "$TEST_DATA/memcheck/nested/.index"
mkdir -p "$TEST_DATA/memcheck/nested/chapter1"
echo "scene1.md" > "$TEST_DATA/memcheck/nested/chapter1/.index"
echo "Scene 1" > "$TEST_DATA/memcheck/nested/chapter1/scene1.md"

# Run memory checks for each mode
echo -e "\nTesting check mode..."
$COLETTE --check "$TEST_DATA/memcheck/valid"

echo -e "\nTesting collate mode..."
$COLETTE "$TEST_DATA/memcheck/valid"

# echo -e "\nTesting init mode..."
# $COLETTE --init "$TEST_DATA/memcheck/nested"

# Clean up
echo -e "\nCleaning up..."
rm -rf "$TEST_DATA/memcheck"

echo -e "\nMemory check complete."
