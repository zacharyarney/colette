#!/bin/bash

# Get absolute paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
# export LSAN_OPTIONS="detect_leaks=1:verbosity=2:report_objects=1:max_leaks=0"
export TEST_DATA="$SCRIPT_DIR/testdata"
export COLETTE="$PROJECT_ROOT/bin/colette"

# ANSI color codes for output
export RED='\033[0;31m'
export GREEN='\033[0;32m'
export YELLOW='\033[1;33m'
export NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS_RUN=0
TOTAL_TESTS_PASSED=0
TOTAL_TESTS_FAILED=0

# Create test environment
setup_test_env() {
    echo -e "${YELLOW}Setting up test environment...${NC}"
    rm -rf "$TEST_DATA"
    mkdir -p "$TEST_DATA"
}

# Clean up test environment
cleanup_test_env() {
    echo -e "\n${YELLOW}Cleaning up test environment...${NC}"
    rm -rf "$TEST_DATA"
}

# Run all test files
run_all_tests() {
    local test_files=("$SCRIPT_DIR"/*_test.sh)
    local exit_status=0
    
    for test_file in "${test_files[@]}"; do
        if [ "$(basename "$test_file")" != "$(basename "$0")" ]; then
            echo -e "\n${YELLOW}Running $(basename "$test_file")...${NC}"
            
            # Source the test file to get its results
            source "$test_file"
            
            # Accumulate test results
            TOTAL_TESTS_RUN=$((TOTAL_TESTS_RUN + TESTS_RUN))
            TOTAL_TESTS_PASSED=$((TOTAL_TESTS_PASSED + TESTS_PASSED))
            TOTAL_TESTS_FAILED=$((TOTAL_TESTS_FAILED + TESTS_FAILED))
            
            # Track if any test suite failed
            if [ $TESTS_FAILED -gt 0 ]; then
                exit_status=1
            fi
        fi
    done
    
    return $exit_status
}

# Print final summary
print_final_summary() {
    echo -e "\n${YELLOW}Final Test Summary:${NC}"
    echo -e "Total tests run:    $TOTAL_TESTS_RUN"
    echo -e "${GREEN}Total tests passed: $TOTAL_TESTS_PASSED${NC}"
    echo -e "${RED}Total tests failed: $TOTAL_TESTS_FAILED${NC}"
    
    if [ $TOTAL_TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}All test suites passed!${NC}"
        exit 0
    else
        echo -e "\n${RED}Some test suites failed.${NC}"
        exit 1
    fi
}

# Main execution
setup_test_env
run_all_tests
cleanup_test_env
print_final_summary
