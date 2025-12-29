#!/bin/bash

# Test persistence (RDB + AOF)

echo "========================================="
echo "Testing Persistence (RDB + AOF)"
echo "========================================="
echo ""

# Clean up any existing data
rm -f kvstore.rdb kvstore.aof

echo "Step 1: Starting server on port 50051..."
../build/kvstore_server --master --address 0.0.0.0:50051 &
SERVER_PID=$!
sleep 2

echo ""
echo "Step 2: Writing test data..."
../build/snapshot_test

echo ""
echo "Step 3: Waiting 65 seconds for RDB snapshot..."
echo "  (Snapshots occur every 60 seconds)"
for i in {65..1}; do
    printf "\r  Waiting... %02d seconds remaining" $i
    sleep 1
done
echo ""

echo ""
echo "Step 4: Stopping server..."
kill $SERVER_PID 2>/dev/null
sleep 2

echo ""
echo "Step 5: Checking persistence files..."
if [ -f kvstore.rdb ]; then
    echo "  ✓ RDB snapshot file exists ($(stat -f%z kvstore.rdb 2>/dev/null || stat -c%s kvstore.rdb) bytes)"
else
    echo "  ✗ RDB snapshot file not found"
fi

if [ -f kvstore.aof ]; then
    echo "  ✓ AOF log file exists ($(stat -f%z kvstore.aof 2>/dev/null || stat -c%s kvstore.aof) bytes)"
else
    echo "  ✗ AOF log file not found"
fi

echo ""
echo "Step 6: Restarting server to verify recovery..."
../build/kvstore_server --master --address 0.0.0.0:50051 &
SERVER_PID=$!
sleep 2

echo ""
echo "Step 7: Verifying persisted data..."
../build/verify_persistence

echo ""
echo "Cleaning up..."
kill $SERVER_PID 2>/dev/null
sleep 1

echo ""
echo "Test complete!"
