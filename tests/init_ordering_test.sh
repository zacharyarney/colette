#!/bin/bash

# New tests focused on preserving pre-existing .index order and appending new files alphabetically.

# Local counters (aggregated by run_tests.sh)
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helpers
_pass() { echo -e "${GREEN}PASS${NC}"; TESTS_PASSED=$((TESTS_PASSED+1)); }
_fail() { echo -e "${RED}FAIL${NC}"; TESTS_FAILED=$((TESTS_FAILED+1)); }

# Assert index file content equals expected (exact match)
assert_index_equals() {
  local index_path="$1"
  local expected="$2"
  # Trim possible trailing whitespace differences by using printf as the ground truth
  local tmp_expected="$(mktemp)"
  printf "%s" "$expected" > "$tmp_expected"
  if diff -u "$tmp_expected" "$index_path" > /dev/null; then
    _pass
  else
    echo "Expected:"
    cat "$tmp_expected" | sed 's/^/  /'
    echo "Actual:"
    cat "$index_path" | sed 's/^/  /'
    _fail
  fi
  rm -f "$tmp_expected"
}

# Case 1: Pre-existing .index with custom (non-alpha) order is preserved;
#         new files are appended in alphabetical order.
preexisting_index_preserved_and_new_appended_sorted() {
  TESTS_RUN=$((TESTS_RUN+1))
  echo -e "\n${YELLOW}Test $TESTS_RUN: Pre-existing order preserved; new files appended alphabetically${NC}"

  local dir="$TEST_DATA/preexisting_order_case"
  rm -rf "$dir"; mkdir -p "$dir"

  # Files present
  touch "$dir/a.txt" "$dir/b.txt" "$dir/c.txt" "$dir/d.txt"

  # User-chosen custom order: deliberately non-alphabetical
  # Note: final newline matters to avoid auto-newline behavior differences.
  printf "c.txt\na.txt\n" > "$dir/.index"

  # Run init
  "$COLETTE" --init "$dir" >/dev/null 2>&1
  local status=$?

  if [ $status -ne 0 ]; then
    echo "Expected exit status 0, got $status"; _fail; return
  fi

  # Expect existing lines preserved in their original order,
  # and unseen files [b.txt, d.txt] appended in alpha order.
  local expected="c.txt
a.txt
b.txt
d.txt
"
  assert_index_equals "$dir/.index" "$expected"
}

# Case 2: Idempotency — running init again does not duplicate entries or reorder anything.
idempotent_second_run_no_changes() {
  TESTS_RUN=$((TESTS_RUN+1))
  echo -e "\n${YELLOW}Test $TESTS_RUN: Idempotent second run — no reordering or duplication${NC}"

  local dir="$TEST_DATA/idempotent_case"
  rm -rf "$dir"; mkdir -p "$dir"
  touch "$dir/a.txt" "$dir/b.txt" "$dir/c.txt"
  printf "b.txt\na.txt\nc.txt\n" > "$dir/.index"

  "$COLETTE" --init "$dir" >/dev/null 2>&1
  local status1=$?
  "$COLETTE" --init "$dir" >/dev/null 2>&1
  local status2=$?

  if [ $status1 -ne 0 ] || [ $status2 -ne 0 ]; then
    echo "Expected both runs to exit 0, got $status1 and $status2"; _fail; return
  fi

  local expected="b.txt
a.txt
c.txt
"
  assert_index_equals "$dir/.index" "$expected"
}

# Case 3: New files with tricky names (spaces, punctuation) get appended in correct alpha order.
append_tricky_names_sorted() {
  TESTS_RUN=$((TESTS_RUN+1))
  echo -e "\n${YELLOW}Test $TESTS_RUN: Append tricky names — sorted order for new entries${NC}"

  local dir="$TEST_DATA/tricky_names_case"
  rm -rf "$dir"; mkdir -p "$dir"
  # Preexisting set & order
  touch "$dir/alpha.txt" "$dir/zulu.txt"
  printf "zulu.txt\nalpha.txt\n" > "$dir/.index"

  # Add tricky filenames
  touch "$dir/Bravo.md" "$dir/bravo2.md" "$dir/ space.md" "$dir/!bang.txt"

  "$COLETTE" --init "$dir" >/dev/null 2>&1
  local status=$?
  if [ $status -ne 0 ]; then
    echo "Expected exit status 0, got $status"; _fail; return
  fi

  # scandir + alphasort is locale-dependent but usually orders this roughly as:
  # " !bang.txt", " space.md", "Bravo.md", "alpha.txt", "bravo2.md", "zulu.txt"
  # However, our logic *appends only unseen entries* in the order returned by alphasort.
  # So expected is: keep preexisting lines in their order, then unseen sorted:
  # zulu.txt
  # alpha.txt
  # !bang.txt
  #  space.md
  # Bravo.md
  # bravo2.md
  # (Note: filename with a leading space is intentional.)

  local expected="zulu.txt
alpha.txt
 space.md
!bang.txt
Bravo.md
bravo2.md
"
  assert_index_equals "$dir/.index" "$expected"
}

# Case 4: Nested directories — each directory gets/keeps its own .index; subdirs added and traversed.
nested_directories_preserve_and_append() {
  TESTS_RUN=$((TESTS_RUN+1))
  echo -e "\n${YELLOW}Test $TESTS_RUN: Nested directories — preserve parent order and append children${NC}"

  local root="$TEST_DATA/nested_case"
  local ch1="$root/ch1"
  local ch2="$root/ch2"
  rm -rf "$root"; mkdir -p "$ch1" "$ch2"

  # Prepare files
  touch "$root/r1.txt" "$root/r2.txt"
  touch "$ch1/a.txt" "$ch1/c.txt"
  touch "$ch2/b.txt"

  # Parent has custom order and references to subdirs will be appended by init
  printf "r2.txt\nr1.txt\n" > "$root/.index"
  # Child ch1 existing order non-alpha
  printf "c.txt\na.txt\n" > "$ch1/.index"
  # ch2 has no .index yet

  "$COLETTE" --init "$root" >/dev/null 2>&1
  local status=$?
  if [ $status -ne 0 ]; then
    echo "Expected exit status 0, got $status"; _fail; return
  fi

  # root: r2.txt, r1.txt (preserved), then 'ch1', 'ch2' appended in alpha order
  local expected_root="r2.txt
r1.txt
ch1
ch2
"
  assert_index_equals "$root/.index" "$expected_root"

  # ch1: keep existing order, and no new files (since only a.txt, c.txt existed)
  local expected_ch1="c.txt
a.txt
"
  assert_index_equals "$ch1/.index" "$expected_ch1"

  # ch2: new index created, listing 'b.txt'
  local expected_ch2="b.txt
"
  assert_index_equals "$ch2/.index" "$expected_ch2"
}

# Execute cases
preexisting_index_preserved_and_new_appended_sorted
idempotent_second_run_no_changes
append_tricky_names_sorted
nested_directories_preserve_and_append

# Print suite summary (consumed by run_tests.sh)
echo -e "\n${YELLOW}init_ordering_test.sh summary:${NC}"
echo "  Tests run:    $TESTS_RUN"
echo "  Tests passed: $TESTS_PASSED"
echo "  Tests failed: $TESTS_FAILED"
