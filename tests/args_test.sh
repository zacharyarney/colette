#!/bin/bash

# Test counter initialization
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Test helper function
test_colette() {
    local cmd="$1"
    local expected_status="$2"
    local expected_output="$3"
    local test_name="$4"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "\n${YELLOW}Test $TESTS_RUN: $test_name${NC}"
    echo "Command: $cmd"
    
    # Run command and capture output and status
    output=$($cmd 2>&1)
    status=$?
    
    # Check exit status
    if [ $status -eq $expected_status ]; then
        echo -e "${GREEN}✓ Exit status correct ($status)${NC}"
        status_passed=true
    else
        echo -e "${RED}✗ Expected status $expected_status, got $status${NC}"
        status_passed=false
    fi
    
    # Check output
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
    
    # Update test counts
    if [ "$status_passed" = true ] && [ "$output_passed" = true ]; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Set up test data for this test suite
mkdir -p "$TEST_DATA/project/chapter1"
mkdir -p "$TEST_DATA/project/chapter2"
mkdir -p "$TEST_DATA/project/_notes"
touch "$TEST_DATA/project/chapter1/scene1.md"
touch "$TEST_DATA/project/chapter1/scene2.md"
touch "$TEST_DATA/project/chapter2/scene1.md"
touch "$TEST_DATA/project/_notes/outline.md"

# Basic usage tests
test_colette "$COLETTE $TEST_DATA/project" 0 "Success" \
    "Basic directory path"

test_colette "$COLETTE -i $TEST_DATA/project" 0 "Success" \
    "Init mode"

test_colette "$COLETTE -c $TEST_DATA/project" 0 "Success" \
    "Check mode"

test_colette "$COLETTE -p 5 $TEST_DATA/project" 0 "Success" \
    "Custom padding"

# Error cases
test_colette "$COLETTE" 1 "No directory specified" \
    "Missing directory"

test_colette "$COLETTE $TEST_DATA/nonexistent" 1 "Invalid directory path" \
    "Invalid directory"

test_colette "$COLETTE -p 0 $TEST_DATA/project" 1 "Prefix padding must be a value from 1 to 10" \
    "Padding range"

test_colette "$COLETTE -p 110 $TEST_DATA/project" 1 "Prefix padding must be a value from 1 to 10" \
    "Padding range"
test_colette "$COLETTE -p abc $TEST_DATA/project" 1 "Invalid prefix padding" \
    "Non-numeric padding"

test_colette "$COLETTE -i -c $TEST_DATA/project" 1 "Conflicting options" \
    "Conflicting flags"

test_colette "$COLETTE -x $TEST_DATA/project" 1 "Invalid option" \
    "Invalid option flag"

# Path tests
test_colette "$COLETTE ." 0 "Success" \
    "Current directory"

test_colette "$COLETTE $TEST_DATA/project/../project" 0 "Success" \
    "Path with parent directory reference"

# Space in path test
mkdir -p "$TEST_DATA/test project"
test_colette "$COLETTE \"$TEST_DATA/test project\"" 1 "Invalid directory path" \
    "Path with spaces"
rm -rf "$TEST_DATA/test project"
