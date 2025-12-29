#!/bin/bash

# Run all tests

echo "========================================="
echo "Running All Tests"
echo "========================================="
echo ""

# Change to tests directory
cd "$(dirname "$0")"

# Ensure all test scripts are executable
chmod +x test_basic_operations.sh
chmod +x test_ttl_expiration.sh
chmod +x test_persistence.sh
chmod +x test_replication.sh
chmod +x test_concurrent_clients.sh

# Track test results
TESTS_PASSED=0
TESTS_FAILED=0

run_test() {
    local test_name=$1
    local test_script=$2
    
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Running: $test_name"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if ./$test_script; then
        echo "✓ $test_name PASSED"
        ((TESTS_PASSED++))
    else
        echo "✗ $test_name FAILED"
        ((TESTS_FAILED++))
    fi
    
    # Clean up any lingering processes
    pkill -f kvstore_server 2>/dev/null
    sleep 1
}

# Run all tests
run_test "Basic Operations" "test_basic_operations.sh"
run_test "TTL & Expiration" "test_ttl_expiration.sh"
run_test "Persistence (RDB + AOF)" "test_persistence.sh"
run_test "Master-Replica Replication" "test_replication.sh"
run_test "Concurrent Clients" "test_concurrent_clients.sh"

# Summary
echo ""
echo "========================================="
echo "Test Summary"
echo "========================================="
echo "Passed: $TESTS_PASSED"
echo "Failed: $TESTS_FAILED"
echo "Total:  $((TESTS_PASSED + TESTS_FAILED))"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo "✓ All tests passed!"
    exit 0
else
    echo "✗ Some tests failed"
    exit 1
fi
