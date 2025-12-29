# Test Suite

Comprehensive tests for the distributed key-value store.

## Overview

The test suite includes:
- **Unit tests** - Standalone component tests (no server required)
- **Integration tests** - End-to-end tests with running servers
- **Test clients** - C++ executables for testing operations

## Running Tests

### Run All Tests

```bash
cd tests
./test_all.sh
```

This runs all tests and provides a summary:
- ✅ Unit tests (hash ring, shard router)
- ✅ Integration tests (operations, persistence, replication, concurrency)

### Run Individual Tests

Unit tests can be run directly from the build directory:

```bash
cd build

# Unit tests (no server needed)
./test_hash_ring       # Test consistent hashing distribution
./test_shard_router    # Test routing logic and connection pooling
```

Integration tests require a running server. Example for basic operations:

```bash
# Terminal 1: Start server
cd build
./kvstore_server --master --address 0.0.0.0:50051

# Terminal 2: Run test client
cd build
./kvstore_client localhost:50051
```

## Test Coverage

### Unit Tests

1. **Hash Ring** (`test_hash_ring`)
   - Virtual node distribution across shards
   - Key assignment consistency
   - Dynamic shard addition/removal
   - Key redistribution when topology changes

2. **Shard Router** (`test_shard_router`)
   - Routing logic and key distribution
   - Connection pooling behavior
   - Statistics tracking
   - Consistent hashing verification

### Integration Tests

1. **Basic Operations**
   - SET, GET, DELETE, CONTAINS operations
   - Client-server communication
   - Single node functionality

2. **TTL & Expiration**
   - EXPIRE command
   - TTL query
   - Automatic expiration
   - Time-based cleanup

3. **Persistence (RDB + AOF)**
   - RDB snapshot creation (60 second interval)
   - AOF log writing
   - Server restart and recovery
   - Data integrity after restart

4. **Master-Replica Replication**
   - Master-replica setup (1 master, 2 replicas)
   - Write to master
   - Read from replicas
   - Eventual consistency verification

5. **Concurrent Clients**
   - Multiple simultaneous clients
   - Thread-safe operations
   - Data consistency under load

## Test Client Executables

Built automatically with the project:

- **kvstore_client** - Full-featured test client
  - Tests: SET, GET, DELETE, CONTAINS, EXPIRE, TTL
  - Source: `client_test.cpp`

- **read_test** - Read-only client
  - Quick GET operations to verify data
  - Used for replication verification
  - Source: `read_test.cpp`

- **snapshot_test** - Persistence test helper
  - Creates sample dataset
  - Source: `snapshot_test.cpp`

- **verify_persistence** - Persistence verification
  - Checks data recovery from RDB/AOF
  - Source: `verify_persistence.cpp`

- **test_hash_ring** - Hash ring unit test
  - Source: `test_hash_ring.cpp`

- **test_shard_router** - Router unit test
  - Source: `test_shard_router.cpp`

## Prerequisites

Build the project to create all test executables:

```bash
mkdir -p build && cd build
cmake ..
make -j8
```

## Test Details

### Persistence Test (~70 seconds)
- Waits 65 seconds for RDB snapshot (60s interval + buffer)
- Verifies both RDB and AOF recovery
- Tests data integrity across restarts

### Replication Test
- Creates temporary `replica1/` and `replica2/` directories
- Tests 3-node cluster (1 master, 2 replicas)
- Verifies asynchronous replication

### Concurrent Clients Test
- Launches 5 simultaneous clients
- Tests thread-safe concurrent access
- Verifies no data corruption under load

## Notes

- `test_all.sh` automatically cleans up processes and temporary files
- Each test runs in isolation with fresh data
- Tests use ports 50051-50053 (ensure they're available)
