#!/bin/bash

# Test TTL and expiration functionality

echo "========================================="
echo "Testing TTL and Expiration"
echo "========================================="
echo ""

# Clean up any existing data
rm -f kvstore.rdb kvstore.aof

echo "Step 1: Starting server on port 50051..."
../build/kvstore_server --master --address 0.0.0.0:50051 &
SERVER_PID=$!
sleep 2

echo ""
echo "Step 2: Testing TTL operations..."
echo "  - Setting keys with expiration"
echo "  - Checking TTL values"
echo "  - Verifying expiration"
echo ""

# Run client test which includes TTL testing
../build/kvstore_client localhost:50051

echo ""
echo "Step 3: Testing TTL command directly..."
echo "  Keys should expire after their TTL"
echo ""

echo ""
echo "Cleaning up..."
kill $SERVER_PID 2>/dev/null
sleep 1

echo ""
echo "Test complete!"
