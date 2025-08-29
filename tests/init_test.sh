#!/bin/bash

# Initialize test counters (required by run_tests.sh)
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Test helper function for init mode
test_init() {
    local project_dir="$1"
    local expected_status="$2"
    local test_name="$3"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "\n${YELLOW}Test $TESTS_RUN: $test_name${NC}"
    
    # Run colette in init mode
    output=$($COLETTE --init "$project_dir" 2>&1)
    status=$?
    
    # Check exit status
    if [ $status -eq $expected_status ]; then
        echo -e "${GREEN}✓ Exit status correct ($status)${NC}"
        status_passed=true
    else
        echo -e "${RED}✗ Expected status $expected_status, got $status${NC}"
        status_passed=false
    fi
    
    # For now, we consider a passed test if the status is correct
    if [ "$status_passed" = true ]; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Test helper for validating index content
check_index_content() {
    local index_file="$1"
    local expected_content="$2" # Using literal newlines in string
    local test_name="$3"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "\n${YELLOW}Test $TESTS_RUN: $test_name${NC}"
    
    if [ ! -f "$index_file" ]; then
        echo -e "${RED}✗ Index file does not exist: $index_file${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return
    fi
    
    # Read the index file content, remove empty lines and comments, and normalize
    content=$(grep -v "^$\|^#" "$index_file" | sort | tr -d '\r' | sed 's/[[:space:]]*$//')
    # Normalize expected content the same way
    sorted_expected=$(echo "$expected_content" | sort | tr -d '\r' | sed 's/[[:space:]]*$//')
    
    # For debugging
    if [ "$content" != "$sorted_expected" ]; then
        echo "Content (hex):" 
        hexdump -C <<< "$content"
        echo "Expected (hex):"
        hexdump -C <<< "$sorted_expected"
    fi
    
    if [ "$content" = "$sorted_expected" ]; then
        echo -e "${GREEN}✓ Index content matches expected${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ Index content mismatch${NC}"
        echo -e "${RED}Expected:${NC}\n$sorted_expected"
        echo -e "${RED}Got:${NC}\n$content"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Setup functions for test cases
setup_empty_project() {
    local dir="$TEST_DATA/empty_project"
    mkdir -p "$dir"
    # intentionally empty
}

setup_simple_project() {
    local dir="$TEST_DATA/simple_project"
    mkdir -p "$dir"
    
    # Create some content files
    echo "Chapter 1 content" > "$dir/chapter1.md"
    echo "Chapter 2 content" > "$dir/chapter2.md"
    echo "Notes" > "$dir/_notes.md"  # Should be excluded
}

setup_nested_project() {
    local dir="$TEST_DATA/nested_project"
    mkdir -p "$dir/part1/chapter1"
    mkdir -p "$dir/part1/chapter2"
    mkdir -p "$dir/_drafts"  # Should be excluded
    
    # Create content files at different levels
    echo "Introduction" > "$dir/intro.md"
    echo "Chapter 1 Scene 1" > "$dir/part1/chapter1/scene1.md"
    echo "Chapter 1 Scene 2" > "$dir/part1/chapter1/scene2.md"
    echo "Chapter 2 Scene 1" > "$dir/part1/chapter2/scene1.md"
    echo "Draft" > "$dir/_drafts/draft.md"  # Should be excluded
}

setup_special_names_project() {
    local dir="$TEST_DATA/special_names"
    mkdir -p "$dir"
    
    # Create files with special characters in names
    touch "$dir/file with spaces.md"
    touch "$dir/file-with-dashes.md"
    touch "$dir/file_with_underscores.md"
    touch "$dir/file.with.dots.md"
    touch "$dir/file@with@special@chars.md"
}

setup_permissions_test() {
    local dir="$TEST_DATA/permissions"
    mkdir -p "$dir/readonly_dir"
    mkdir -p "$dir/nowrite_dir"
    
    # Create content files
    echo "Content" > "$dir/readonly_dir/file.md"
    echo "Content" > "$dir/nowrite_dir/file.md"
    
    # Create empty index file first (for read-only test)
    touch "$dir/nowrite_dir/.index"
    
    # Set permissions
    chmod 555 "$dir/readonly_dir"  # read-only directory
    chmod 444 "$dir/nowrite_dir/.index"  # read-only index file (no write permission)
}

# Run tests for empty project
setup_empty_project
test_init "$TEST_DATA/empty_project" 0 "Initialize empty project"
check_index_content "$TEST_DATA/empty_project/.index" "" "Empty project index content"

# Run tests for simple project
setup_simple_project
test_init "$TEST_DATA/simple_project" 0 "Initialize simple project"
check_index_content "$TEST_DATA/simple_project/.index" "chapter1.md
chapter2.md" "Simple project index content"

# Run tests for nested project
setup_nested_project
test_init "$TEST_DATA/nested_project" 0 "Initialize nested project"
check_index_content "$TEST_DATA/nested_project/.index" "intro.md
part1" "Root index content in nested project"
check_index_content "$TEST_DATA/nested_project/part1/.index" "chapter1
chapter2" "Part1 index content in nested project"
check_index_content "$TEST_DATA/nested_project/part1/chapter1/.index" "scene1.md
scene2.md" "Chapter1 index content in nested project"
check_index_content "$TEST_DATA/nested_project/part1/chapter2/.index" "scene1.md" "Chapter2 index content in nested project"

# Run tests for special names
setup_special_names_project
test_init "$TEST_DATA/special_names" 0 "Initialize project with special filenames"
check_index_content "$TEST_DATA/special_names/.index" "file with spaces.md
file-with-dashes.md
file.with.dots.md
file@with@special@chars.md
file_with_underscores.md" "Special names index content"

# Run test for re-initialization
# First initialize, then modify index, then re-initialize
test_init "$TEST_DATA/simple_project" 0 "Re-initialize existing project"
# Modify index to remove one file (use sed differently on MacOS vs Linux)
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    sed -i '' '/chapter2.md/d' "$TEST_DATA/simple_project/.index"
else
    # Linux
    sed -i '/chapter2.md/d' "$TEST_DATA/simple_project/.index"
fi
# Re-run initialization
test_init "$TEST_DATA/simple_project" 0 "Re-initialize after index modification"
check_index_content "$TEST_DATA/simple_project/.index" "chapter1.md
chapter2.md" "Re-initialized index content"

# Run tests for error conditions
setup_permissions_test
test_init "$TEST_DATA/permissions/readonly_dir" 1 "Initialize read-only directory"
test_init "$TEST_DATA/permissions/nowrite_dir" 1 "Initialize directory with read-only index"

# Clean up
cleanup_test_projects() {
    chmod -R 777 "$TEST_DATA/permissions" 2>/dev/null || true
    rm -rf "$TEST_DATA/empty_project"
    rm -rf "$TEST_DATA/simple_project"
    rm -rf "$TEST_DATA/nested_project"
    rm -rf "$TEST_DATA/special_names"
    rm -rf "$TEST_DATA/permissions"
}

cleanup_test_projects
