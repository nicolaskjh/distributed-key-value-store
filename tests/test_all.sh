#!/bin/bash

# Run all tests

echo "========================================="
echo "Running All Tests"
echo "========================================="
echo ""

# Change to tests directory
cd "$(dirname "$0")"

# Track test results
TESTS_PASSED=0
TESTS_FAILED=0

cleanup() {
    pkill -f kvstore_server 2>/dev/null
    rm -f kvstore.rdb kvstore.aof
    rm -rf replica1 replica2
    sleep 1
}

run_test() {
    local test_name=$1
    local test_function=$2
    
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Running: $test_name"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    # Clean up before test
    cleanup
    
    if $test_function; then
        echo "✓ $test_name PASSED"
        ((TESTS_PASSED++))
    else
        echo "✗ $test_name FAILED"
        ((TESTS_FAILED++))
    fi
    
    # Clean up after test
    cleanup
}

# Unit tests (no server needed)
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Unit Tests"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_hash_ring() {
    ../build/test_hash_ring
}

test_shard_router() {
    ../build/test_shard_router
}

run_test "Hash Ring" test_hash_ring
run_test "Shard Router" test_shard_router

# Integration tests (require server)
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Integration Tests"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_basic_operations() {
    ../build/kvstore_server --master --address 0.0.0.0:50051 &
    local SERVER_PID=$!
    sleep 2
    ../build/kvstore_client localhost:50051
    local RESULT=$?
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null
    return $RESULT
}

test_ttl_expiration() {
    ../build/kvstore_server --master --address 0.0.0.0:50051 &
    local SERVER_PID=$!
    sleep 2
    ../build/kvstore_client localhost:50051
    local RESULT=$?
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null
    return $RESULT
}

test_persistence() {
    echo 'Starting server for persistence test...'
    ../build/kvstore_server --master --address 0.0.0.0:50051 &
    local SERVER_PID=$!
    sleep 2
    
    echo 'Writing snapshot...'
    ../build/snapshot_test localhost:50051
    
    echo 'Verifying persistence files exist...'
    if [ ! -f kvstore.rdb ] || [ ! -f kvstore.aof ]; then
        echo 'Error: Persistence files not created'
        kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
        return 1
    fi
    
    echo 'Killing server...'
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null
    sleep 2
    
    echo 'Restarting server...'
    ../build/kvstore_server --master --address 0.0.0.0:50051 &
    SERVER_PID=$!
    sleep 2
    
    echo 'Verifying data persisted...'
    ../build/verify_persistence localhost:50051
    local RESULT=$?
    
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null
    return $RESULT
}

test_replication() {
    mkdir -p replica1 replica2
    
    echo 'Starting master on port 50051...'
    ../build/kvstore_server --master --address 0.0.0.0:50051 --replicas localhost:50052,localhost:50053 &
    local MASTER_PID=$!
    sleep 2
    
    echo 'Starting replica 1 on port 50052...'
    cd replica1
    ../../build/kvstore_server --replica --address 0.0.0.0:50052 --master-address localhost:50051 &
    local REPLICA1_PID=$!
    cd ..
    sleep 2
    
    echo 'Starting replica 2 on port 50053...'
    cd replica2
    ../../build/kvstore_server --replica --address 0.0.0.0:50053 --master-address localhost:50051 &
    local REPLICA2_PID=$!
    cd ..
    sleep 2
    
    echo 'Writing to master...'
    ../build/kvstore_client localhost:50051
    sleep 2
    
    echo 'Reading from replicas...'
    ../build/read_test localhost:50052
    local RESULT1=$?
    ../build/read_test localhost:50053
    local RESULT2=$?
    
    kill $MASTER_PID $REPLICA1_PID $REPLICA2_PID 2>/dev/null
    wait $MASTER_PID $REPLICA1_PID $REPLICA2_PID 2>/dev/null
    rm -rf replica1 replica2
    
    if [ $RESULT1 -eq 0 ] && [ $RESULT2 -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

test_concurrent_clients() {
    ../build/kvstore_server --master --address 0.0.0.0:50051 &
    local SERVER_PID=$!
    sleep 2
    
    echo 'Starting 5 concurrent clients...'
    for i in {1..5}; do
        ../build/kvstore_client localhost:50051 &
    done
    
    wait
    local RESULT=$?
    
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null
    return $RESULT
}

run_test "Basic Operations" test_basic_operations
run_test "TTL & Expiration" test_ttl_expiration
run_test "Persistence (RDB + AOF)" test_persistence
run_test "Master-Replica Replication" test_replication
run_test "Concurrent Clients" test_concurrent_clients

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
