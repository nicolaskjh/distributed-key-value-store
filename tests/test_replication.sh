#!/bin/bash

# Test async master-replica replication

echo "========================================="
echo "Testing Async Master-Replica Replication"
echo "========================================="
echo ""

# Clean up any existing data
rm -f kvstore.rdb kvstore.aof
rm -rf replica1 replica2
mkdir -p replica1 replica2

echo "Step 1: Starting Master on port 50051..."
../build/kvstore_server --master --address 0.0.0.0:50051 --replicas localhost:50052,localhost:50053 &
MASTER_PID=$!
sleep 2

echo "Step 2: Starting Replica 1 on port 50052..."
cd replica1 && ../../build/kvstore_server --replica --address 0.0.0.0:50052 --master-address localhost:50051 &
REPLICA1_PID=$!
cd ..
sleep 2

echo "Step 3: Starting Replica 2 on port 50053..."
cd replica2 && ../../build/kvstore_server --replica --address 0.0.0.0:50053 --master-address localhost:50051 &
REPLICA2_PID=$!
cd ..
sleep 2

echo ""
echo "Step 4: Writing data to MASTER (port 50051)..."
../build/kvstore_client localhost:50051

echo ""
echo "Step 5: Waiting 2 seconds for replication..."
sleep 2

echo ""
echo "Step 6: Reading from REPLICA 1 (port 50052)..."
echo "  Expected: Data should match master"
../build/read_test localhost:50052

echo ""
echo "Step 7: Reading from REPLICA 2 (port 50053)..."
echo "  Expected: Data should match master"
../build/read_test localhost:50053

echo ""
echo "Step 8: Verifying eventual consistency..."
../build/kvstore_client localhost:50052 2>&1 | grep "GET name"
../build/kvstore_client localhost:50053 2>&1 | grep "GET name"

echo ""
echo "Cleaning up..."
kill $MASTER_PID $REPLICA1_PID $REPLICA2_PID 2>/dev/null
sleep 1
rm -rf replica1 replica2

echo ""
echo "✓ Replication test complete!"
