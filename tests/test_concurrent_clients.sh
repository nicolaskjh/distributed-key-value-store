#!/bin/bash

# Test concurrent client connections

echo "========================================="
echo "Testing Concurrent Clients"
echo "========================================="
echo ""

# Clean up any existing data
rm -f kvstore.rdb kvstore.aof

echo "Step 1: Starting server on port 50051..."
../build/kvstore_server --master --address 0.0.0.0:50051 &
SERVER_PID=$!
sleep 2

echo ""
echo "Step 2: Running 5 concurrent clients..."
echo "  Each client performs SET, GET, DELETE operations"
echo ""

for i in {1..5}; do
    echo "  Starting client $i..."
    ../build/kvstore_client localhost:50051 &
    CLIENT_PIDS[$i]=$!
done

echo ""
echo "Step 3: Waiting for all clients to complete..."
for pid in "${CLIENT_PIDS[@]}"; do
    wait $pid
done

echo ""
echo "Step 4: Verifying data integrity..."
../build/kvstore_client localhost:50051 | grep "GET name"

echo ""
echo "Cleaning up..."
kill $SERVER_PID 2>/dev/null
sleep 1

echo ""
echo "Test complete!"
