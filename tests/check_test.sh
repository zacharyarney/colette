#!/bin/bash

# Test counter initialization
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Test helper function that runs colette in check mode and verifies the results
test_check_mode() {
    local project_dir="$1"
    local expected_status="$2"
    local expected_output="$3"
    local test_name="$4"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "\n${YELLOW}Test $TESTS_RUN: $test_name${NC}"
    
    # Run colette in check mode and capture both output and status
    output=$($COLETTE --check "$project_dir" 2>&1)
    status=$?
    
    # Verify the exit status matches what we expect
    if [ $status -eq $expected_status ]; then
        echo -e "${GREEN}✓ Exit status correct ($status)${NC}"
        status_passed=true
    else
        echo -e "${RED}✗ Expected status $expected_status, got $status${NC}"
        status_passed=false
    fi
    
    # Verify the output contains our expected message
    if [[ "$output" == *"$expected_output"* ]]; then
        echo -e "${GREEN}✓ Output matches expected${NC}"
        output_passed=true
    else
        echo -e "${RED}✗ Expected output containing:${NC}"
        echo -e "${RED}'$expected_output'${NC}"
        echo -e "${RED}Got:${NC}"
        echo -e "${RED}'$output'${NC}"
        output_passed=false
    fi
    
    # Update our test counts based on results
    if [ "$status_passed" = true ] && [ "$output_passed" = true ]; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Creates our test project structures
setup_test_projects() {
    # A valid minimal project just needs a root index 
    # pointing to at least one valid file
    mkdir -p "$TEST_DATA/minimal_valid"
    echo "chapter1.md" > "$TEST_DATA/minimal_valid/.index"
    echo "Chapter 1 content" > "$TEST_DATA/minimal_valid/chapter1.md"

    # A valid nested project demonstrates the full structure
    mkdir -p "$TEST_DATA/nested_valid/chapter1"
    echo "chapter1" > "$TEST_DATA/nested_valid/.index"
    echo "scene1.md" > "$TEST_DATA/nested_valid/chapter1/.index"
    echo "Scene 1 content" > "$TEST_DATA/nested_valid/chapter1/scene1.md"

    # A project with a missing root index file
    mkdir -p "$TEST_DATA/no_index"
    touch "$TEST_DATA/no_index/chapter1.md"

    # A project where index references a nonexistent file
    mkdir -p "$TEST_DATA/missing_file"
    echo "nonexistent.md" > "$TEST_DATA/missing_file/.index"

    # A project with an empty index file
    mkdir -p "$TEST_DATA/empty_index"
    touch "$TEST_DATA/empty_index/.index"
}

setup_filesystem_edge_cases() {
    local base_dir="$TEST_DATA/edge_cases"
    
    # === Symlink test setup ===
    mkdir -p "$base_dir/symlink_project"
    echo "valid_link.md" > "$base_dir/symlink_project/.index"
    echo "dir_link" >> "$base_dir/symlink_project/.index"
    echo "circular_link.md" >> "$base_dir/symlink_project/.index"
    
    # Create target file for valid symlink
    echo "Content" > "$base_dir/symlink_project/target.md"
    ln -s "target.md" "$base_dir/symlink_project/valid_link.md"
    
    # Create directory structure for directory symlink
    mkdir -p "$base_dir/symlink_project/target_dir"
    echo "file.md" > "$base_dir/symlink_project/target_dir/.index"
    echo "Content" > "$base_dir/symlink_project/target_dir/file.md"
    ln -s "target_dir" "$base_dir/symlink_project/dir_link"
    
    # Create circular symlink
    ln -s "circular_link.md" "$base_dir/symlink_project/circular_link.md"
    
    # === Special names test setup ===
    mkdir -p "$base_dir/special_names"
    echo "file with spaces.md" > "$base_dir/special_names/.index"
    echo "café.md" >> "$base_dir/special_names/.index"
    echo "file@#$%^&.md" >> "$base_dir/special_names/.index"
    
    touch "$base_dir/special_names/file with spaces.md"
    touch "$base_dir/special_names/café.md"
    touch "$base_dir/special_names/file@#$%^&.md"
    
    # === Long names test setup ===
    mkdir -p "$base_dir/long_names"
    # Create a filename that's exactly 255 characters
    local long_name=$(printf 'a%.0s' {1..251})".md"
    echo "$long_name" > "$base_dir/long_names/.index"
    touch "$base_dir/long_names/$long_name"
    
    # === Permission test setup ===
    mkdir -p "$base_dir/permissions/readonly_dir"
    mkdir -p "$base_dir/permissions/noexec_dir"
    mkdir -p "$base_dir/permissions/noread_file"
    
    echo "file.md" > "$base_dir/permissions/readonly_dir/.index"
    echo "Content" > "$base_dir/permissions/readonly_dir/file.md"
    chmod 555 "$base_dir/permissions/readonly_dir"
    
    echo "file.md" > "$base_dir/permissions/noexec_dir/.index"
    echo "Content" > "$base_dir/permissions/noexec_dir/file.md"
    chmod 666 "$base_dir/permissions/noexec_dir"
    
    echo "noread.md" > "$base_dir/permissions/noread_file/.index"
    echo "Content" > "$base_dir/permissions/noread_file/noread.md"
    chmod 000 "$base_dir/permissions/noread_file/noread.md"
    
    # === Hidden files test setup ===
    mkdir -p "$base_dir/hidden_files"
    mkdir -p "$base_dir/hidden_files/.hidden_dir"
    mkdir -p "$base_dir/hidden_files/_ignored_dir"
    
    echo ".hidden_file.md" > "$base_dir/hidden_files/.index"
    echo "_ignored_file.md" >> "$base_dir/hidden_files/.index"
    echo ".hidden_dir" >> "$base_dir/hidden_files/.index"
    echo "_ignored_dir" >> "$base_dir/hidden_files/.index"
    
    touch "$base_dir/hidden_files/.hidden_file.md"
    touch "$base_dir/hidden_files/_ignored_file.md"
    touch "$base_dir/hidden_files/.hidden_dir/file.md"
    touch "$base_dir/hidden_files/_ignored_dir/file.md"
}

setup_index_content_cases() {
    local base_dir="$TEST_DATA/index_cases"
    
    # === Blank lines and comments case ===
    mkdir -p "$base_dir/formatted_index"
    cat > "$base_dir/formatted_index/.index" << EOL
# Introduction section
intro.md

# Character development
chapter1.md

# Important climax scene!
chapter2.md
EOL
    touch "$base_dir/formatted_index/intro.md"
    touch "$base_dir/formatted_index/chapter1.md"
    touch "$base_dir/formatted_index/chapter2.md"
    
    # === Duplicate entries case ===
    mkdir -p "$base_dir/duplicates"
    cat > "$base_dir/duplicates/.index" << EOL
chapter1.md
chapter2.md
chapter1.md
EOL
    touch "$base_dir/duplicates/chapter1.md"
    touch "$base_dir/duplicates/chapter2.md"
    
    # === Path traversal cases ===
    mkdir -p "$base_dir/path_cases/project"
    mkdir -p "$base_dir/path_cases/outside"
    
    # Create a file outside the project directory
    echo "External content" > "$base_dir/path_cases/outside/external.md"
    
    # Create index with relative path
    cat > "$base_dir/path_cases/project/.index" << EOL
chapter1.md
../outside/external.md
EOL
    touch "$base_dir/path_cases/project/chapter1.md"
    
    # Create index with absolute path
    mkdir -p "$base_dir/absolute_paths"
    cat > "$base_dir/absolute_paths/.index" << EOL
chapter1.md
/tmp/external.md
EOL
    touch "$base_dir/absolute_paths/chapter1.md"
    touch "/tmp/external.md"  # Create temporary external file
}

setup_depth_test_cases() {
    local base_dir="$TEST_DATA/depth_cases"
    
    # === Valid Maximum Depth Case ===
    # This creates a project that's exactly at the maximum allowed depth
    local current_dir="$base_dir/max_depth"
    mkdir -p "$current_dir"
    
    # Create the initial index file
    echo "level2" > "$current_dir/.index"
    
    # Create each level, going down to depth 5
    for i in {2..5}; do
        current_dir="$current_dir/level$i"
        mkdir -p "$current_dir"
        
        if [ $i -eq 5 ]; then
            # At maximum depth, point to a file
            echo "final.md" > "$current_dir/.index"
            echo "Final content" > "$current_dir/final.md"
        else
            # At intermediate depths, point to next directory
            echo "level$(($i + 1))" > "$current_dir/.index"
        fi
    done
    
    # === Exceeded Depth Case ===
    # This creates a project that exceeds the maximum depth
    current_dir="$base_dir/exceeded_depth"
    mkdir -p "$current_dir"
    
    # Create the initial index file
    echo "level1" > "$current_dir/.index"
    
    # Create levels going beyond the maximum depth
    for i in {1..6}; do
        current_dir="$current_dir/level$i"
        mkdir -p "$current_dir"
        
        if [ $i -eq 6 ]; then
            # Beyond maximum depth
            echo "too_deep.md" > "$current_dir/.index"
            echo "Too deep content" > "$current_dir/too_deep.md"
        else
            # At intermediate depths, point to next directory
            echo "level$(($i + 1))" > "$current_dir/.index"
        fi
    done
}

# Run our tests
setup_test_projects

# Test a minimal but valid project structure
test_check_mode "$TEST_DATA/minimal_valid" 0 "Success" \
    "Minimal valid project structure"

# Test a nested but valid project structure
test_check_mode "$TEST_DATA/nested_valid" 0 "Success" \
    "Nested valid project structure"

# Test a project missing its root index file
test_check_mode "$TEST_DATA/no_index" 1 "Required .index file not found" \
    "Missing root index file"

# Test a project with an index that points to a nonexistent file
test_check_mode "$TEST_DATA/missing_file" 1 "File not found" \
    "Index references nonexistent file"

# Test a project with an empty index file
test_check_mode "$TEST_DATA/empty_index" 0 "Success" \
    "Empty index file"

# Run tests for filesystem edge cases
setup_filesystem_edge_cases

# Test symlink handling
test_check_mode "$TEST_DATA/edge_cases/symlink_project" 1 "Symbolic links not permitted" \
    "Project with symlinks"

# Test special character handling
test_check_mode "$TEST_DATA/edge_cases/special_names" 0 "Success" \
    "Project with special characters in filenames"

# Test long name handling
test_check_mode "$TEST_DATA/edge_cases/long_names" 0 "Success" \
    "Project with maximum length filenames"

# Test permission handling
test_check_mode "$TEST_DATA/edge_cases/permissions/readonly_dir" 0 "Success" \
    "Project with read-only directory"

test_check_mode "$TEST_DATA/edge_cases/permissions/noexec_dir" 1 "Permission denied" \
    "Project with non-executable directory"

test_check_mode "$TEST_DATA/edge_cases/permissions/noread_file" 1 "Permission denied" \
    "Project with unreadable file"

# Test hidden file handling
test_check_mode "$TEST_DATA/edge_cases/hidden_files" 0 "Success" \
    "Project with hidden and ignored files"

# Run tests for index content cases
setup_index_content_cases

# Test blank lines and comments in index
test_check_mode "$TEST_DATA/index_cases/formatted_index" 0 "Success" \
    "Index with comments and blank lines"

# Test duplicate entries
test_check_mode "$TEST_DATA/index_cases/duplicates" 0 "Success" \
    "Index with duplicate entries"

# Test relative path traversal
test_check_mode "$TEST_DATA/index_cases/path_cases/project" 0 "Success" \
    "Index with relative path traversal"

# Test absolute paths
test_check_mode "$TEST_DATA/index_cases/absolute_paths" 1 "validating path component (absolute path in index file)" \
    "Index with nonexistent file (absolute path)"

# Clean up the temporary file we created
cleanup_index_cases() {
    rm -f "/tmp/external.md"
}

#Run tests for project depth
setup_depth_test_cases

# Test maximum allowed depth
test_check_mode "$TEST_DATA/depth_cases/max_depth" 0 "Success" \
    "Project at maximum allowed depth"

# Test exceeded depth
test_check_mode "$TEST_DATA/depth_cases/exceeded_depth" 1 "Project hierarchy exceeds maximum depth" \
    "Project exceeding maximum depth"

# Clean up test files
cleanup_test_projects() {
    rm -rf "$TEST_DATA/minimal_valid"
    rm -rf "$TEST_DATA/nested_valid"
    rm -rf "$TEST_DATA/no_index"
    rm -rf "$TEST_DATA/missing_file"
    rm -rf "$TEST_DATA/empty_index"
    rm -rf "$TEST_DATA/depth_cases"
    rm -rf "$TEST_DATA/index_cases"
    chmod 777 "$TEST_DATA/edge_cases/permissions/readonly_dir"
    chmod 777 "$TEST_DATA/edge_cases/permissions/noexec_dir"
    chmod 777 "$TEST_DATA/edge_cases/permissions/noread_file/noread.md"
    rm -rf "$TEST_DATA/edge_cases"
}

cleanup_test_projects
