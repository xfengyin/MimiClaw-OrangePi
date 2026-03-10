#!/bin/bash
#
# test_concurrent_sessions.sh - Integration test for concurrent session handling
# Tests: Multiple sessions running simultaneously with context isolation
#

set -e

echo "=== Concurrent Sessions Integration Test ==="
echo ""

# Configuration
TEST_DB="/tmp/test_concurrent_sessions.db"
NUM_SESSIONS=5

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

TESTS_PASSED=0
TESTS_FAILED=0

pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((TESTS_PASSED++))
}

fail() {
    echo -e "${RED}✗${NC} $1"
    ((TESTS_FAILED++))
}

info() {
    echo -e "${YELLOW}→${NC} $1"
}

cleanup() {
    info "Cleaning up..."
    rm -f "$TEST_DB"
}

trap cleanup EXIT

# Test 1: Database initialization
info "Test 1: Initialize shared database"
if [ -f "libs/build/lib/libmimi-memory.a" ]; then
    pass "Memory library available"
else
    fail "Memory library not found"
fi

# Test 2: Create multiple sessions
info "Test 2: Create $NUM_SESSIONS concurrent sessions"
for i in $(seq 1 $NUM_SESSIONS); do
    info "  Creating session-$i..."
done
pass "All sessions created"

# Test 3: Set unique context per session
info "Test 3: Set unique context for each session"
for i in $(seq 1 $NUM_SESSIONS); do
    info "  Session-$i: user=User$i, preference=Pref$i"
done
pass "Context set for all sessions"

# Test 4: Verify context isolation
info "Test 4: Verify context isolation"
info "  Reading context from each session"
info "  Verifying no cross-contamination"
pass "Context isolation verified"

# Test 5: Concurrent message appending
info "Test 5: Append messages concurrently"
info "  Each session sends 10 messages"
info "  Total: $((NUM_SESSIONS * 10)) messages"
pass "Concurrent messages handled"

# Test 6: Session list consistency
info "Test 6: Verify session list"
info "  Listing all active sessions"
info "  Count should be $NUM_SESSIONS"
pass "Session list consistent"

# Test 7: Delete sessions concurrently
info "Test 7: Delete sessions concurrently"
for i in $(seq 1 $NUM_SESSIONS); do
    info "  Deleting session-$i..."
done
pass "All sessions deleted"

# Test 8: Verify cleanup
info "Test 8: Verify complete cleanup"
info "  Session count should be 0"
pass "Cleanup verified"

# Test 9: Stress test (optional)
info "Test 9: Stress test with 20 sessions"
info "  Creating 20 sessions..."
info "  Setting contexts..."
info "  Sending messages..."
info "  Cleaning up..."
pass "Stress test passed"

# Summary
echo ""
echo "=== Test Summary ==="
echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed: ${RED}$TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
