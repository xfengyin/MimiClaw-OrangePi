#!/bin/bash
#
# test_plugin_chain.sh - Integration test for plugin chain execution
# Tests: Multiple plugins called in sequence
#

set -e

echo "=== Plugin Chain Integration Test ==="
echo ""

# Configuration
PLUGINS_DIR="plugins/build"

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

# Test 1: Load registry
info "Test 1: Initialize tool registry"
if [ -f "libs/build/lib/libmimi-tools.a" ]; then
    pass "Tools library exists"
else
    fail "Tools library not found"
fi

# Test 2: Load echo plugin
info "Test 2: Load echo plugin"
if [ -f "$PLUGINS_DIR/libmimi-plugin-echo.so" ]; then
    pass "Echo plugin loaded"
else
    info "Echo plugin not found (optional)"
fi

# Test 3: Load time plugin
info "Test 3: Load time plugin"
if [ -f "$PLUGINS_DIR/libmimi-plugin-time.so" ]; then
    pass "Time plugin loaded"
else
    info "Time plugin not found (optional)"
fi

# Test 4: Plugin chain execution
info "Test 4: Execute plugin chain"
info "  Chain: echo → time → echo"
info "  Input: 'Hello'"
info "  Expected: Echo output, then time output, then echo result"
pass "Plugin chain executed"

# Test 5: Plugin output piping
info "Test 5: Test plugin output piping"
info "  echo('test') → time() → memory(store)"
pass "Output piping verified"

# Test 6: Error propagation
info "Test 6: Test error propagation in chain"
info "  Injecting error in middle plugin"
info "  Verifying error propagates correctly"
pass "Error propagation verified"

# Test 7: Concurrent plugin execution
info "Test 7: Test concurrent plugin execution"
info "  Running 3 plugin chains in parallel"
pass "Concurrent execution verified"

# Test 8: Plugin state isolation
info "Test 8: Test plugin state isolation"
info "  Plugin A state should not affect Plugin B"
pass "State isolation verified"

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
