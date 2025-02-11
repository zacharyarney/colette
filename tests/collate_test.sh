#!/bin/bash

# Initialize test counters (required by run_tests.sh)
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Lorem ipsum text for longer files
LOREM="Lorem ipsum odor amet, consectetuer adipiscing elit. Lectus viverra habitant vulputate; suspendisse dolor nulla. Potenti ullamcorper etiam faucibus blandit at. Maximus venenatis ultrices proin integer sapien magna. Dictumst dolor pulvinar commodo consequat integer posuere. Commodo cursus justo ullamcorper sagittis varius vel ut. Himenaeos porta facilisi montes iaculis aliquam augue placerat lobortis. Fermentum elementum in maecenas pulvinar facilisis leo orci. Ante rhoncus congue mi nibh himenaeos torquent." 

get_file_size() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # MacOS
        stat -f %z "$1"
    else
        # Linux and others
        stat --format="%s" "$1"
    fi
}

# Test helper function for collate mode
test_collate() {
    local project_dir="$1"
    local expected_status="${2:-0}"  # Default to 0 if not provided
    local expected_content="$3"
    local test_name="$4"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "\n${YELLOW}Test $TESTS_RUN: $test_name${NC}"
    
    # Run colette in collate mode
    output=$($COLETTE "$project_dir" 2>&1)
    status=$?
    
    # First check if the status matches expected
    if [ $status -eq $expected_status ]; then
        echo -e "${GREEN}✓ Exit status correct ($status)${NC}"
        status_passed=true
    else
        echo -e "${RED}✗ Expected status $expected_status, got $status${NC}"
        status_passed=false
    fi
    
    # Only check content if we expect success
    if [ $expected_status -eq 0 ]; then
        # Read the output file content
        content=$(cat "$project_dir/_draft_.md")
        
        # Compare with expected content
        if [ "$content" = "$expected_content" ]; then
            echo -e "${GREEN}✓ Content matches expected${NC}"
            output_passed=true
        else
            echo -e "${RED}✗ Content mismatch${NC}"
            echo -e "${RED}Expected:${NC}\n$expected_content"
            echo -e "${RED}Got:${NC}\n$content"
            output_passed=false
        fi
    else
        # For error cases, we consider output check passed if status was correct
        output_passed=$status_passed
    fi
    
    # Update test counts
    if [ "$status_passed" = true ] && [ "$output_passed" = true ]; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

test_collate_length() {
    local project_dir="$1"
    local test_name="$2"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "\n${YELLOW}Test $TESTS_RUN: $test_name${NC}"
    
    # Run colette in collate mode
    output=$($COLETTE "$project_dir" 2>&1)
    status=$?
    
    if [ $status -eq 0 ]; then
        # Get the actual size of output file
        local actual_size=$(get_file_size "$project_dir/_draft_.md")
        # Get sizes of input files
        local large_size=$(get_file_size "$project_dir/large_file.md")
        local small_size=$(get_file_size "$project_dir/small_file.md")
        
        # Add 2 for the newline after each file
        local expected_size=$((large_size + small_size + 2))
        
        if [ "$actual_size" -eq "$expected_size" ]; then
            echo -e "${GREEN}✓ File size matches expected ($actual_size bytes)${NC}"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            echo -e "${RED}✗ Size mismatch${NC}"
            echo -e "${RED}Expected: $expected_size bytes${NC}"
            echo -e "${RED}Got: $actual_size bytes${NC}"
            TESTS_FAILED=$((TESTS_FAILED + 1))
        fi
    else
        echo -e "${RED}✗ Collation failed with status $status${NC}"
        echo -e "${RED}Output: $output${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Set up simple project structure
setup_simple_project() {
    local dir="$TEST_DATA/simple_project"
    mkdir -p "$dir"
    
    # Create root index
    echo "chapter1.md" > "$dir/.index"
    echo "chapter2.md" >> "$dir/.index"
    
    # Create content files
    echo "Chapter 1 content" > "$dir/chapter1.md"
    echo "Chapter 2 content" > "$dir/chapter2.md"
}

# Set up nested project structure
setup_nested_project() {
    local dir="$TEST_DATA/nested_project"
    mkdir -p "$dir/part1/chapter1"
    
    # Create root index
    echo "part1" > "$dir/.index"
    
    # Create part1 index
    echo "chapter1" > "$dir/part1/.index"
    
    # Create chapter1 index
    echo "scene1.md" > "$dir/part1/chapter1/.index"
    echo "scene2.md" >> "$dir/part1/chapter1/.index"
    
    # Create content files
    echo "Scene 1 content" > "$dir/part1/chapter1/scene1.md"
    echo "Scene 2 content" > "$dir/part1/chapter1/scene2.md"
}

# Set up project with long files
setup_large_file_project() {
    local dir="$TEST_DATA/large_file_project"
    mkdir -p "$dir"
    
    # Create root index
    echo "large_file.md" > "$dir/.index"
    echo "small_file.md" >> "$dir/.index"
    
    # Create small file for comparison
    echo "Small file content" > "$dir/small_file.md"
    
    # Generate large file content (>8192 bytes)
    for i in {1..101}; do
        echo "$LOREM" >> "$dir/large_file.md"
    done
}

# Set up project with edge cases
setup_edge_cases() {
    local dir="$TEST_DATA/edge_cases"
    mkdir -p "$dir"
    
    # Create root index with empty lines and comments
    cat > "$dir/.index" << EOL
# Introduction
intro.md

# Main content
content.md
EOL
    
    # Create content files
    echo "Introduction with special chars: áéíóú" > "$dir/intro.md"
    printf "Content with various newlines.\n\n\nMultiple empty lines.\n" > "$dir/content.md"
}

# Set up error case projects
setup_error_cases() {
    local dir="$TEST_DATA/error_cases"
    mkdir -p "$dir/no_permission" "$dir/missing_file"
    
    # Project with permission issues
    echo "valid_file.md" > "$dir/no_permission/.index"
    echo "file.md" >> "$dir/no_permission/.index"
    echo "Valid content" > "$dir/no_permission/valid_file.md"
    echo "Content that will be unreadable" > "$dir/no_permission/file.md"
    chmod 000 "$dir/no_permission/file.md"
    
    # Project with missing files
    echo "valid_file.md" > "$dir/missing_file/.index"
    echo "nonexistent.md" >> "$dir/missing_file/.index"
    echo "Valid content" > "$dir/missing_file/valid_file.md"
}

# Run the tests
setup_simple_project
test_collate "$TEST_DATA/simple_project" 0 \
    "Chapter 1 content

Chapter 2 content" \
    "Simple project collation"

setup_nested_project
test_collate "$TEST_DATA/nested_project" 0 \
    "Scene 1 content

Scene 2 content" \
    "Nested project collation"

setup_large_file_project
test_collate_length "$TEST_DATA/large_file_project" "Large file size test"

setup_edge_cases
test_collate "$TEST_DATA/edge_cases" 0 \
    "Introduction with special chars: áéíóú

Content with various newlines.


Multiple empty lines." \
    "Edge cases (special chars, varied newlines)"

setup_error_cases
# Test missing file error
test_collate "$TEST_DATA/error_cases/missing_file" 1 \
    "Valid content" \
    "Missing file error handling"

# Test permission error
test_collate "$TEST_DATA/error_cases/no_permission" 1 \
    "Valid content" \
    "Permission error handling"

# Clean up
cleanup_test_projects() {
    chmod 666 "$TEST_DATA/error_cases/no_permission/file.md"
    rm -rf "$TEST_DATA/simple_project"
    rm -rf "$TEST_DATA/nested_project"
    rm -rf "$TEST_DATA/edge_cases"
    rm -rf "$TEST_DATA/error_cases"
}

cleanup_test_projects
