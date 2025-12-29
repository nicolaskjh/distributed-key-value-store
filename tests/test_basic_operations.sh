#!/bin/bash

# Test basic key-value operations (GET, SET, DELETE, CONTAINS)

echo "========================================="
echo "Testing Basic Operations"
echo "========================================="
echo ""

# Clean up any existing data
rm -f kvstore.rdb kvstore.aof

echo "Step 1: Starting server on port 50051..."
../build/kvstore_server --master --address 0.0.0.0:50051 &
SERVER_PID=$!
sleep 2

echo ""
echo "Step 2: Running basic operations test..."
echo "  - SET operations"
echo "  - GET operations"
echo "  - CONTAINS checks"
echo "  - DELETE operations"
echo ""

../build/kvstore_client localhost:50051

echo ""
echo "Cleaning up..."
kill $SERVER_PID 2>/dev/null
sleep 1

echo ""
echo "Test complete!"
